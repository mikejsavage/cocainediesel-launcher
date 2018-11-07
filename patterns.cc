/*
 * this is derived from OpenBSD's patterns.c, which is itself derived from
 * Lua's lstrlib.c
 */

/*	$OpenBSD: patterns.c,v 1.5 2016/02/14 18:20:59 semarie Exp $	*/

/*
 * Copyright (c) 2015 Reyk Floeter <reyk@openbsd.org>
 * Copyright (C) 1994-2015 Lua.org, PUC-Rio.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Derived from Lua 5.3.1:
 * $Id: patterns.c,v 1.5 2016/02/14 18:20:59 semarie Exp $
 * Standard library for string operations and pattern-matching
 */

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "patterns.h"

#define uchar(c)	((unsigned char)(c)) /* macro to 'unsign' a char */
#define CAP_UNFINISHED	(-1)
#define CAP_POSITION	(-2)
#define L_ESC		'%'
#define SPECIALS	"^$*+?.([%-"

/* recursive function */
static const char *match(struct match_state *, const char *, const char *);

static int
match_error(struct match_state *ms, const char *error)
{
	ms->error = ms->error == NULL ? error : ms->error;
	return (-1);
}

static int
check_capture(struct match_state *ms, int l)
{
	l -= '1';
	if (l < 0 || l >= ms->level || ms->capture[l].len == CAP_UNFINISHED)
		return match_error(ms, "invalid capture index");
	return (l);
}

static int
capture_to_close(struct match_state *ms)
{
	int level = ms->level;
	for (level--; level >= 0; level--)
		if (ms->capture[level].len == CAP_UNFINISHED)
			return (level);
	return match_error(ms, "invalid pattern capture");
}

static const char *
classend(struct match_state *ms, const char *p)
{
	switch (*p++) {
	case L_ESC:
		if (p == ms->p_end)
			match_error(ms,
			    "malformed pattern (ends with '%')");
		return p + 1;
	case '[':
		if (*p == '^')
			p++;
		do {
			/* look for a ']' */
			if (p == ms->p_end) {
				match_error(ms,
				    "malformed pattern (missing ']')");
				break;
			}
			if (*(p++) == L_ESC && p < ms->p_end) {
				/* skip escapes (e.g. '%]') */
				p++;
			}
		} while (*p != ']');
		return p + 1;
	default:
		return p;
	}
}

static int
match_class(int c, int cl)
{
	int res;
	switch (tolower(cl)) {
	case 'a':
		res = isalpha(c);
		break;
	case 'c':
		res = iscntrl(c);
		break;
	case 'd':
		res = isdigit(c);
		break;
	case 'g':
		res = isgraph(c);
		break;
	case 'l':
		res = islower(c);
		break;
	case 'p':
		res = ispunct(c);
		break;
	case 's':
		res = isspace(c);
		break;
	case 'u':
		res = isupper(c);
		break;
	case 'w':
		res = isalnum(c);
		break;
	case 'x':
		res = isxdigit(c);
		break;
	default:
		return (cl == c);
	}
	return (islower(cl) ? res : !res);
}

static int
matchbracketclass(int c, const char *p, const char *ec)
{
	int sig = 1;
	if (*(p + 1) == '^') {
		sig = 0;
		/* skip the '^' */
		p++;
	}
	while (++p < ec) {
		if (*p == L_ESC) {
			p++;
			if (match_class(c, uchar(*p)))
				return sig;
		} else if ((*(p + 1) == '-') && (p + 2 < ec)) {
			p += 2;
			if (uchar(*(p - 2)) <= c && c <= uchar(*p))
				return sig;
		} else if (uchar(*p) == c)
			return sig;
	}
	return !sig;
}

static int
singlematch(struct match_state *ms, const char *s, const char *p,
    const char *ep)
{
	if (s >= ms->src_end)
		return 0;
	else {
		int c = uchar(*s);
		switch (*p) {
		case '.':
			/* matches any char */
			return (1);
		case L_ESC:
			return match_class(c, uchar(*(p + 1)));
		case '[':
			return matchbracketclass(c, p, ep - 1);
		default:
			return (uchar(*p) == c);
		}
	}
}

static const char *
matchbalance(struct match_state *ms, const char *s, const char *p)
{
	if (p >= ms->p_end - 1) {
		match_error(ms,
		    "malformed pattern (missing arguments to '%b')");
		return (NULL);
	}
	if (*s != *p)
		return (NULL);
	else {
		int b = *p;
		int e = *(p + 1);
		int cont = 1;
		while (++s < ms->src_end) {
			if (*s == e) {
				if (--cont == 0)
					return s + 1;
			} else if (*s == b)
				cont++;
		}
	}

	/* string ends out of balance */
	return (NULL);
}

