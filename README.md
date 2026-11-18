# ZahnCAD

Free, local-first dental CAD research project for Windows and Linux.
License: GPL-3.0-or-later. Version 0.3.1 adds camera/tool hotkeys and vertex brushing.

See [capability status](docs/FEATURES.md), [development milestones](docs/ROADMAP.md)
and [verification record](docs/VERIFICATION.md). Behavior and remaining limits are
documented in [viewer reliability](docs/RELIABILITY.md).
See [M0 research evaluation](docs/EVALUATION.md) and
[M1 viewer and packaging acceptance](docs/M1_ACCEPTANCE.md) for work in progress.

## Implemented

- Dependency-free C++20 core: strict binary/ASCII STL loading, positive scaling,
  binary export, bounds, degeneracy, boundary, manifold-edge and winding checks.
- Headless `zahncad-cli inspect scan.stl` and
  `zahncad-cli convert scan.stl scaled.stl 25.4`.
- Optional Qt6/VTK desktop: multiple scans, role and scale confirmation,
  visibility, opacity, standard views, projection, distance measurement,
  clipping and PNG screenshots.
- ZIP `.zahncad` projects with versioned JSON, original STL bytes, scale and
  display settings; atomic saves, autosave recovery and a SQLite recent-case index.
- Undo/redo for scan import/removal, visibility and opacity; history shares
  immutable geometry and does not duplicate meshes for display changes.
- Cancellable background scan/project loading, geometry preparation and diagnostics.
- Display-only navigation LOD for scans above 200,000 triangles; full-resolution
  data returns after navigation and is used for diagnostics and export.
- Nonblocking autosave, two recovery snapshots with fallback, and a session lock
  that prevents concurrent instances from overwriting recovery files.
- PLY (ASCII and binary) and OBJ polygon import with vertex RGB and double
  coordinates. Original file bytes remain unchanged.
- Face/box selection, exact axis-aligned cropping, component isolation, duplicate
  and degenerate face removal, winding correction, limited hole filling,
  fixed-boundary smoothing and topology-preserving simplification.
- Candidate previews and before/after reports with sampled bidirectional deviation;
  geometry edits are undoable. Projects retain source and edited meshes.
- Three-landmark rigid registration, editable saved margin traces, sampled
  insertion analysis and directional blockout.

See [scan-preparation instructions and limits](docs/SCAN_PREPARATION.md).
See [keyboard and mouse controls](docs/CONTROLS.md), also available with F12.

## Build the core

Requires CMake >=3.24 and a C++20 compiler.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Build the desktop

On Windows, the workspace-local MSYS2 UCRT64 toolchain can be used with:

```powershell
.\scripts\build-windows.ps1
python scripts/package_windows.py --prefix build/toolchain/msys64/ucrt64
.\out\ZahnCAD\zahncad.exe
```

The packaging script copies the executable, Qt platform/SQLite plugins and the
recursive MinGW DLL dependencies into `out/ZahnCAD`. Keep the entire folder
together when moving or distributing it.

On Ubuntu 26.04, install `g++ cmake ninja-build qt6-base-dev libqt6sql6-sqlite
libvtk9-qt-dev libarchive-dev`, then use the desktop CMake commands below.
Ubuntu 24.04's packaged VTK uses Qt5 and does not match this application's Qt6
interface. Linux binary archives target Ubuntu 26.04 and list their runtime
dependencies in the included README.

Install Qt6 Widgets/Sql with the SQLite driver, libarchive, and VTK built with
`VTK_QT_VERSION=6` and `VTK_MODULE_ENABLE_VTK_GUISupportQt=YES`.
Set `CMAKE_PREFIX_PATH` to these installations, then:

```sh
cmake -S . -B build-desktop -DZAHNCAD_DESKTOP=ON
cmake --build build-desktop --config Release
cmake --install build-desktop --config Release --prefix out
```

Alternatively use the supplied vcpkg manifest and its CMake toolchain. It is a
dependency inventory, not yet a reproducibly pinned toolchain. Desktop builds
must use matching Qt major versions throughout. Deploy Qt's platform and SQL
plugins and VTK/libarchive runtime libraries when distributing a build.

## Current limits

The [M1 packaging instructions](docs/M1_ACCEPTANCE.md) cover the unsigned
Windows x64 installer and Ubuntu 26.04 x86-64 AppImage. The separate Linux
binary archive requires matching system runtime packages.

The importer caps input at 10 million triangles; inspection uses exact shared
coordinates and is memory-intensive. Research STL export does not imply
manufacturability. Self-intersections, thickness, fit and clinical suitability
are not checked. No restoration generation or tooth library is supplied yet.
General self-intersection/non-manifold repair and professional
release gates remain planned. OBJ/PLY image textures and point clouds are not supported.
Manual save remains synchronous; undo history is session-local and is not stored
in project files. Patient data stays local; the program has no network services.

## Reliability and performance checks

The desktop build includes `project_tests` and `viewer_tests`. For a reproducible
synthetic throughput test:

```sh
python scripts/generate_stress_stl.py build-desktop/curved-stress.stl
build-desktop/viewer_benchmark build-desktop/curved-stress.stl
```

`zahncad --smoke-test input.stl screenshot.png` exercises cancellation, async
import, undo/redo, scan removal/restore, navigation LOD and clean shutdown.
With `examples/triangle.stl` it also checks surface measurement and clipping,
then uses Recent Cases to reopen a saved two-scan project. It writes
`screenshot.png.ruler.png` and `screenshot.png.zahncad` for that fixture.
On Linux run it with `xvfb-run -a` if no display is available. The synthetic
fixture is a curved plate, not dental anatomy; real scan performance still
requires representative cases and workstation-specific evaluation.

`zahncad --stage2-smoke-test examples/colored-scan.ply screenshot.png` exercises
actual face picking, crop-box bounds, async cropping, undo/redo, color rendering
and edited-project persistence. It writes `screenshot.png.zahncad` as a synthetic
interoperability fixture. Automated preparation is enabled only by this explicit
test argument; interactive operations always offer Apply/Cancel.

See [development roadmap](docs/ROADMAP.md) and [project format](docs/FORMAT.md).
