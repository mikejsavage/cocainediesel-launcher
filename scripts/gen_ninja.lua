local function copy( t, extra )
	local res = { }
	for k, v in pairs( t ) do
		res[ k ] = v
	end
	if extra then
		for k, v in pairs( extra ) do
			res[ k ] = v
		end
	end
	return res
end

local zig
do
    local f = assert( io.open( "scripts/zig_version.txt", "r" ) )
    local zig_version = assert( f:read( "*all" ) ):gsub( "%s+$", "" )
    assert( f:close() )
    zig = "scripts/zig-" .. zig_version .. "/zig"
end

local configs = { }

configs[ "windows" ] = {
	bin_suffix = ".exe",
	obj_suffix = ".obj",
	lib_suffix = ".lib",

	toolchain = "msvc",

	cxxflags = "/I . /c /Oi /Gm- /GR- /EHa- /EHsc /nologo /DNOMINMAX /DWIN32_LEAN_AND_MEAN",
	ldflags = "user32.lib shell32.lib advapi32.lib dbghelp.lib /nologo",
	warnings = "/W4 /wd4100 /wd4146 /wd4189 /wd4201 /wd4324 /wd4351 /wd4127 /wd4505 /wd4530 /wd4702 /D_CRT_SECURE_NO_WARNINGS",
}

configs[ "windows-debug" ] = {
	cxxflags = "/MTd /Z7",
	ldflags = "/NOLOGO /DEBUG:FULL /FUNCTIONPADMIN /OPT:NOREF /OPT:NOICF",
}
configs[ "windows-release" ] = {
	cxxflags = "/O2 /MT /DRELEASE_BUILD",
	output_dir = "release/",
}

configs[ "linux" ] = {
	obj_suffix = ".o",
	lib_prefix = "lib",
	lib_suffix = ".a",

	toolchain = "gcc",
	cxx = zig .. " c++",
	ar = zig .. " ar",

	cxxflags = "-I . -c -std=c++17 -msse2 -fno-exceptions -fno-rtti -fno-strict-aliasing -fno-strict-overflow -fdiagnostics-color",
	warnings = "-Wall -Wextra -Wno-unused-parameter -Wno-unused-function -Wshadow -Wcast-align -Wstrict-overflow -Wvla -Wformat-security", -- -Wconversion
}

configs[ "linux-debug" ] = {
	cxxflags = "-g -fno-omit-frame-pointer",
}
configs[ "linux-asan" ] = {
	bin_suffix = "-asan",
	cxxflags = "-g -fno-omit-frame-pointer -fsanitize=address",
	ldflags = "-fsanitize=address",
}
configs[ "linux-release" ] = {
	output_dir = "release/",
	cxxflags = "-O2 -DRELEASE_BUILD",
	ldflags = "-fstrip",
	can_static_link = true,
}

configs[ "macos" ] = copy( configs[ "linux" ], {
	cxx = "clang++",
	ldflags = "",
	cxxflags = configs[ "linux" ].cxxflags .. " -mmacosx-version-min=10.13",
} )
configs[ "macos-debug" ] = {
	cxxflags = "-O0 -g -fno-omit-frame-pointer",
}
configs[ "macos-release" ] = copy( configs[ "linux-release" ], {
	ldflags = "-Wl,-dead_strip -Wl,-x",
} )

OS = os.name:lower()
config = arg[ 1 ] or "debug"

local OS_config = OS .. "-" .. config

if not configs[ OS_config ] then
	io.stderr:write( "bad config: " .. OS_config .. "\n" )
	os.exit( 1 )
end

local function concat( key )
	return ""
		.. ( ( configs[ OS ] and configs[ OS ][ key ] ) or "" )
		.. " "
		.. ( ( configs[ OS_config ] and configs[ OS_config ][ key ] ) or "" )
end

local function rightmost( key )
	return nil
		or ( configs[ OS_config ] and configs[ OS_config ][ key ] )
		or ( configs[ OS ] and configs[ OS ][ key ] )
		or ""
end

local output_dir = rightmost( "output_dir" )
local bin_suffix = rightmost( "bin_suffix" )
local obj_suffix = rightmost( "obj_suffix" )
local lib_prefix = rightmost( "lib_prefix" )
local lib_suffix = rightmost( "lib_suffix" )
local prebuilt_lib_dir = rightmost( "prebuilt_lib_dir" )
prebuilt_lib_dir = prebuilt_lib_dir == "" and OS_config or prebuilt_lib_dir
local cxxflags = concat( "cxxflags" ) .. " " .. concat( "warnings" )
local ldflags = concat( "ldflags" )
local can_static_link = rightmost( "can_static_link" ) == true

toolchain = rightmost( "toolchain" )

local dir = "build/" .. OS_config
local output = { }

local objs = { }
local bins = { }
local libs = { }
local prebuilt_libs = { }

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
	if not t then
		return { }
	end

	local res = { }
	flatten_into( res, t )
	return res
end

local function join_srcs( names )
	if not names then
		return ""
	end

	local flat = flatten( names )
	for i = 1, #flat do
		flat[ i ] = dir .. "/" .. flat[ i ] .. obj_suffix
	end
	return table.concat( flat, " " )
end

local function join_libs( names )
	local joined = { }
	for _, lib in ipairs( flatten( names ) ) do
		local prebuilt_lib = prebuilt_libs[ lib ]

		if prebuilt_lib then
			for _, archive in ipairs( prebuilt_lib ) do
				table.insert( joined, "libs/" .. lib .. "/" .. prebuilt_lib_dir .. "/" .. lib_prefix .. archive .. lib_suffix )
			end
		else
			table.insert( joined, dir .. "/" .. lib_prefix .. lib .. lib_suffix )
		end
	end

	return table.concat( joined, " " )
