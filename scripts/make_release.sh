#! /bin/sh

version=$(basename $(pwd))
find . -type f | xargs chmod 644
mkdir release

file_platform() {
	if [ "$1" = "cocaine_diesel.x86_64.exe" ] || [ "$1" = "launch.exe" ] || [ "$1" = "elevate_for_update.exe" ]; then
		echo " windows64"
	elif [ "$1" = "cocaine_diesel.x86_64" ] || [ "$1" = "launch" ]; then
		echo " linux64"
	fi
}

# create manifest
find * -type f -print0 | while IFS= read -r -d '' f; do
	digest=$(../utils/b2sum/b2sum < "$f")
	size=$(stat -c "%s" "$f")
	platform=$(file_platform "$f")
	cp "$f" "release/$digest"
	echo "$f $digest $size$platform" >> "release/manifest.txt"
done

# sign manifest
../utils/genkeys/sign "release/manifest.txt" | cat - "release/manifest.txt" > "release/$version.txt"
rm release/manifest.txt
