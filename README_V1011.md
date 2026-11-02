# EvoLab Ω v10.11 — Environment × Genotype: within-model evidence, linked observer genealogy

**Version:** local source and experimental release. **Deployment:** NOT published to GitHub or Render by this release operation. Keeps v10.10 C++17, 2D simulation, public multi-visitor server, functional organ dynamics, WebGL2/Canvas observation and v10.9 mobile drawer. No fixed species/technology tree is introduced.

## What is actually implemented

### World-level controlled environments (research CLI)

Three new native flags, with explicit bounds and independent from inherited genomes:

- `--experiment-resource 0.0..3.0`: override replenishment `c.resourceRate` after binary-checkpoint restore. Sentinel `-1` means no override. Food fields still come from the actual reference checkpoint and remain finite; the new rate changes subsequent resource replenishment.
- `--experiment-migration-cost 0.0..3.0`: dissipate a configurable amount of individual energy only on an **actual allowed crossing** of native 1300×850 regional boundaries. No boundary crossing, no charge.
- `--experiment-isolation 0.0..1.0`: introduce static physical regional boundary walls with progressively narrower central passage windows; rejected crossings reflect only the blocked velocity component and increment `blockedCrossings`. These walls are deliberately set by experimenters, not evolved by organisms. `0` means fully open, `1` means closed.

Values are exposed as actual native state fields `resourceRate`, `experimentalMigrationCost`, `experimentalIsolation`, `blockedCrossings`, `migrationEnergyPaid`, `regionalMigrants`. Native research flags are **not forwarded by the anonymous public web server**. The experimental migration/isolation controls are runtime-only and MUST be repeated after each checkpoint resume; the normal world state checkpoint binary version is not changed. Resource rate uses the existing checkpoint field, and new overrides are applied after restore.

### Actual organism-to-ancestry observation (read-only)

Every native visual frame includes `lineageLinks`: one row per actually selected and rendered living organism `[uid, parent_uid, founder_uid, generation, genome_signature_string, node_count, absorber_nodes, digester_nodes, root_nodes]`. The genome signature is a string to prevent precision loss from JavaScript's 53-bit integer limit. Links are computed from the **same native `display` sampling** as `individuals`, so order and UIDs match exactly.

Selecting an organism in the observer now shows its parent UID, founder UID, real structural hash, three organ counts, actual world-region ecological context, and *group-level* functional uptake for the sampled tick. If its parent appeared in a prior observer sample, the UI shows the observed parent-child structural difference and allows navigating to the parent only if still present. Observer cache is bounded (2,500 sampled UIDs). It does **not** invent unobserved parents or classify a structure as superior. The full native `--lineage` CSV provides complete births for offline analysis.

### Reproducible evidence, with bounded claims

Main study: **12 predetermined consecutive seeds** `13101..13112`, source checkpoint at tick 300, horizon 2,200; five investigator-assigned environmental conditions, two inherited genotype conditions (intact vs real root-node and subtree pruning), plus a root **function-only** ablation in the reference condition. **132 full native branches**, **1,056 temporal records**, all baseline checkpoint replays separately repeated for exact JSON-state equality. Within a seed, branches share the *same checkpoint file and SHA256*, not merely the same seed. Each branch is a separate alternate continuous timeline; branches do **not** compete with one another.

| Condition | Resource replenish | Crossing cost | Isolation | Mean Δ births: root pruning − intact (12 seeds) | Sign reversals |
|---|---:|---:|---:|---:|---:|
| Reference | 0.65 | 0 | 0 | −18.08 | 0/12 positive |
| Scarce | 0.16 | 0 | 0 | −15.83 | 0/12 positive |
| Abundant | 1.60 | 0 | 0 | −10.75 | 2/12 positive |
| Migration cost | 0.65 | 2.0 | 0 | −18.58 | 1/12 positive |
| Isolated regions | 0.65 | 0 | 0.92 | −19.58 | 0/12 positive |