end

local function printf( form, ... )
	print( form and form:format( ... ) or "" )
end

local function add_srcs( srcs )
	for _, src in ipairs( srcs ) do
		if not objs[ src ] then
			objs[ src ] = { }
		end
	end
end

function bin( bin_name, cfg )
	assert( type( cfg ) == "table", "cfg should be a table" )
	assert( type( cfg.srcs ) == "table", "cfg.srcs should be a table" )
	assert( not cfg.libs or type( cfg.libs ) == "table", "cfg.libs should be a table or nil" )
	assert( not bins[ bin_name ] )

	bins[ bin_name ] = cfg

	add_srcs( cfg.srcs )
end

function lib( lib_name, srcs )
	assert( type( srcs ) == "table", "srcs should be a table" )
	assert( not libs[ lib_name ] )

	libs[ lib_name ] = srcs
	add_srcs( srcs )
end

function prebuilt_lib( lib_name, archives )
	assert( not prebuilt_libs[ lib_name ] )
	prebuilt_libs[ lib_name ] = archives or { lib_name }
end

function obj_cxxflags( src_name, cxxflags )
	for name, cfg in pairs( objs ) do
		if name:match( src_name ) then
			cfg.extra_cxxflags = ( cfg.extra_cxxflags or "" ) .. " " .. cxxflags
		end
	end
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

msvc_obj_cxxflags = toolchain_helper( "msvc", obj_cxxflags )
msvc_obj_replace_cxxflags = toolchain_helper( "msvc", obj_replace_cxxflags )

gcc_obj_cxxflags = toolchain_helper( "gcc", obj_cxxflags )
gcc_obj_replace_cxxflags = toolchain_helper( "gcc", obj_replace_cxxflags )

local function sort_by_key( t )
	local ret = { }
	for k, v in pairs( t ) do
		table.insert( ret, { key = k, value = v } )
	end
	table.sort( ret, function( a, b ) return a.key < b.key end )

	function iter()
		for _, x in ipairs( ret ) do
			coroutine.yield( x.key, x.value )
		end
	end

	return coroutine.wrap( iter )
end

function write_ninja_script()
	printf( "builddir = build" )
	printf( "cxxflags = %s", cxxflags )
	printf( "ldflags = %s", ldflags )
	printf()

	if toolchain == "msvc" then

printf( [[
rule cpp
    command = cl /showIncludes $cxxflags $extra_cxxflags -Fo$out $in
    description = $in
    deps = msvc

rule bin
    command = link /OUT:$out $in $ldflags $extra_ldflags
    description = $out

rule lib
    command = lib /NOLOGO /OUT:$out $in
    description = $out

rule rc
    command = rc /fo$out /nologo $in_rc
    description = $in
]] )

else

printf( "cpp = %s", rightmost( "cxx" ) )
printf( "ar = %s", rightmost( "ar" ) )

printf( [[
rule zig
    command = scripts/download_zig.sh
    description = Downloading zig, this can be slow and first time linking is slow, subsequent builds will go fast
    generator = true

build %s: zig scripts/download_zig.sh

rule cpp
    command = $cpp -MD -MF $out.d $cxxflags $extra_cxxflags -c -o $out $in --target=x86_64-linux-musl
    depfile = $out.d
    description = $in
    deps = gcc

rule bin
    command = %s build-exe -femit-bin=$out $in -lc -lc++ $ldflags $extra_ldflags
    description = $out

rule bin-static
    command = %s build-exe -femit-bin=$out $in -lc -lc++ $ldflags $extra_ldflags -target x86_64-linux-musl -static
    description = $out

rule lib
    command = ar cr $out $in
    description = $out
]], zig, zig, zig )

printf( [[
]] )

	end

	for src_name, cfg in sort_by_key( objs ) do
		printf( "build %s/%s%s: cpp %s%s", dir, src_name, obj_suffix, src_name, OS == "linux" and ( " | " .. zig ) or "" )
		if cfg.cxxflags then
			printf( "    cxxflags = %s", cfg.cxxflags )
		end
		if cfg.extra_cxxflags then
			printf( "    extra_cxxflags = %s", cfg.extra_cxxflags )
		end
	end

	printf()

	for lib_name, srcs in sort_by_key( libs ) do
		printf( "build %s/%s%s%s: lib %s", dir, lib_prefix, lib_name, lib_suffix, join_srcs( srcs ) )
	end

	printf()

	for bin_name, cfg in sort_by_key( bins ) do
		local srcs = { cfg.srcs }

		if OS == "windows" and cfg.rc then
			srcs = { cfg.srcs, cfg.rc }
			printf( "build %s/%s%s: rc %s.rc", dir, cfg.rc, obj_suffix, cfg.rc )
			printf( "    in_rc = %s.rc", cfg.rc )
		end

		local full_name = output_dir .. bin_name .. bin_suffix
		printf( "build %s: %s %s %s%s",
			full_name,
			( can_static_link and not cfg.no_static_link ) and "bin-static" or "bin",
			join_srcs( srcs ),
			join_libs( cfg.libs ),
			OS == "linux" and ( " | " .. zig ) or ""
		)

		local ldflags_key = OS .. "_ldflags"
		if cfg[ ldflags_key ] then
			printf( "    extra_ldflags = %s", cfg[ ldflags_key ] )
		end

		printf( "default %s", full_name )
	end
end
