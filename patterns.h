/*	$OpenBSD: patterns.h,v 1.3 2015/12/12 19:59:43 mmcc Exp $	*/

/*
 * Copyright (c) 2015 Reyk Floeter <reyk@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef PATTERNS_H
#define PATTERNS_H

#include <stddef.h>

#include "platform.h"
#include "array.h"

#if PLATFORM_WINDOWS && !defined( ssize_t )
#define ssize_t long long
#endif

#define MAXCAPTURES	32	/* Max no. of allowed captures in pattern */
#define MAXCCALLS	200	/* Max recusion depth in pattern matching */
#define MAXREPETITION	0xfffff	/* Max for repetition items */

struct match_state {
	int matchdepth;		/* control for recursive depth (to avoid C
				 * stack overflow) */
	int repetitioncounter;	/* control the repetition items */
	int maxcaptures;	/* configured capture limit */
	const char *src_init;	/* init of source string */
	const char *src_end;	/* end ('\0') of source string */
	const char *p_end;	/* end ('\0') of pattern */
	const char *error;	/* should be NULL */
	int level;		/* total number of captures (finished or
				 * unfinished) */
	struct {
		const char * init;
		ssize_t len;
	} capture[MAXCAPTURES];
};

struct gmatch_state {
	const char *src;
	const char *p;
	const char *lastmatch;
	match_state ms;
};

struct Matches {
	const array< const char > operator[]( size_t i ) {
		return matches[ i ];
	}

	StaticArray< array< const char >, MAXCAPTURES > matches_data;
	array< array< const char > > matches;
};

bool match( Matches * matches, const char * str, const char * pattern );
bool match( Matches * matches, array< const char > str, const char * pattern );

class gmatch {
	gmatch_state gm;

	struct Iterator {
		Iterator( gmatch_state * gm );
		array< array< const char > > operator*();
		void operator++();
		bool operator!=( const Iterator & other );
		void next();

		bool done;
		gmatch_state * gm;
		Matches matches;
	};

public:
	gmatch( const char * str, const char * pattern );

	Iterator begin() {
		return Iterator( &gm );
	}

	Iterator end() {
		return Iterator( NULL );
	}
};

#endif /* PATTERNS_H */
