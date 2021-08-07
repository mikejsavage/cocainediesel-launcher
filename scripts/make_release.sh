#! /bin/sh

set -e

version=$(basename $(pwd))
find . -type f -exec chmod 644 {} \;
mkdir release

file_platform() {
	if [ "${1: -4}" = ".exe" ]; then
		echo " windows64"
	elif [ "$1" = "client" ] || [ "$1" = "server" ] || [ "$1" = "cocainediesel" ] || [ "$1" = "headlessupdater" ]; then
		echo " linux64"
	fi
}

# create manifest
find * -type f -print0 | sort -z | while IFS= read -r -d '' f; do
	digest=$(../b2sum < "$f")
	size=$(stat -c "%s" "$f")
	platform=$(file_platform "$f")
	cp "$f" "release/$digest"

	if echo "$f" | grep -q " "; then
		echo "filenames can't have spaces - $f"
		exit 1
	fi

	if [[ $size = 0 ]]; then
		echo "$f has size 0"
		exit 1
	fi

	echo "$f $digest $size$platform" >> "release/manifest.txt"
done

# sign manifest
../sign "release/manifest.txt" | cat - "release/manifest.txt" > "release/$version.txt"
rm release/manifest.txt
