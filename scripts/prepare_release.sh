#! /bin/sh

wget https://ci.appveyor.com/api/projects/mikejsavage/forksow/artifacts/cocaine_diesel_windows.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-g1fxg/artifacts/cocaine_diesel_linux.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-launcher/artifacts/launcher_windows.zip &
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-launcher-jbg0q/artifacts/launcher_linux.zip &

version=$(basename $(pwd))
wget -O base.zip https://github.com/mikejsavage/forksow/archive/v$version.zip &

wait

7z x cocaine_diesel_windows.zip
7z x cocaine_diesel_linux.zip
7z x launcher_windows.zip
7z x launcher_linux.zip

7z x base.zip forksow-$version/base
mv forksow-$version/base .
rmdir forksow-$version

rm cocaine_diesel_windows.zip
rm cocaine_diesel_linux.zip
rm launcher_windows.zip
rm launcher_linux.zip
rm base.zip

cp ../installer/*.txt .
rm base/*.md base/.gitignore
rm *.exp *.lib
