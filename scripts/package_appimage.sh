#!/bin/sh
# Run in the Linux environment that built the desktop binaries.
set -eu

build=$(cd "${1:?Pass the Linux desktop build directory}" && pwd)
deploy=${2:?Pass the absolute path to linuxdeploy-x86_64.AppImage}
case "$deploy" in
  /*) test -x "$deploy" ;;
  *) echo 'linuxdeploy must be an absolute path' >&2; exit 1 ;;
esac

repo=$(cd "$(dirname "$0")/.." && pwd)
mkdir -p "$repo/out"
stage=$(mktemp -d "$repo/out/appimage.XXXXXX")
trap 'rm -rf -- "$stage"' EXIT
appdir=$stage/ZahnCAD.AppDir
install -Dm644 "$repo/LICENSE" "$appdir/usr/share/doc/zahncad/LICENSE"
install -Dm644 "$repo/NOTICE" "$appdir/usr/share/doc/zahncad/NOTICE"

cd "$stage"
APPIMAGE_EXTRACT_AND_RUN=1 "$deploy" \
  --appdir "$appdir" \
  --executable "$build/zahncad" \
  --executable "$build/zahncad-cli" \
  --desktop-file "$repo/scripts/zahncad.desktop" \
  --icon-file "$repo/scripts/zahncad.svg" \
  --plugin qt

for plugin in libqxcb.so libqsqlite.so; do
  if [ -z "$(find "$appdir/usr" -name "$plugin" -print -quit)" ]; then
    printf 'Missing Qt plugin: %s\n' "$plugin" >&2
    exit 1
  fi
done

APPIMAGE_EXTRACT_AND_RUN=1 OUTPUT=ZahnCAD-Linux-x64.AppImage "$deploy" \
  --appdir "$appdir" --output appimage
install -m755 ZahnCAD-Linux-x64.AppImage "$repo/out/ZahnCAD-Linux-x64.AppImage"
