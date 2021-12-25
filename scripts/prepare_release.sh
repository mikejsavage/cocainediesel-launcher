#! /usr/bin/env bash

set -e

wget https://ci.appveyor.com/api/projects/mikejsavage/cocainediesel-windows-tagged/artifacts/cocaine_diesel_windows.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/cocainediesel-linux-tagged/artifacts/cocaine_diesel_linux.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/cocainediesel-launcher-windows/artifacts/launcher_windows.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/cocainediesel-launcher-linux/artifacts/launcher_linux.zip &

version="$(basename "$(pwd)")"
wget -O base.zip "https://github.com/mikejsavage/cocainediesel/archive/v$version.zip" &

wait

7z x cocaine_diesel_windows.zip
7z x cocaine_diesel_linux.zip
7z x launcher_windows.zip
7z x launcher_linux.zip

7z x base.zip "cocainediesel-$version/base"
mv "cocainediesel-$version/base" .
rmdir "cocainediesel-$version"

rm cocaine_diesel_windows.zip
rm cocaine_diesel_linux.zip
rm launcher_windows.zip
rm launcher_linux.zip
rm base.zip

cp ../installer/*.txt .
rm base/*.md base/.gitignore
rm -- *.exp *.lib
