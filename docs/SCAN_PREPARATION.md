# Scan preparation (0.3)

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

Geometry stays double precision internally and in v2 projects. STL exports use
32-bit coordinates. These tools require real-scan evaluation and do not establish
manufacturing or clinical suitability.
