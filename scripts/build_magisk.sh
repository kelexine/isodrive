#!/bin/sh

# Usage: ./build_magisk.sh <binary_path> <output_zip> <version_code> <version_name>

BINARY_PATH=$1
OUTPUT_ZIP=$2
VERSION_CODE=${3:-1}
VERSION_NAME=${4:-"v1.0"}
BUILD_DIR="build/magisk_gen"

echo "Building Magisk module..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/system/bin"
mkdir -p "$BUILD_DIR/META-INF/com/google/android"

# Copy binary
if [ -f "$BINARY_PATH" ]; then
    cp "$BINARY_PATH" "$BUILD_DIR/system/bin/isodrive"
else
    echo "Error: Binary not found at $BINARY_PATH"
    exit 1
fi

# module.prop
cat <<EOF > "$BUILD_DIR/module.prop"
id=isodrive
name=ISODrive
version=$VERSION_NAME
versionCode=$VERSION_CODE
author=kelexine
description=Mount ISOs as USB drives on Android.
EOF

# updater-script
echo "#MAGISK" > "$BUILD_DIR/META-INF/com/google/android/updater-script"

# update-binary
cat <<'EOF' > "$BUILD_DIR/META-INF/com/google/android/update-binary"
#!/sbin/sh
OUTFD=$2
ZIPFILE=$3
ui_print() { echo "ui_print $1" > /proc/self/fd/$OUTFD; echo "ui_print" > /proc/self/fd/$OUTFD; }
ui_print "Installing ISODrive..."
unzip -o "$ZIPFILE" "system/*" "module.prop" -d $MODPATH >&2
set_perm $MODPATH/system/bin/isodrive 0 0 0755
ui_print "Done!"
EOF

chmod +x "$BUILD_DIR/META-INF/com/google/android/update-binary"

# Resolve absolute path for output zip
case "$OUTPUT_ZIP" in
    /*) ZIP_DEST="$OUTPUT_ZIP" ;;
    *) ZIP_DEST="$(pwd)/$OUTPUT_ZIP" ;;
esac

# Zip it
cd "$BUILD_DIR" && zip -q -r "$ZIP_DEST" .

echo "Magisk module created: $ZIP_DEST"