static const char *
max_expand(struct match_state *ms, const char *s, const char *p, const char *ep)
{
	ptrdiff_t i = 0;
	/* counts maximum expand for item */
	while (singlematch(ms, s + i, p, ep))
		i++;
	/* keeps trying to match with the maximum repetitions */
	while (i >= 0) {
		const char *res = match(ms, (s + i), ep + 1);
		if (res)
			return res;
		/* else didn't match; reduce 1 repetition to try again */
		i--;
	}
	return NULL;
}

static const char *
min_expand(struct match_state *ms, const char *s, const char *p, const char *ep)
{
	for (;;) {
		const char *res = match(ms, s, ep + 1);
		if (res != NULL)
			return res;
		else if (singlematch(ms, s, p, ep))
			s++;	/* try with one more repetition */
		else
			return NULL;
	}
}

static const char *
start_capture(struct match_state *ms, const char *s, const char *p, int what)
{
	const char *res;

	int level = ms->level;
	if (level >= ms->maxcaptures) {
		match_error(ms, "too many captures");
		return (NULL);
	}
	ms->capture[level].init = s;
	ms->capture[level].len = what;
	ms->level = level + 1;
	/* undo capture if match failed */
	if ((res = match(ms, s, p)) == NULL)
		ms->level--;
	return res;
}

static const char *
end_capture(struct match_state *ms, const char *s, const char *p)
{
	int l = capture_to_close(ms);
	const char *res;
	if (l == -1)
		return NULL;
	/* close capture */
	ms->capture[l].len = s - ms->capture[l].init;
	/* undo capture if match failed */
	if ((res = match(ms, s, p)) == NULL)
		ms->capture[l].len = CAP_UNFINISHED;
	return res;
}

static const char *
match_capture(struct match_state *ms, const char *s, int l)
{
	size_t len;
	l = check_capture(ms, l);
	if (l == -1)
		return NULL;
	len = ms->capture[l].len;
	if ((size_t) (ms->src_end - s) >= len &&
	    memcmp(ms->capture[l].init, s, len) == 0)
		return s + len;
	else
		return NULL;
}

static const char *
match(struct match_state *ms, const char *s, const char *p)
{
	const char *ep, *res;
	char previous;

	if (ms->matchdepth-- == 0) {
		match_error(ms, "pattern too complex");
		return (NULL);
	}

	/* using goto's to optimize tail recursion */
 init:
	/* end of pattern? */
	if (p != ms->p_end) {
		switch (*p) {
		case '(':
			/* start capture */
			if (*(p + 1) == ')')
				/* position capture? */
				s = start_capture(ms, s, p + 2, CAP_POSITION);
			else
				s = start_capture(ms, s, p + 1, CAP_UNFINISHED);
			break;
		case ')':
			/* end capture */
			s = end_capture(ms, s, p + 1);
			break;
		case '$':
			/* is the '$' the last char in pattern? */
			if ((p + 1) != ms->p_end) {
				/* no; go to default */
				goto dflt;
			}
			 /* check end of string */
			s = (s == ms->src_end) ? s : NULL;
			break;
		case L_ESC:
			/* escaped sequences not in the format class[*+?-]? */
			switch (*(p + 1)) {
			case 'b':
				/* balanced string? */
				s = matchbalance(ms, s, p + 2);
				if (s != NULL) {
					p += 4;
					/* return match(ms, s, p + 4); */
					goto init;
				} /* else fail (s == NULL) */
				break;
			case 'f':
				/* frontier? */
				p += 2;
				if (*p != '[') {
					match_error(ms, "missing '['"
					    " after '%f' in pattern");
					break;
				}
				/* points to what is next */
				ep = classend(ms, p);
				if (ms->error != NULL)
					break;
				previous =
				    (s == ms->src_init) ? '\0' : *(s - 1);
				if (!matchbracketclass(uchar(previous),
				    p, ep - 1) &&
				    matchbracketclass(uchar(*s),
				    p, ep - 1)) {
					p = ep;
					/* return match(ms, s, ep); */
					goto init;
				}
				/* match failed */
				s = NULL;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				/* capture results (%0-%9)? */
				s = match_capture(ms, s, uchar(*(p + 1)));
				if (s != NULL) {
					p += 2;
					/* return match(ms, s, p + 2) */
					goto init;
				}
				break;
			default:
				goto dflt;
			}
			break;
		default:

			/* pattern class plus optional suffix */
	dflt:
			/* points to optional suffix */
			ep = classend(ms, p);
			if (ms->error != NULL)
				break;

			/* does not match at least once? */
			if (!singlematch(ms, s, p, ep)) {
				if (ms->repetitioncounter-- == 0) {
					match_error(ms, "max repetition items");
					s = NULL; /* fail */
				/* accept empty? */
				} else if
				    (*ep == '*' || *ep == '?' || *ep == '-') {
					 p = ep + 1;
					/* return match(ms, s, ep + 1); */
					 goto init;
				} else {
					/* '+' or no suffix */
					s = NULL; /* fail */
				}
			} else {
				/* matched once */
				/* handle optional suffix */
				switch (*ep) {
				case '?':
					/* optional */
					if ((res =
					    match(ms, s + 1, ep + 1)) != NULL)
						s = res;
					else {
						/* 
						 * else return
						 *     match(ms, s, ep + 1);
						 */
						p = ep + 1;
						goto init;
					}
					break;
				case '+':
					/* 1 or more repetitions */
					s++; /* 1 match already done */
					/* FALLTHROUGH */
				case '*':
					/* 0 or more repetitions */
					s = max_expand(ms, s, p, ep);
					break;
				case '-':
					/* 0 or more repetitions (minimum) */
					s = min_expand(ms, s, p, ep);
					break;
				default:
					/* no suffix */
					s++;
					p = ep;
					/* return match(ms, s + 1, ep); */
					goto init;
				}
			}
			break;
		}
	}
	ms->matchdepth++;
	return s;
}

