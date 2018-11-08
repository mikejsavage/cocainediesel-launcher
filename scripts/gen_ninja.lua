local function copy( t )
	local res = { }
	for k, v in pairs( t ) do
		res[ k ] = v
	end
	return res
end

local configs = {
	[ "windows" ] = {
		bin_suffix = ".exe",
		obj_suffix = ".obj",
		lib_suffix = ".lib",

		toolchain = "msvc",

		cxxflags = "/I . /c /Oi /Gm- /GR- /EHa- /EHsc /nologo /DNOMINMAX /DWIN32_LEAN_AND_MEAN",
		ldflags = "user32.lib shell32.lib advapi32.lib dbghelp.lib /nologo",
		warnings = "/W4 /wd4100 /wd4146 /wd4189 /wd4201 /wd4324 /wd4351 /wd4127 /wd4505 /wd4530 /wd4702 /D_CRT_SECURE_NO_WARNINGS",
	},

	[ "windows-64" ] = { },

	[ "windows-debug" ] = {
		cxxflags = "/Od /MTd /Z7 /Zo",
		ldflags = "/Od /MTd /Z7 /Zo",
	},
	[ "windows-release" ] = {
		cxxflags = "/O2 /MT /DRELEASE_BUILD",
		bin_prefix = "release/",
	},

	[ "linux" ] = {
		obj_suffix = ".o",
		lib_prefix = "lib",
		lib_suffix = ".a",

		toolchain = "gcc",
		cxx = "g++",

		cxxflags = "-I . -c -x c++ -std=c++11 -msse2 -ffast-math -fno-exceptions -fno-rtti -fno-strict-aliasing -fno-strict-overflow -fdiagnostics-color",
		ldflags = "-lm -lpthread -ldl",
		warnings = "-Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wshadow -Wcast-align -Wstrict-overflow -Wvla -Wformat-security", -- -Wconversion
	},

	[ "linux-64" ] = { },

	[ "linux-debug" ] = {
		cxxflags = "-O0 -ggdb3 -fno-omit-frame-pointer",
	},
	[ "linux-asan" ] = {
		bin_suffix = "-asan",
		cxxflags = "-O0 -ggdb3 -fno-omit-frame-pointer -fsanitize=address",
		ldflags = "-fsanitize=address",
	},
	[ "linux-release" ] = {
		cxxflags = "-O2 -DRELEASE_BUILD",
		ldflags = "-s",
		bin_prefix = "release/",
	},

	-- TODO: mingw?
}

configs[ "macos" ] = copy( configs[ "linux" ] )
configs[ "macos" ].cxx = "clang++"
-- TODO: this is not quite right because it can get nuked by gcc_obj_replace_cxxflags
configs[ "macos" ].cxxflags = configs[ "macos" ].cxxflags .. " -mmacosx-version-min=10.9"
configs[ "macos" ].ldflags = "-lm"

configs[ "macos-64" ] = copy( configs[ "linux-64" ] )
configs[ "macos-debug" ] = copy( configs[ "linux-debug" ] )
configs[ "macos-asan" ] = copy( configs[ "linux-asan" ] )
configs[ "macos-release" ] = copy( configs[ "linux-release" ] )
configs[ "macos-release" ].ldflags = "-Wl,-dead_strip -Wl,-x"

local function identify_host()
	local dll_ext = package.cpath:match( "(%a+)$" )

	if dll_ext == "dll" then
		return "windows", "64"
	end

	local p = assert( io.popen( "uname -s" ) )
	local uname = assert( p:read( "*all" ) ):gsub( "%s*$", "" )
	assert( p:close() )

	if uname == "Linux" then
		return "linux", "64"
	end

	if uname == "Darwin" then
		return "macos", "64"
	end

	io.stderr:write( "can't identify host OS" )
	os.exit( 1 )
end

OS, arch = identify_host()
config = arg[ 1 ] or "debug"

local double_arch = OS .. "-" .. arch
local double_config = OS .. "-" .. config
local triple = OS .. "-" .. arch .. "-" .. config

if not configs[ double_arch ] or not configs[ double_config ] then
	io.stderr:write( "bad config: " .. triple .. "\n" )
	os.exit( 1 )
end

local function concat( key )
	return ""
		.. ( ( configs[ OS ] and configs[ OS ][ key ] ) or "" )
		.. " "
		.. ( ( configs[ double_arch ] and configs[ double_arch ][ key ] ) or "" )
		.. " "
		.. ( ( configs[ double_config ] and configs[ double_config ][ key ] ) or "" )
