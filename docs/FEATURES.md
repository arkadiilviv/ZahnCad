# Capability status

| Capability | 0.3 source status | Evidence required next |
|---|---|---|
| ASCII/binary STL, scaling, binary export | Windows and Linux tests passed | Scanner corpus |
| Bounds, degeneracy and edge topology | Implemented | Larger scan corpus and performance |
| Qt/VTK viewer, clipping, measurement, screenshots | Windows/Linux builds and rendering verified | Full manual interaction tests |
| ZIP projects, SQLite recent cases, autosave/recovery | Background autosave, two snapshots and fallback | Broader crash/failure-injection cases |
| Undo/redo | Import/remove scans, visibility and opacity; shared immutable geometry | Commands for future editing tools |
| Background loading | Cancellable STL/project loading and display preparation | Real scanner corpus |
| Large-scan navigation | Display-only clustering during interaction, original restored afterwards | Presented-frame FPS on real dental scans |
| Windows installer / Linux AppImage | CI build and package checks configured | Clean desktop integration and distribution notices |
| PLY/OBJ | ASCII/binary PLY and OBJ geometry/RGB import | Scanner-specific variants, image textures |
| Scan preparation | Selection, crop, components, cleanup, winding, hole fill, smooth and simplify with reports | Real scanner corpus and precision evaluation |
| Edited projects | v2 preserves source/current mesh, RGB and operation summaries; reads v1 | Larger mixed-format projects |
| Registration, margins, insertion analysis and blockout | Three-landmark rigid fit, saved editable margins, sampled directional analysis and grid envelope | Representative-scan alignment, trace repeatability and resolution checks |
| Crowns and reviewed CC0 anatomy library | Pending | M3 and bench evaluation |
| Copings, bridges, inlays, veneers and fit checks | Pending | M4 |
| Manufacturing export | Unavailable | All mandatory geometry/material/accessibility checks |
| Professional use claims | Unavailable | Market, intended use and evidence review |

Research export only copies/scales imported surfaces. It never represents a
generated or validated restoration. CGAL, Eigen and OpenVDB integration begins
with the geometry milestones; they are not yet linked into the foundation.
