#! /usr/bin/env bash

set -e

version="$(basename "$(pwd)")"
find . -maxdepth 1 -type f -exec chmod 644 {} \;
mkdir release

../scripts/lua.linux ../scripts/make_manifest.lua > release/manifest.txt
../sign release/manifest.txt > "release/$version.txt"
rm release/manifest.txt