int
gmatch_aux(gmatch_state * gm)
{
	const char *src;
	for (src = gm->src; src <= gm->ms.src_end;) {
		const char *e;
		gm->ms.level = 0;
		if ((e = match(&gm->ms, src, gm->p)) != NULL && e != gm->lastmatch) {
			gm->src = gm->lastmatch = e;
			return 1;
		}
		else src++;
		ASSERT( gm->ms.error == NULL );
	}
	return 0;  /* not found */
}

void
gmatch_init(const char * str, const char * pattern, gmatch_state * gm)
{
	memset(gm, 0, sizeof(*gm));

	gm->ms.maxcaptures = MAXCAPTURES;
	gm->ms.matchdepth = MAXCCALLS;
	gm->ms.repetitioncounter = MAXREPETITION;
	gm->ms.src_init = str;
	gm->ms.src_end = str + strlen(str);
	gm->ms.p_end = pattern + strlen(pattern);

	gm->src = str;
	gm->p = pattern;
	gm->lastmatch = NULL;
}

bool match( Matches * matches, array< const char > str, const char * pattern ) {
	match_state ms;
	memset( &ms, 0, sizeof( ms ) );

	bool anchor = pattern[ 0 ] == '^';
	if( anchor ) {
		pattern++;
	}

	ms.maxcaptures = MAXCAPTURES;
	ms.matchdepth = MAXCCALLS;
	ms.repetitioncounter = MAXREPETITION;
	ms.src_init = str.ptr();
	ms.src_end = str.ptr() + str.n;
	ms.p_end = pattern + strlen( pattern );

	for( size_t i = 0; i < str.n; i++ ) {
		ms.level = 0;
		const char * end = match( &ms, str.ptr() + i, pattern );
		ASSERT( ms.error == NULL );
		if( end != NULL ) {
			break;
		}
		if( anchor ) {
			break;
		}
	}

	matches->matches = matches->matches_data.slice( 0, ms.level );
	for( int i = 0; i < ms.level; i++ ) {
		ssize_t len = ms.capture[ i ].len;
		if( len == CAP_POSITION ) len = 0;
		matches->matches_data[ i ] = array< const char >( ms.capture[ i ].init, checked_cast< size_t >( len ) );
	}

	return ms.level > 0;
}

bool match( Matches * matches, const char * str, const char * pattern ) {
	return match( matches, array< const char >( str, strlen( str ) ), pattern );
}

gmatch::Iterator::Iterator( gmatch_state * gm_ ) {
	if( gm_ == NULL ) {
		done = true;
		return;
	}

	gm = gm_;
	done = false;
	next();
}

array< array< const char > > gmatch::Iterator::operator*() {
	return matches.matches;
}

void gmatch::Iterator::operator++() {
	next();
}

bool gmatch::Iterator::operator!=( const Iterator & other ) {
	return done != other.done;
}

void gmatch::Iterator::next() {
	int ok = gmatch_aux( gm );
	if( ok == 0 ) {
		done = true;
		return;
	}

	matches.matches = matches.matches_data.slice( 0, gm->ms.level );
	for( int i = 0; i < gm->ms.level; i++ ) {
		ssize_t len = gm->ms.capture[ i ].len;
		if( len == CAP_POSITION ) len = 0;
		matches.matches_data[ i ] = array< const char >( gm->ms.capture[ i ].init, checked_cast< size_t >( len ) );
	}
}

gmatch::gmatch( const char * str, const char * pattern ) {
	gmatch_init( str, pattern, &gm );
}