end

local function rightmost( key )
	return nil
		or ( configs[ double_config ] and configs[ double_config ][ key ] )
		or ( configs[ double_arch ] and configs[ double_arch ][ key ] )
		or ( configs[ OS ] and configs[ OS ][ key ] )
		or ""
end

local bin_prefix = rightmost( "bin_prefix" )
local bin_suffix = rightmost( "bin_suffix" )
local obj_suffix = rightmost( "obj_suffix" )
local lib_prefix = rightmost( "lib_prefix" )
local lib_suffix = rightmost( "lib_suffix" )
local cxxflags = concat( "cxxflags" ) .. " " .. concat( "warnings" )
local ldflags = concat( "ldflags" )

toolchain = rightmost( "toolchain" )

local dir = "build/" .. triple
local output = { }

local function flatten_into( res, t )
	for _, x in ipairs( t ) do
		if type( x ) == "table" then
			flatten_into( res, x )
		else
			table.insert( res, x )
		end
	end
end

local function flatten( t )
	local res = { }
	flatten_into( res, t )
	return res
end

local function join( names, suffix, prefix )
	if not names then
		return ""
	end

	prefix = prefix or ""
	local flat = flatten( names )
	for i = 1, #flat do
		flat[ i ] = dir .. "/" .. prefix .. flat[ i ] .. suffix
	end
	return table.concat( flat, " " )
end

local function printf( form, ... )
	print( form:format( ... ) )
end

local objs = { }
local bins = { }
local libs = { }

local function add_srcs( srcs )
	for _, src in ipairs( flatten( srcs ) ) do
		if not objs[ src ] then
			objs[ src ] = { }
		end
	end
end

function bin( bin_name, srcs, libs )
	assert( type( srcs ) == "table", "srcs should be a table" )
	assert( not libs or type( libs ) == "table", "libs should be a table or nil" )
	assert( not bins[ bin_name ] )

	bins[ bin_name ] = {
		srcs = srcs,
		libs = libs,
	}

	add_srcs( srcs )
end

function lib( lib_name, srcs )
	assert( type( srcs ) == "table", "srcs should be a table" )
	assert( not libs[ lib_name ] )

	libs[ lib_name ] = {
		srcs = srcs,
	}

	add_srcs( srcs )
end

function rc( rc_name )
	if OS == "windows" then
		printf( "%s/%s%s: %s.rc %s.xml", dir, rc_name, obj_suffix, rc_name, rc_name )
		printf( "\t@printf \"\\033[1;33mbuilding $@\\033[0m\\n\"" )
		printf( "\t@mkdir -p \"$(@D)\"" )
		printf( "\t@rc /fo$@ /nologo $<" )
	else
		local cxx = rightmost( "cxx" )
		printf( "build %s/%s%s: rc", dir, rc_name, obj_suffix )
	end
end

function bin_ldflags( bin_name, ldflags )
	bins[ bin_name ].extra_ldflags = ( bins[ bin_name ].extra_ldflags or "" ) .. " " .. ldflags
end

function obj_cxxflags( src_name, cxxflags )
	objs[ src_name ].extra_cxxflags = ( objs[ src_name ].extra_cxxflags or "" ) .. " " .. cxxflags
end

function obj_replace_cxxflags( src_name, cxxflags )
	for name, cfg in pairs( objs ) do
		if name:match( src_name ) then
			cfg.cxxflags = cxxflags
		end
	end
end

local function toolchain_helper( t, f )
	return function( ... )
		if toolchain == t then
			f( ... )
		end
	end
end

msvc_bin_ldflags = toolchain_helper( "msvc", bin_ldflags )
msvc_obj_cxxflags = toolchain_helper( "msvc", obj_cxxflags )
msvc_obj_replace_cxxflags = toolchain_helper( "msvc", obj_replace_cxxflags )

gcc_bin_ldflags = toolchain_helper( "gcc", bin_ldflags )
gcc_obj_cxxflags = toolchain_helper( "gcc", obj_cxxflags )
gcc_obj_replace_cxxflags = toolchain_helper( "gcc", obj_replace_cxxflags )

printf( "cxxflags = %s", cxxflags )
printf( "ldflags = %s", ldflags )

if toolchain == "msvc" then

