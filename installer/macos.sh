#! /bin/sh

APP="Cocaine Diesel.app"

mkdir -p "dmg/$APP/Contents/MacOS"
mkdir -p "dmg/$APP/Contents/Resources"
cp ../release/cocainediesel "dmg/$APP/Contents/MacOS/Cocaine Diesel"
# /usr/libexec/PlistBuddy "$APP/Contents/Info.plist" -c "add CFBundleDisplayName string \"${APP}\""
# /usr/libexec/PlistBuddy "$APP/Contents/version.plist" -c "add ProjectName string \"$APP\""

ln -s /Applications dmg/Applications

hdiutil create -volname "Cocaine Diesel" -srcfolder "dmg" -ov -format UDZO CocaineDiesel.dmg
