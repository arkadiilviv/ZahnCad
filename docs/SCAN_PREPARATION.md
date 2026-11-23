# Scan preparation

1. Use **Case → Import scan** for STL, PLY or OBJ. Confirm the scale to mm and
   role. Select a scan in the left panel before editing.
2. **View → Toggle scan colors** and **Toggle face normals** help inspect it.
   Cyan segments show about 2,000 sampled normals, each 0.5 mm long.
3. Press **B** for the vertex brush. Left-drag paints visible vertices; Shift+left
   erases. Orange points mark selected vertices. Change radius with **[ / ]** or
   the toolbar. Right-drag rotates, Shift+right pans, and the wheel zooms. Left
   and middle drags cannot move the camera in brush mode. **Escape** exits it.
   Keep retains triangles whose three vertices are selected; Delete removes all
   triangles incident to any selected vertex. Component isolation uses the first
   touched triangle in mesh order; brush one component at a time.
4. Enable the crop/selection box and move/resize its handles. Rotation is disabled:
   it is axis-aligned in scan coordinates. Box selection selects vertices;
   **Crop to box** clips triangles and interpolates RGB at cut edges. Box and
   measurement widgets turn off the brush, since their left-drag gestures differ.
5. Choose a repair/refinement tool. A cancellable worker prepares a candidate,
   displayed during the change report. **Apply** commits one undoable edit;
   **Cancel** leaves the scan unchanged. Reports show topology changes and sampled
   max/RMS deviation. **Restore original scan** is undoable too.
6. Save the case to retain original bytes, current geometry and the audit summary.
   Research STL export uses current geometry; STL cannot retain colors. The source
   import file is not overwritten.

## Tool behavior and limitations

- Components connect through exactly coincident vertices; no gap-closing tolerance.
- Cleanup removes duplicate/zero-area faces and welds exact points. It does not
  repair arbitrary self-intersections or non-manifold arrangements. Colors at
  welded seams can be consolidated.
- Consistent winding aligns connected faces; it does not prove outward orientation.
- Hole filling uses an approximate maximum circumscribing radius, not diameter.
  It can fill an outer boundary or create overlap; review the preview. It does
  not reconstruct missing anatomy.
- Smoothing uses relaxation 0.1, 1–100 iterations, and fixed boundary vertices.
  It can shrink detail and is not margin-aware.
- Simplification requests 1–95% reduction while preserving topology and boundary
  vertices. Constraints can prevent the requested reduction; check actual counts.
- Smoothing/simplification/filling reject degenerate faces, non-manifold edges
  and inconsistent winding. Vertex-only non-manifold defects are not fully checked.
- Refinement's deviation threshold is **sampled**. Samples include vertices and
  centroids on sampled triangles, about 100,000 per direction. Reports compare
  both directions. This is not a Hausdorff bound; narrow defects can be missed.
  Cropping/components intentionally remove surfaces, so deviation may be large.

PLY supports ASCII and little/big-endian binary properties, double coordinates,
vertex RGB (integer 0–255 or float 0–1), and polygon faces. OBJ supports polygons,
relative indices, homogeneous vertices and xyz RGB (0–1). Polygons are triangulated.
OBJ MTL/image textures, PLY face textures, point clouds and external resources
are not supported. Directions follow triangle winding, not imported normals.

Geometry stays double precision internally and in saved projects. STL exports use
32-bit coordinates. These tools require real-scan evaluation and do not establish
manufacturing or clinical suitability.

## Registration, margins, and insertion

- Select the moving scan and choose **Scan → Register using landmarks**.
  Choose the fixed scan. Pick three well-separated, non-collinear matching
  landmarks on each scan in the same order. Review the preview and landmark
  residuals before applying. The transform preserves scale and vertex colors
  and moves all saved margins with the scan. Inspect alignment away from the
  three landmarks.
- Choose **Design → Trace or edit margin** on a visible scan.
  Click the scan to trace points in order. When editing a saved margin, click
  near a segment to insert a point between its ends. Shift+click near a point
  to move it. Use **Undo** (Ctrl+Z) and **Redo** (Ctrl+Y or Ctrl+Shift+Z) for
  point edits, or press Escape to discard the draft. **Apply margin** commits
  the whole draft as one case edit and closes the polyline.
  Use close spacing on curved areas. Geometry-changing preparation clears the
  saved margins on that scan, apart from rigid registration. Undo restores them.
