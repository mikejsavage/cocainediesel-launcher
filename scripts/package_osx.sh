#! /bin/sh

rm -r release/pkg
mkdir -p release/pkg/Applications/Medfall.app/Contents/MacOS
cp release/launch release/pkg/Applications/Medfall.app/Contents/MacOS

cat > release/pkg/Applications/Medfall.app/Contents/Info.plist << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>launch</string>
</dict>
</plist>
EOF

pkgbuild --root release/pkg --identifier com.medfall release/Medfall.pkg
