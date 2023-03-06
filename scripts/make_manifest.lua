local lfs = require( "INTERNAL_LFS" )

local function FindAllFilesRecursive( files, dir )
	for f in lfs.dir( dir ) do
		if f ~= "." and f ~= ".." then
			local path = dir .. "/" .. f
			local attr = lfs.attributes( path )

			if attr.mode == "directory" then
				FindAllFilesRecursive( files, path )
			elseif not path:match( "^%./release" ) then
				-- trim leading ./
				table.insert( files, {
					path = path:sub( 3 ),
					size = attr.size,
				} )
			end
		end
	end
end

local function FindAllFiles()
	local files = { }
	FindAllFilesRecursive( files, "." )
	return files
end

local function B2Sum( path )
	local b2sum = assert( io.popen( "../b2sum < " .. path, "r" ) )
	local digest = ( assert( b2sum:read( "*all" ) ):gsub( "\n$", "" ) )
	assert( b2sum:close() )
	return digest
end

local function FilePlatform( path, contents )
	if path:match( "%.exe$" ) or path:match( "%.dll$" ) then
		return " windows64"
	end

	local f = assert( io.open( path, "r" ) )
	local elf = assert( f:read( 4 ) )
	assert( f:close() )

	if elf == "\x7fELF" then
		return " linux64"
	end

	return ""
end

local files = FindAllFiles()
table.sort( files, function( a, b ) return a.path < b.path end )

for _, file in ipairs( files ) do
	local digest = B2Sum( file.path )
	local platform = FilePlatform( file.path )

	if file.path:match( " " ) then
		print( "Filenames can't contain spaces: " .. file.path )
		return 1
	end

	if file.size == 0 then
		print( file.path .. " is empty" )
		return 1
	end

	print( file.path .. " " .. digest .. " " .. file.size .. platform )
end
