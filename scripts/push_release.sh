#! /bin/sh

version=$(basename $(pwd))
cd release
rsync -v * mjs:/var/www/cocainediesel
ssh mjs "echo \"$version\" > /var/www/cocainediesel/version.txt"
