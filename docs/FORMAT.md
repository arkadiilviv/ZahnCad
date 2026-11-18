# ZahnCAD project format

A ZIP contains `manifest.json`, `sources/N.stl|ply|obj`, and optional
`meshes/N.zmesh`. Paths are archive keys and are never extracted to disk.
Archive bytes and expanded data are limited to 1 GiB, entries to 512 MiB and
entry count to 128. Writers limit cases to 63 scans to allow source/current
entries plus the manifest. Duplicate entries, corrupt ZIP data, missing geometry
and unsupported versions are rejected. Saves use QSaveFile's atomic commit.

The manifest contains `format: "ZahnCAD"`, integer `version`, `caseId`,
`units: "mm"` and `scans`. Each scan contains `name`, `role`, `source`,
`sourceFormat`, `scaleToMm`, `visible`, `opacity`, `operations`, and optional
`current`. Source bytes remain unchanged. Source geometry is parsed and scaled
into mm; a current mesh already uses mm and must not be scaled again. The SQLite
case index is not needed to open a portable project.

`ZMSH1`: five ASCII magic bytes, uint64 triangle count and uint8 color flag,
little-endian. Each triangle has nine IEEE-754 doubles (xyz for three corners),
then nine uint8 RGB values when colored. Exact size: 14 + count × (72 or 81)
bytes. Colors are per corner. Invalid sizes/coordinates fail loading.

Operation summaries record the parameters and results relevant to each edit.
Registration records a row-major 4 × 4 transform and landmark residuals.
Blockout records its insertion axis and grid spacing. The current mesh stores
the edited result. Undo history and temporary selections are session state.

Version 1 remains readable. Source format defaults to STL, the scaled source is
the current mesh and operations are empty. Saving cases without margins writes
version 2. A saved margin requires version 3. Its scan contains `marginMm`, an
ordered array of 3–10,000 XYZ triples in current-mesh coordinates. The closing
segment connects the last point to the first.
