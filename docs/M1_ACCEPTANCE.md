# M1 viewer and packaging acceptance

Use the [controls reference](CONTROLS.md) and [evaluation fixtures](EVALUATION.md).
For each platform record the package type and checksum, build revision, OS, GPU
and driver, fixture IDs, result, and any reproduction steps. Use a clean account
or VM for install and recovery checks.

## Manual viewer checks

| Action | Expected result |
|---|---|
| Launch the package and open F12 | Main window and controls appear without the build toolchain on PATH |
| Import multiple scans, choosing role and scale | Scan list, bounds and reference distance reflect the selected roles and units |
| Import a corrupt file, then cancel a large import | Error or cancellation preserves the open case and the UI remains usable |
| Exercise standard views, orbit, pan, projection, and zoom | View changes follow the selected control |
| Toggle scan visibility, adjust opacity, remove a scan, and undo/redo | Listed scans and the rendering reflect each edit and its reversal |
| Measure a known edge, clip through a scan, and save a PNG | Measurement matches the fixture within recorded precision; clipping and screenshot match the view |
| Save, relaunch, and reopen from Recent Cases | Geometry, role, scale, visibility and opacity survive; the SQLite recent entry is available |
| Wait for autosave on a disposable case, force close, then relaunch | The current snapshot can be recovered and saved under a new path |
| Repeat with two snapshots and damage the newer copy in the disposable profile | The previous snapshot is offered and opens successfully |
| Uninstall Windows setup after saving a case outside the install directory | Shortcut and installed binaries are removed; the saved case remains |

Run the smoke commands from the README as automated evidence. The triangle
smoke checks a 5 mm span on the scan surface, rejects background and clipped
picks, and saves a two-scan case. It reopens the case through the SQLite recent
entry and checks the saved roles, scale, visibility and opacity.

## Scanner performance

For each representative file in millimetres, run `viewer_benchmark <scan>`
from the desktop build three times, noting whether the file cache was warm.
Record triangle counts, import seconds, display preparation seconds, and
navigation triangle counts. Record UI import-to-visible time separately,
including the first render. For other source units, use the UI with the
confirmed scale and record that scale with the result.

For scans above 200,000 triangles, capture presented FPS during a repeatable
10-second orbit on the target desktop using a platform frame capture tool.
Record the tool, viewport size, frame count, measurement duration, and GPU.
Compare against the project's 30 FPS target. Check that full detail returns
after releasing the mouse and that a reference measurement remains consistent.
Record brush latency separately if evaluated. The command-line benchmark
measures CPU preparation and does not measure presented FPS.

## Packaging checks

### Windows x64 installer

Build with `scripts/build-windows.ps1` and install [Inno Setup](https://jrsoftware.org/isdl.php)
6.3 or newer on the packaging machine. From the repo root, use a fresh staging
directory:

```powershell
python scripts/package_windows.py --prefix build/toolchain/msys64/ucrt64 --output out/ZahnCAD-installer-payload
iscc scripts/zahncad.iss
```

Pass `/DPayloadDir=<absolute path>` to `iscc` when using a different staging
directory.

The installer is written to `out/`. Install as a standard user on a clean
Windows machine, launch from the Start menu, and run the manual package checks.
The local build recipe produces an unsigned installer.

### Linux x86-64 AppImage

Use the Ubuntu 26.04 environment that built the Qt6/VTK desktop binaries. Put
the official [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy/releases)
and [Qt plugin](https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases)
AppImages beside each other and make both executable. Install `qmake6` from the
build environment's matching Qt6 packages. From the repo root:

```sh
QMAKE="$(command -v qmake6)" sh scripts/package_appimage.sh build-desktop /absolute/path/linuxdeploy-x86_64.AppImage
```

Copy the resulting `out/ZahnCAD-Linux-x64.AppImage` to a clean Ubuntu 26.04
desktop without Qt/VTK development packages. Launch with
`./ZahnCAD-Linux-x64.AppImage`, or use `--appimage-extract-and-run` where FUSE
is unavailable. The AppImage uses an X11 display or XWayland and the host's
graphics and font stack, including `libopengl0` and `libharfbuzz0b`. Check
rendering and Recent Cases. Record the tool versions and runtime requirements.
Prepare the dependency notices and source information before distributing
either package.

## Local verification, 2026-11-18

The packages were built from the M1 working tree based on revision `296d925`.

- Windows x64, build 26300, AMD Radeon RX 9070 driver 32.0.32015.16001.
  Inno Setup 6.7.4 produced `ZahnCAD-0.3.1-Windows-x64-Setup.exe` with
  SHA-256 `da359753ad71d82ccdb3dd39e9ec2452526b95277c4c5f9eef22ab57c97e81d7`.
  All four CTest suites and installed GUI smoke runs for `triangle.stl`,
  `colored-scan.ply` and `brush-test.ply` passed with a Windows-only PATH.
  The Start menu shortcut pointed to the installed app. Uninstall removed the
  shortcut and binaries and preserved the saved case outside the install.
  The installed ruler displayed 5.000 mm for the 5 mm synthetic span.
  A separate packaged smoke run restored the full five-million-triangle
  synthetic mesh after navigation. Screenshots and cases are in `out/m1-windows/`.
- Ubuntu 26.04 x86-64 in Docker, Xvfb with software Mesa 26.0.8.
  linuxdeploy `07333c6` and its Qt plugin `9b9fca1` produced
  `ZahnCAD-Linux-x64.AppImage` with SHA-256
  `75ebaba420d3da5fd07492e85e897feb0fe9b9cd4a0972164e96ce90e2e25369`.
  All four CTest suites passed. The AppImage passed the triangle, colored
  crop and brush smoke checks in a separate Ubuntu container without Qt or
  VTK packages installed. The packaged ruler displayed 4.987 mm for the 5 mm
  synthetic span. Logs, screenshots and cases are in `out/m1-linux/`.

Remaining acceptance includes the full manual walkthrough on clean desktop
accounts and representative scanner measurements with presented-frame FPS.
The synthetic ruler checks allow 0.05 mm for screen-pixel placement error.

### Forced-close recovery regression

Run `python scripts/verify_recovery.py build-desktop/zahncad.exe out/recovery`
with the desktop toolchain on PATH on Windows. On Linux use the corresponding
desktop executable under a display or `xvfb-run`. It creates two disposable
autosaves through the desktop worker, force-closes the process, and relaunches
through the recovery load path. It verifies that Save As preserves the latest
snapshot and that a damaged latest snapshot falls back to the previous one.
The interactive startup confirmation is part of the manual walkthrough.
