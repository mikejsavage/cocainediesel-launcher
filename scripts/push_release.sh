#! /bin/sh

version=$(basename $(pwd))
cd release
rsync -v * mjs:/var/www/medfall
ssh mjs "echo \"$version\" > /var/www/medfall/version.txt"
