#! /bin/sh

wget https://ci.appveyor.com/api/projects/mikejsavage/forksow/artifacts/cocaine_diesel_windows.zip
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-g1fxg/artifacts/cocaine_diesel_linux.zip
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-g1fxg/artifacts/cocaine_diesel_base.zip
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-launcher/artifacts/launcher_windows.zip
wget https://ci.appveyor.com/api/projects/mikejsavage/forksow-launcher-jbg0q/artifacts/launcher_linux.zip

7z x cocaine_diesel_windows.zip
7z x cocaine_diesel_linux.zip
7z x -obase cocaine_diesel_base.zip
7z x launcher_windows.zip
7z x launcher_linux.zip

rm cocaine_diesel_windows.zip
rm cocaine_diesel_linux.zip
rm cocaine_diesel_base.zip
rm launcher_windows.zip
rm launcher_linux.zip

cp ../installer/*.txt .
rm base/*.md base/.gitignore
rm -r base/models/mapobjects base/models/objects/jumppad.md3
rm *.exp *.lib
