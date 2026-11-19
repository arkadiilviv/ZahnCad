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
M1 still needs broader manual evaluation and real scanner performance
measurements. M0's external dental reviews and later gates are not completed
by these software tests.

- **M0 in progress.** The [research evaluation protocol](EVALUATION.md) records
  the draft intended use, synthetic baseline and initial typodont import and
  navigation checks. Independent measurements and external review remain open.
- **M1 software verified.** The viewer checks cover surface measurement,
  two-scan persistence, and forced-close recovery with a damaged latest
  snapshot. CI builds the Windows installer and Linux AppImage.
  The [acceptance checklist](M1_ACCEPTANCE.md) tracks clean desktop checks and
  representative performance runs.

- **M2 software implemented.** Scan preparation includes reversible geometry
  edits, rigid landmark registration, persistent margin editing, sampled
  insertion analysis and directional blockout. M2 acceptance still needs
  approved representative scans and reference measurements.

- **M3 prototype implemented.** Select a permanent tooth on the FDI chart and
  use its saved preparation margin to generate a hollow research crown. The
  candidate can be saved and compared with case surfaces. Anatomical design,
  a reviewed CC0 anatomy library and physical bench measurements remain open.

- **Desktop UI refresh implemented.** The dark workbench groups current tools
  into Scan, Prepare, Design, Analyze and Export, with a scene panel, case
  statistics and Tool Properties beside the viewport.
