# ZahnCAD project format v2

A ZIP contains `manifest.json`, `sources/N.stl|ply|obj`, and optional
`meshes/N.zmesh`. Paths are archive keys and are never extracted to disk.
Archive bytes and expanded data are limited to 1 GiB, entries to 512 MiB and
entry count to 128. Writers limit cases to 63 scans to allow source/current
entries plus the manifest. Duplicate entries, corrupt ZIP data, missing geometry
and unsupported versions are rejected. Saves use QSaveFile's atomic commit.

The manifest contains `format: "ZahnCAD"`, integer `version: 2`, `caseId`,
`units: "mm"` and `scans`. Each scan contains `name`, `role`, `source`,
`sourceFormat`, `scaleToMm`, `visible`, `opacity`, `operations`, and optional
`current`. Source bytes remain unchanged. Source geometry is parsed and scaled
into mm; a current mesh already uses mm and must not be scaled again. The SQLite
case index is not needed to open a portable project.

`ZMSH1`: five ASCII magic bytes, uint64 triangle count and uint8 color flag,
little-endian. Each triangle has nine IEEE-754 doubles (xyz for three corners),
then nine uint8 RGB values when colored. Exact size: 14 + count × (72 or 81)
bytes. Colors are per corner. Invalid sizes/coordinates fail loading.

Operations record name, UTC time, parameters, selection count and sampled
deviation; crop also records its box. This is an audit summary, not a replay
log. Selected indices are not stored; the current mesh is the edited result.
Undo history and temporary selections are session state.

Version 1 remains readable: source format defaults to STL, the scaled source is
the current mesh and operations are empty. Saving writes v2. Older builds cannot
read v2; retain a separate copy for old builds. Margins, restorations and material
validation need future schemas.
