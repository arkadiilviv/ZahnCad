# ZahnCAD project format

A ZIP contains `manifest.json`, `sources/N.stl|ply|obj|zmesh`, and optional
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
the current mesh and operations are empty. Cases use the lowest version that
can represent their contents, starting at version 2. A saved margin requires
version 3. Its scan contains `marginMm`, an ordered array of 3–10,000 XYZ triples
in current-mesh coordinates. The closing segment connects the last point to the
first.

Version 4 adds optional `toothFdi`, a permanent tooth number (11–18, 21–28,
31–38 or 41–48), and generated sources. A generated crown uses `sourceFormat`
`zmesh`, with its original generated geometry stored in `ZMSH1` at scale 1.
Subsequent geometry edits use the same optional `current` entry as imported
scans. The crown operation records the preparation scan name, the insertion
axis and the requested height.

Version 5 adds an optional top-level `referenceMeasurement` record. It stores
`landmarks`, `method`, `notes`, `expectedMm`, `toleranceMm` and up to 200
`readings`. Each reading stores its scan name and role, source SHA-256,
scale-to-mm factor, measured distance in mm, reopen flag, application version,
platform and UTC capture time. Readers preserve compatibility with versions
1–4. The CSV derives signed errors, tolerance results and repeat spread from
these stored readings.

Version 6 adds `toothMargins` to a scan with multiple tooth assignments or
margins. Each entry has a unique `toothFdi` and an optional `marginMm` trace.
An absent trace on a numbered tooth means the tooth has been added but has no
saved margin. Tooth number `0` is reserved for an unassigned trace. The scan's
own `toothFdi` selects its active tooth. Versions 3–5 and single-tooth entries
still use the scan-level `marginMm`, which the reader associates with that
tooth or retains as unassigned.

Version 7 stores optional per-scan `insertionAxes` entries as arrays of FDI tooth
number and unit direction components (x, y, z) in case coordinates.
