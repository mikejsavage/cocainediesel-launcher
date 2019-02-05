#! /bin/sh

version=$(basename $(pwd))
cd release

rsync -v * mjs:/var/www/cocainediesel
ssh mjs "echo \"$version\" > /var/www/cocainediesel/version.txt"

aws s3 sync . s3://cocaine-diesel-cdn
echo "$version" | aws s3 cp - s3://cocaine-diesel-cdn/version.txt