The observed mean number of successful crossings in the intact reference condition was approximately 347 per simulated world and fell to approximately 52 under regional isolation. The boundary simulation registered blocked crossings (in the first six seeds, approximately 15,644 events on average), confirming that this setting changes physics rather than just the label.

Supplementary stress-test: three predetermined seeds `13101..13103` ran the **same 11-branch protocol to tick 5,000**, with **33 branches and 627 timeline records**. This is a shorter-seed, longer-horizon subset, not 12 more independent long-term replicates. At 5,000 ticks, root-pruning birth deltas under the reference regime were `[-89, +10, -140]`—even the reference effect can reverse over time.

Complete native lineage audit: 12 initial reference worlds to tick 1,600, **13,497 children linked to their real parents**, among them 4,091 of 4,134 children of root-positive parents were root-positive; 101 root-positive children appeared among 9,363 births of root-negative parents. This proves that inherited structures are actually tracked and mutated in this model; it does **not** prove they were selected for increased survival. The audit also traces three-generation paths and structural hash changes.

## How to reproduce

On a supported Linux environment with g++ / Node 20+ / Python3:

```bash
bash native/build.sh
python3 tests/test-multienv-v1011.py
npm test
npm run test:functional
npm run test:competition
npm run test:genomic
# To re-run the entire fixed matrix, split into short batches:
python3 tests/multienv-v1011.py --start 0 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part0
python3 tests/multienv-v1011.py --start 2 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part1
python3 tests/multienv-v1011.py --start 4 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part2
python3 tests/multienv-v1011.py --start 6 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part3
python3 tests/multienv-v1011.py --start 8 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part4
python3 tests/multienv-v1011.py --start 10 --seeds 2 --horizon 2200 --out evidence/v1011_batches/part5
python3 tests/merge-multienv-v1011.py --seeds 12
python3 tests/lineage-audit-v1011.py --start 13101 --seeds 12
```

Read `evidence/v1011_multienv/summary.json`, `branches.csv`, `trajectories.csv`, `paired_summary.csv`, `checkpoint_hashes.csv` and `lineages/lineage_audit.csv`. Longer subset results are under `evidence/v1011_long_horizon/`.

## Precise limitations and next stage

1. **No proof of adaptive innovation yet.** Born offspring and apparent reproductive effects at carrying capacity do not measure lifetime fitness, long-term ecological niche stability or unanticipated complex behaviors. Several scenarios have effect sign reversals.
2. Pruning root nodes removes their descendant support tissue; effect is compound morphology + mechanics + resource exposure, **not** a clean marginal root-biochemistry intervention. The reference group additionally includes phenotype-only ablation to help explore this confounding.
3. Environment manipulation is external and physical. It does not prewrite which organisms emerge. Five conditions probe the model's response; stronger claims require 24–100 seeds, multiple uncapped horizons, predeclared outcome thresholds and uncertainty intervals.
4. Visually sampled ancestors may disappear from view, die or be evicted from bounded browser cache; this is not the complete native lineage graph. Group-level uptake shown in a life dossier is **not** a measured per-life uptake.
5. A repeated same-seed checkpoint creation has occasionally produced SHA256-different binaries even when the decoded native state agreed; binary checkpoint byte-for-byte reproducibility across **independent creation** is not established. Exact replay from the **same saved checkpoint** passed across all study seeds. Checkpoint binary serialization deserves a separate canonical-padding audit before stronger claims.
6. This is still a limited 2D artificial life world. No emergent intelligence, civilization, planetary evolution, mathematically higher-dimensional universe or self-sustaining open-ended innovation has been demonstrated. GPU visualization must continue to display actual native body topology rather than predesigned species.
7. The public free-tier Render service limits slots/ticks and does not provide enough capacity for the full research matrix. The research CLI remains intentionally unavailable to anonymous web users. Cloud service requires an independently verified new GitHub/Render commit.