printf( [[
VC = ${ProgramFiles(x86)}\Microsoft Visual Studio 14.0\VC
KIT81 = ${ProgramFiles(x86)}\Windows Kits\8.1
KIT10 = ${ProgramFiles(x86)}\Windows Kits\10
DX = ${ProgramFiles(x86)}\Microsoft DirectX SDK (June 2010)

export INCLUDE := $(VC)\include;$(KIT10)\Include\10.0.10240.0\ucrt;$(KIT81)\Include\shared;$(DX)\Include;$(KIT81)\Include\um;$(KIT81)\Include\winrt
export LIB := $(VC)\lib\amd64;$(KIT10)\Lib\10.0.10240.0\ucrt\x64;$(KIT81)\lib\winv6.3\um\x64
export PATH := /cygdrive/c/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/amd64/:/cygdrive/c/Program Files (x86)/Windows Kits/8.1/bin/x64/:$(PATH)
]] )

printf( [[
$(BINS): %%:
	@printf "\033[1;31mbuilding $@\033[0m\n"
	@mkdir -p "$(@D)"
	@cl -Fe$@ $^ $(LDFLAGS)
]] )
printf( [[
%s/%%%s: %%.cc
	@printf "\033[1;32mbuilding $<\033[0m\n"
	@mkdir -p "$(@D)"
	@cl $(CXXFLAGS) -Fo$@ $^
]], dir, obj_suffix )
printf( [[
$(OBJS): %%:
	@printf "\033[1;32mbuilding $<\033[0m\n"
	@mkdir -p "$(@D)"
	@cl $(CXXFLAGS) -Fo$@ -Tp$<
]] )
printf( [[
%%%s:
	@printf "\033[1;35mbuilding $@\033[0m\n"
	@mkdir -p "$(@D)"
	@lib -OUT:$@ $^
]], lib_suffix )

elseif toolchain == "gcc" then

local cxx = rightmost( "cxx" )

printf( [[
cc = %s
cpp = %s

rule cpp
    command = $cpp -MD -MF $out.d $cxxflags $extra_cxxflags -c  -o $out $in
    depfile = $out.d
    deps = gcc

rule m
    command = $cpp -MD -MF $out.d $mflags $extra_mflags -c  -o $out $in
    depfile = $out.d
    deps = gcc

rule bin
    command = $cpp -o $out $in $ldflags $extra_ldflags

rule lib
    command = ar rs $out $in

rule rc
    command = $cpp -c -x c++ /dev/null -o $out
]], "gcc", cxx )

end

-- function bin( bin_name, srcs, libs )
-- 	assert( type( srcs ) == "table", "srcs should be a table" )
-- 	assert( not libs or type( libs ) == "table", "libs should be a table or nil" )
-- 	local bin_path = ( "%s%s%s" ):format( bin_prefix, bin_name, bin_suffix )
-- 	printh( "default %s", bin_path )
-- 	printf( "build %s: bin %s %s", bin_path, join( srcs, obj_suffix ), join( libs, lib_suffix, lib_prefix ) )
-- end
--
-- function lib( lib_name, srcs )
-- 	assert( type( srcs ) == "table", "srcs should be a table" )
-- 	printf( "build %s/%s%s%s: lib %s", dir, lib_prefix, lib_name, lib_suffix, join( srcs, obj_suffix ) )
-- end

local function rule_for_src( src_name )
	local ext = src_name:match( "([^%.]+)$" )
	return ( { cc = "cpp", m = "m" } )[ ext ]
end

automatically_print_output_at_exit = setmetatable( { }, {
	__gc = function()
		for src_name, cfg in pairs( objs ) do
			local rule = rule_for_src( src_name )
			printf( "build %s/%s%s: %s %s", dir, src_name, obj_suffix, rule, src_name )
			if cfg.cxxflags then
				printf( "    cxxflags = %s", cfg.cxxflags )
			end
			if cfg.extra_cxxflags then
				printf( "    extra_cxxflags = %s", cfg.extra_cxxflags )
			end
		end

		for lib_name, cfg in pairs( libs ) do
			printf( "build %s/%s%s%s: lib %s", dir, lib_prefix, lib_name, lib_suffix, join( cfg.srcs, obj_suffix ) )
		end

		for bin_name, cfg in pairs( bins ) do
			local bin_path = ( "%s%s%s" ):format( bin_prefix, bin_name, bin_suffix )
			printf( "build %s: bin %s %s", bin_path, join( cfg.srcs, obj_suffix ), join( cfg.libs, lib_suffix, lib_prefix ) )
			if cfg.ldflags then
				printf( "    ldflags = %s", cfg.ldflags )
			end
			if cfg.extra_ldflags then
				printf( "    extra_ldflags = %s", cfg.extra_ldflags )
			end
			printf( "default %s", bin_path )
		end
	end
} )
