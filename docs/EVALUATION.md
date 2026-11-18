# M0 research evaluation

ZahnCAD 0.3.1 is a local desktop research prototype for viewing, measuring,
and preparing copies of dental surface scans. Proposed evaluation outputs are
software observations, screenshots of synthetic fixtures, and geometry
measurements. The current build does not make diagnoses, establish fit, or
generate validated restorations for treatment or manufacturing.

## Workflow review

Ask a dental technician or dental CAD workflow reviewer to walk through a
representative case and record the date, reviewer role, version, and outcome.

1. Identify which scan exports, units, jaw roles, and opposing or bite records
   are available. Record the expected way to confirm orientation and scale.
2. Import and inspect the case, isolate a region, measure a known reference,
   save, and reopen. Record confusing steps, missing workflow inputs, and any
   unexpected change in geometry or display.
3. Identify which tasks are suitable for the research prototype. Record which
   downstream decisions require separate tools or evidence and record proposed
   tolerances with their rationale before evaluating representative cases.

## Corpus

| Fixture | Coverage | Expected observation |
|---|---|---|
| `examples/triangle.stl` | Basic import, view and measurement | One face, 10 × 10 × 0 mm bounds, and two 10 mm edges at scale 1 |
| `examples/colored-scan.ply` | Vertex RGB, clipping and edited case persistence | Crop preserves color and leaves the original available after reopening |
| `examples/brush-test.ply` | Brush visibility and navigation | A stroke on the front surface leaves hidden vertices unselected |
| Generated curved stress STL | Import and display preparation throughput | Full source triangle count remains available after navigating with LOD |
| Representative scanner exports | Units, formats, multiple scans, real geometry and performance | Pending authorized fixtures and reviewer expectations |

Prefer public, license-compatible or approved de-identified fixtures. Store
representative case files outside the repository. Keep an inventory with a case
ID, provenance and permission, scanner/export settings, format, units, files
and roles, triangle counts, and expected dimensions or defects. Record measured
values and deviations against the references. Keep file paths and identifiers
out of shared logs and screenshots when they identify a person or case.

## Reproduce the synthetic baseline

Use a desktop build with `BUILD_TESTING=ON`, Python 3, and a working display.
On Windows, put the matching UCRT64 `bin` directory on PATH. Run from the
repository root with a new output directory for each run.

```powershell
python scripts/evaluate_m0.py build-desktop out/m0-evidence
```

The runner checks the triangle's topology and scaled binary STL roundtrip,
benchmarks all four synthetic fixtures, and runs the desktop smoke checks.
It saves synthetic screenshots and reopened edited cases in the output
directory. The generated stress STL occupies approximately 250 MB during
the run.

## Geometry feasibility

### Synthetic baseline, 2026-11-18

Ran ZahnCAD 0.3.1 on Windows x64 with scale 1. The benchmark ran once per
fixture and measures import and CPU display preparation.

| Fixture | Imported triangles | Navigation triangles | Import (ms) | Display preparation (ms) |
|---|---:|---:|---:|---:|
| `triangle.stl` | 1 | 1 | 4.22 | 1.01 |
| `colored-scan.ply` | 2 | 2 | 5.63 | 0.59 |
| `brush-test.ply` | 16 | 16 | 5.23 | 0.54 |
| Generated curved stress STL, 2,500 × 1,000 cells | 5,000,000 | 27,504 | 588.79 | 533.08 |

`zahncad-cli inspect` reported the expected 10 × 10 × 0 mm bounds,
three boundary edges, and zero degenerate, non-manifold, or inconsistently
wound edges on the triangle. Converted it with scale factor 2.54 and reopened
the 134-byte binary STL. The bounds became 25.4 × 25.4 × 0 mm and the face
and edge counts matched the original. These dimensions match the reference to
the precision printed by the CLI.

All four desktop smoke runs passed. The stress scan restored all 5,000,000
triangles after the navigation render exercise. The colored crop reopened with
its edited colors, the original scan and its colors, and the recorded operation.
The brush left the hidden back layer unselected. Local artifacts are in
`out/m0-verified-2026-11-18/` and the run log is
`out/m0-verified-2026-11-18.log`.

Prior runs documented exact box crop, color retention after reopening an edited
case, and an analytical 2 mm offset for sampled bidirectional deviation.
Sampled deviation has no certified error bound. The synthetic checks establish
geometric feasibility for these fixtures. Manual ruler placement, scanner
accuracy, representative geometry, and presented frame rates remain open.
[Verification record](VERIFICATION.md)

## First representative case

Prefer a scan of a physical training model or typodont. Start with an upper or
lower arch, its scanner/export settings and units, and one independently
measured reference distance with identifiable endpoints. An opposing arch and
bite record are useful for evaluating case roles if available.

| Record | Capture |
|---|---|
| Case provenance | Case ID, permission or license, scanner and export settings, source units, roles, format and triangle counts per file |
| Measured reference | Endpoint descriptions, independent reference distance, measurement method, and tolerance with rationale agreed before the run |
| ZahnCAD result | Build and OS, import scale, observed orientation, initial and reopened measurements, signed differences from the reference, and any visible or persistence issues |
| Reviewer assessment | Date, reviewer role, workflow findings, proposed intended-use changes, and unresolved issues |

### Case walkthrough

1. Use `triangle.stl` as a control. Import at scale 1 as Reference, choose
   the occlusal view with F3, and place the ruler with M on each of the two
   perpendicular edges. Record the readings against 10 mm and any ambiguity
   in placing the endpoints.
2. Start a fresh case with the approved case code and import each scan using
   its confirmed scale and role. Compare the standard views and the relative
   placement of the scans to the expected orientation. Inspect the geometry
   of each scan and record bounds and any unexpected topology findings.
3. Toggle visibility to isolate the scan of interest. Turn on the ruler with
   M and measure the agreed endpoints. Record the reading, subtract the
   independent reference, and compare the absolute difference with the agreed
   tolerance. Record whether endpoint placement could be repeated.
4. Use X to inspect a region with the crop box. If testing a crop, apply it
   to the working copy and retain both reference landmarks. Save, reopen, and
   confirm the listed roles, visible geometry, and original scan availability.
   Place a fresh ruler at the reference endpoints and record the result.

## Intended-use assessment

The synthetic evidence supports continued evaluation of local scan viewing,
geometry inspection, and reversible preparation of research copies. Distance
measurement on representative scans remains a candidate research workflow
pending repeatable landmark placement and comparison against an independent
reference. Diagnosis, fit assessment, and restoration outputs for treatment
or manufacturing require separate evidence and tools. Record any changes to
this assessment after the dental workflow review.

M0 closes when the workflow review, authorized corpus, geometry feasibility
findings, and intended-use assessment are recorded. The synthetic baseline is
recorded. Reviewer input, an authorized representative scan with an independent
reference, and the resulting measurements remain open.

The same synthetic fixtures support the [M1 viewer and packaging checks](M1_ACCEPTANCE.md).
