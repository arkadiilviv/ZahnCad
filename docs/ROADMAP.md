# Delivery roadmap

The milestone implementation tasks and acceptance gates are recorded in
[FEATURES.md](FEATURES.md) and the root README's limitations. The initial
milestone schedule, assuming 4–6 engineers and dental partners, is:

| Milestone | Target from kickoff | Gate |
|---|---|---|
| M0 | Months 0–2 | Workflow review, evaluation corpus, geometry feasibility, intended-use assessment |
| M1 | Months 2–4 | Tested viewer/project workflows, asynchronous loading, measurement/clipping, packaged Windows/Linux builds |
| M2 | Months 4–7 | Scan preparation, registration, margins, insertion and blockout |
| M3 | Months 7–11 | Single crown plus bench measurements and reviewed CC0 library |
| M4 | Months 11–17 | Core restorative beta and manufacturing checks |
| M5 | Months 17–24 | Multi-lab research release and complete reliability evidence |
| M6 | Conditional | Market-specific professional release |
| M7 | Years 3–5+ | Expanded modules with separate acceptance gates |

These are estimates, not guarantees. Version 0.2 completes the first viewer
reliability development batch: background loading/cancellation, undo/redo,
display LOD, improved recovery, and verified Windows/Linux desktop builds.
M1 still needs broader manual evaluation, installer/AppImage packaging and real
scanner performance measurements. M0's external dental reviews and later gates
are not completed by these software tests.

Version 0.3 adds the second development batch: colored PLY/OBJ import,
selection/cropping, component isolation, controlled repair, smoothing,
simplification and sampled deviation reports. Projects preserve original and
edited geometry. This does not complete the entire M2 milestone: registration,
margin editing, insertion analysis and blockout remain next.
