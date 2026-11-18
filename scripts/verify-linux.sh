#!/bin/sh
# Run under an existing Linux kernel. The root is isolated from host packages.
set -eu
workspace=${1:?Pass the Linux path to the ZahnCAD workspace}
root=${2:-}
if [ -n "$root" ]; then
  case "$root" in /run/zahncad-linux.*) test -d "$root" ;; *) exit 1 ;; esac
else
  root=$(mktemp -d /run/zahncad-linux.XXXXXX)
  tar -xzf "$workspace/build-linux/ubuntu-base.tar.gz" -C "$root"
fi
printf '%s\n' "$root" > "$workspace/build-linux/root-path.txt"
cp /etc/resolv.conf "$root/etc/resolv.conf"
mountpoint -q "$root/dev" || mount --bind /dev "$root/dev"
mountpoint -q "$root/proc" || mount -t proc proc "$root/proc"
mkdir -p "$workspace/build-linux/apt-cache"
mountpoint -q "$root/var/cache/apt/archives" || mount --bind "$workspace/build-linux/apt-cache" "$root/var/cache/apt/archives"
cleanup() {
  safe=true
  for target in "$root/var/cache/apt/archives" "$root/proc" "$root/dev"; do
    while mountpoint -q "$target"; do
      if ! umount "$target"; then safe=false; break; fi
    done
  done
  # Retain the isolated root on failure for a fast retry, without mounted host resources.
  if [ "$status" -eq 0 ] && [ "$safe" = true ]; then
    case "$root" in /run/zahncad-linux.*) rm -rf "$root" ;; esac
  fi
}
trap 'status=$?; cleanup' EXIT
chroot "$root" /bin/sh -c 'apt-get -o APT::Sandbox::User=root update && DEBIAN_FRONTEND=noninteractive apt-get -o APT::Sandbox::User=root install -y --no-install-recommends g++ cmake ninja-build qt6-base-dev libqt6sql6-sqlite libvtk9-qt-dev libarchive-dev xvfb xauth libgl1-mesa-dri'
mkdir -p "$root/work"
cp -R "$workspace/src" "$workspace/include" "$workspace/tests" "$workspace/examples" "$root/work/"
cp "$workspace/CMakeLists.txt" "$root/work/"
if [ -f "$workspace/build-desktop/stage2-color-crop.png.zahncad" ]; then
  cp "$workspace/build-desktop/stage2-color-crop.png.zahncad" "$root/work/windows-edited.zahncad"
fi
chroot "$root" /bin/sh -c 'cd /work && cmake -S . -B build -G Ninja -DZAHNCAD_DESKTOP=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel 4 && ctest --test-dir build --output-on-failure && QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a build/zahncad --smoke-test /work/examples/triangle.stl /work/build/smoke-linux.png && QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a build/zahncad --stage2-smoke-test /work/examples/colored-scan.ply /work/build/preparation-linux.png'
if [ -f "$root/work/windows-edited.zahncad" ]; then
  chroot "$root" /work/build/surface_tests /work/windows-edited.zahncad
fi
chroot "$root" /bin/sh -c 'cd /work && QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a build/zahncad --stage2-smoke-test /work/examples/brush-test.ply /work/build/brush-linux.png'
cp "$root/work/build/zahncad" "$root/work/build/zahncad-cli" "$root/work/build/smoke-linux.png" "$workspace/build-linux/"
cp "$root/work/build/preparation-linux.png" "$workspace/build-linux/"
cp "$root/work/build/brush-linux.png" "$root/work/build/brush-linux.png.brush.png" "$workspace/build-linux/"
chroot "$root" /bin/sh -c 'dpkg-query -W' > "$workspace/build-linux/DEPENDENCIES.txt"
echo "Linux desktop build, tests and rendering passed. Temporary build root: $root"
