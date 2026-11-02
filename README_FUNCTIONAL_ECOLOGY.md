# EvoLab Ω v10.8 — Functional Ecology (experimental)

This extends the original **real C++17 engine** of v10.7, retaining multi-agent ecology, genetic reproduction, body topology, organ mutation, internal cargo transport and checkpoint file format. Node organ types are inherited and can mutate or disappear by natural selection. It does not prescribe new species or an evolutionary destination.

## Implemented mechanics

- **Type 2 / absorber:** extracts finite **food** into the inherited internal transport graph. Mobile bodies can forage across different resource patches.
- **Type 5 / digester:** extracts finite **detritus** (including dead-body residues) into the same graph; energy conversion is less efficient and competes with other detritivores.
- **Type 7 / root:** extracts finite **food** at its own endpoint, with additional contact benefit when it moves slowly over fertile substrate. Expressed root geometry also mechanically drags against terrain, reducing movement. This is a physical tradeoff, not an AI-coded role or new matter source.
- Internal transport capacity now depends on **inherited segment width, length and conductance**, so longer/branched bodies can have genuine logistical bottlenecks.

At each native state sample, `functionalUptake[absorber, digester, root]` are per-tick amounts removed from actual resource grids, `functionalCarriers` counts expressed-organ carriers, and `functionalMeanGeneration`/`functionalMeanEnergy` report the corresponding living populations. These carrier classes overlap and are not taxa. A dashboard evidence panel shows the same observed values. The existing lineage CSV carries the inherited structure hash, generation and ancestry for reproduction studies.

## Verification and limitations

`tests/test-functional-v108.sh` performs controlled matched-genome perturbations, checks finite-grid debit against measured intake, root drag, and child inherited body type. Run `npm test` after `bash native/build.sh` to verify two independent web visitors and real native simulation.

This is a **mechanistic capability test**, not evidence that any strategy was selected over thousands of generations. Validation of adaptive advantage demands competing descendants, random-seed replication, and matched-genome ablation. The v10.7 binary checkpoint format remains unchanged, but switching simulation mechanics changes future trajectories and old-run bitwise replay must not be presumed. Output counters are transient readouts rather than checkpoint fields.

Render deployment: push these source files to `chongsheng20000/evolab/main`; the existing Render service auto-deploys. Render free plan currently limits compute to one concurrent active native simulation. Free container storage is ephemeral.
