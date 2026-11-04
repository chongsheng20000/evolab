# EvoLab Ω v10.12.1 — Stable World Camera & Single-Pass Pose Rendering

This is a **frontend-only stability hotfix** for the already deployed v10.12 C++ system. It changes no heredity, natural selection, resource physics or mortality rules.

Fixes:
1. Render exactly **one** UID pose map per visual frame. Canvas relation overlays and WebGL2 use the **same** map instead of calling the stateful smoothing function twice.
2. Start in a **stable world overview**. v9.8's legacy auto-focus followed a random large-bodied organism and shifted the entire world before the user selected it. Users can still click to follow and pause a real organism.
3. Clamp world camera to the actual simulated domain at the current zoom. The resource field is not extended or faked; the camera simply cannot display non-world black strips beyond the map boundary.
4. Display a distinct v10.12.1 label; the HTTP `/healthz` also reports `v10.12.1-stable-observation` so the exact deployed version can be checked.

Not fixed by this patch: real organism death or energy depletion, underlying C++ physical or neural behavior, longer cloud run limits, GPU performance on every phone. Those need native lifecycle traces and hardware-specific profiling.

## Verification performed

- `python tests/test-observer-v1012.py`: smartphone viewport, settings drawer scrolling, world coverage, native snapshot and pause on select.
- `python tests/test-display-stability-v1012.py`: no automatic first-run follow, one stateful pose update per render, bounded camera at both world edges, no browser JS errors.
- Existing native public API isolation and physical/ecological/genomic/multienvironment tests passed unchanged.

The GitHub integration currently rejects direct commits with HTTP 403. Upload these 5 files plus this README and test through the authenticated user repository. After a successful GitHub push, **verify Render's live commit SHA matches**, because this account's Render autobuild has previously failed to trigger. Do not equate `git push` success with a live deployment.
