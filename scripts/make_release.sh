#! /bin/sh

version=$(basename $(pwd))
find . -type f | xargs chmod 644
mkdir release

file_platform() {
	if [ "${1: -4}" = ".exe" ] || [ "${1: -4}" = ".dll" ]; then
		echo " windows64"
	elif [ "$1" = "client" ] || [ "$1" = "server" ] || [ "$1" = "cocainediesel" ] || [ "${1: -3}" = ".so" ]; then
		echo " linux64"
	fi
}

# create manifest
find * -type f -print0 | sort -z | while IFS= read -r -d '' f; do
	digest=$(../b2sum < "$f")
	size=$(stat -c "%s" "$f")
	platform=$(file_platform "$f")
	cp "$f" "release/$digest"
	echo "$f $digest $size$platform" >> "release/manifest.txt"
done

# sign manifest
../sign "release/manifest.txt" | cat - "release/manifest.txt" > "release/$version.txt"
rm release/manifest.txt
