# Viewer reliability (0.2)

Undo/redo covers scan import/removal, visibility/opacity and v0.3 geometry edits.
Geometry commands share immutable original/current snapshots. Dragging opacity
previews the value and creates one command on release. Navigation, clipping and
camera settings are view state and do not create project edits. Undo/redo uses
shared immutable meshes and Qt's shared original-byte buffers. The saved history
index determines whether a case needs saving. History starts fresh on reopen.

STL and project opening are transactional: read, parse, scale and prepare display
geometry in a worker, then commit to the document on the UI thread. Cancellation
checkpoints cover file chunks, parsing/scaling loops and display generation.
VTK filter progress callbacks translate cancellation into native abort requests;
exceptions never escape VTK callbacks. Errors/cancellation leave the current case
unchanged. Large allocations and destruction may delay cancellation briefly.

Over 200,000 triangles, a clustered navigation mesh is prepared off the UI
thread. Interaction events select this mesh and restore the full mesh afterwards.
This is a visual approximation with no dental error guarantee. Measurements,
diagnostics, saved source data and research export use original geometry.

Autosave captures a shared immutable snapshot every 60 seconds while dirty,
without disabling editing. Recovery writes are atomic and retain a previous
snapshot. A damaged latest snapshot falls back to the previous one. Successful
manual save or deliberate case replacement clears obsolete recovery. Generation
tracking prevents an in-flight autosave from resurrecting an old case after save
or replacement. Closing waits for outstanding workers and cancels cancellable
work before prompting about unsaved changes. One instance per user-data directory
is enforced with QLockFile. Recovery remains dirty until saved to a chosen path.

Limits: manual saving and initial GPU upload can still pause the UI. Binary STL
and project-memory limits remain unchanged. Undo history is session-local.
Real patient scans, crash injection, large multi-case memory use and sustained
presented-frame performance require further evaluation.