- Select a tooth, orient the scan, and choose **Design → Set insertion direction
  from view**. **Analyze → Check insertion undercuts** and crown generation use
  that tooth’s saved direction after orbiting or reopening the case. Repeat the
  command to replace it, or use Undo. Teeth without a saved direction use the current view. Orbit to see sampled shadowed triangles in red.
  The report checks up to 100,000 face centroids along parallel rays and gives
  the largest sampled axial depth. Narrow undercuts can fall between samples.
- **Block out from view** uses the current camera axis and a 0.05–5 mm grid.
  Each sampled column extends from its first surface hit toward a plane one
  grid step below the lowest source point on that axis. Review the resulting
  closed envelope and its resolution before applying. The envelope adds a
  base and has its own geometry. The source remains available through undo
  or **Restore original scan**.

## M2 acceptance

Use the fixtures and provenance rules in [Evaluation](EVALUATION.md).
The synthetic suite verifies a known rigid rotation/translation, degenerate
landmark rejection, a 1 mm directional undercut, blockout topology and bounds,
and margin undo, persistence and recovery. The desktop smoke can be run with
`zahncad --m2-smoke-test examples/colored-scan.ply margin.png`. It exercises
surface picking, point editing, registration, undo/redo, Save As, and
blockout. Use a disposable output directory.

For representative scans, record the selected region and expected tolerances
before testing. Compare registration against independent landmarks excluded
from the fit. Repeat the margin trace and record the observed displacement.
Compare insertion results with an independently identified undercut, and rerun
blockout at two grid spacings to record changes in its bounds and surface.

## M3 tooth markup and research crowns

1. Select a visible preparation scan. Open **Design → Select tooth markup**
   and choose a permanent tooth from the FDI chart. The scene shows the chosen
   number next to the scan. Choose another tooth on the same scan to add or
   switch to it. Each tooth has its own margin. Click a row in **Saved margins**
   to switch directly. The active margin is gold and other saved margins on
   that scan are teal. **Clear saved margin** affects the active tooth.
2. Trace and apply a margin around the tooth. For this prototype, trace the
   points in order so the outline is convex when viewed along the insertion
   direction. Save the intended insertion direction before generating.
3. With the intended tooth active, choose **Design → Build research crown**,
   enter a height above the highest
   margin point and inspect the preview. **Apply** adds a separate restoration
   candidate. Changing the scan or margin afterward leaves that candidate as
   built. Generate a fresh candidate after adjusting the preparation.

For a filed-down tooth, choose **Design → Crown over prepared tooth** after
applying its convex margin and setting the insertion direction. Set a height
above the highest margin point and review the candidate. The tool samples the
first scan surface visible along the insertion axis inside the margin, joins
the resulting cavity to the traced rim, and closes the exterior with a flat
roof. Missing interior samples and sampled points reaching the roof stop the
build. Undercuts and details between samples can be missed. The prototype
includes no cement-space allowance or physical fit verification.

The margin-only prototype uses a tapered wall and a recessed underside with a flat roof.
The geometry has no tooth-specific anatomy. Fit, material thickness, occlusion
and manufacturing suitability remain outside this prototype's checks.

### Bench measurements

Import a candidate with the **Restoration candidate** role and confirm its units
and alignment with the case. Select it in the scene and use **Analyze → Measure
surface separation** to choose a second surface. The report gives the smallest
sampled unsigned distance in millimetres, the two endpoint coordinates in the
case coordinate system, and the number of queries across both meshes.

The tool checks vertices and centroids from up to 25,000 triangles per direction
against the full opposite surface, regardless of visibility, clipping or display
detail. Samples can miss a closer area or an intersection. Use the result as a
research measurement and compare it against an independent reference when
evaluating physical samples. The desktop smoke above also checks a synthetic
2 mm offset and verifies that the candidate role survives reopening the case.
