# Keyboard and mouse controls (0.3.1)

Menu entries display their shortcuts. **F12** opens the in-app controls list.
Shortcuts apply to the main window, not modal filename/case/parameter dialogs.

| Camera | Key |
|---|---|
| Fit all scans | F |
| Front / side / occlusal | F1 / F2 / F3 |
| Orthographic/perspective | F4 |
| Zoom in / out | F5 / F6 |
| Orbit in 15-degree steps | Alt + arrow keys |

| Selection and display | Key |
|---|---|
| Vertex brush on/off | B |
| Exit brush or discard a margin or registration draft | Escape |
| Smaller / larger radius | [ / ] |
| Clear vertex selection | Ctrl+Shift+A |
| Box widget / select vertices inside box | X / Shift+X |
| Keep selected vertices | Ctrl+Shift+K |
| Delete selected vertices and incident faces | Delete |
| Colors / normals | V / N |
| Measurement / axial clipping | M / C |
| Screenshot | Ctrl+Shift+P |

| Preparation | Key |
|---|---|
| Crop to box | Ctrl+Shift+C |
| Largest component / component touched by selection | Ctrl+Alt+L / Ctrl+Alt+I |
| Cleanup / winding | Ctrl+Alt+C / Ctrl+Alt+N |
| Fill holes / smooth / simplify | Ctrl+Alt+H / Ctrl+Alt+S / Ctrl+Alt+D |
| Restore original | Ctrl+Alt+R |

New/open/import/save/save-as use Ctrl+N/O/I/S/Shift+S. Undo: Ctrl+Z.
Redo: Ctrl+Y or Ctrl+Shift+Z.

Press **M**, then click two points on visible scan surfaces to measure the
straight-line distance in millimetres. A third click starts a new measurement.
Right-drag to orbit, Shift+right-drag to pan, and scroll to zoom while measuring.
The ruler uses full-resolution geometry and respects axial clipping.

## Brush gestures

- Left-drag adds vertices; Shift+left-drag removes them. Repeated painting is
  additive, not a toggle. Intermediate samples keep fast strokes continuous.
- Right-drag orbits; Shift+right pans; the wheel zooms. Left never rotates or
  pans while the brush is active. Middle drag is disabled in brush mode.
- Outside brush mode, left-drag uses ordinary trackball navigation; right-drag
  still orbits. Middle drag pans outside brush mode.
- The circle shows a radius in logical screen pixels, 5–200. A stroke must hit
  the active scan; it samples a surface neighborhood and uses depth visibility
  to avoid painting through the mesh. Exactly coincident corners are one selected
  geometric vertex. Vertex colors and original files are unaffected.
- Selection is temporary and clears when the active mesh changes. Activating
  the box or measurement widget exits brush mode; enabling the brush disables
  those widgets. Preparing acceleration structures runs in a cancellable job;
  cancellation may wait for a locator-build phase to finish.

For Keep, all three corners of a triangle must be selected. Delete removes every
triangle touching a selected vertex. These operations still use a preview and
are undoable; no partial edges or orphan points are exported as STL.
