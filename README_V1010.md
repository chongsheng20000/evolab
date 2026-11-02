# EvoLab Ω v10.10 — Genomic Intervention & Long-Horizon Causal Evidence

## Outcome / scope

This is an **actual C++17 engine update** built on v10.9 (keeping all prior capabilities), NOT a prescribed evolutionary technology tree. It remains a finite 2D native artificial-life simulation, not a simulated real universe or proof that civilization/high-dimensional life must emerge.

- **Actual inherited structural deletion**: experimental native command `--genome-prune absorber|digester|root`, only with `--resume CHECKPOINT`. For each currently living genome, removes target organ nodes **and all downstream descendant tissue nodes**, remaps surviving body parent indexes and living mechanical state, reconciles finite food/detritus cargo into the local world grid without inventing new resources, leaves neural network and other hereditary parameters otherwise unchanged. Descendant organisms inherit the edited body through the existing native mutation/reproduction engine. A deleted structure can emerge again by genuine mutation; its future is not scripted.
- **Reference structure rescue**: experimental `--genome-rescue ORIGINAL_CHECKPOINT`, only with `--resume EDITED_CHECKPOINT`. Restores original inherited body topology and neural blueprint for currently surviving individuals whose persistent UID existed in that donor checkpoint; does **not** resurrect dead individuals, reconstruct genotypes of descendants born afterward, or reset the world / RNG / resource fields. Because joint state is reinitialized and residual tissue cargo returned to environment, this is an **experimental genetic restoration, not an exact inversion of the preceding world history**.
- **Reproducible parallel branch protocol**: from the **same SHA256-verified binary checkpoint** per world, independently replay baseline, root functional ablation (body still present), actual root-node/descendant genetic deletion, and early deletion with vs. without reference structural rescue. The baseline must exactly reproduce on a second independently executed process. In the causal study, 12 predetermined independent seeds (`12001..12012`) were tested at **3,000 and 6,000 native ticks**. No favorable seeds were omitted. This is *within-model* perturbation evidence, not a general causal theorem about biological fitness.
- **Observation and evidence**: retains v10.9 mobile parameter drawer and real WebGL2/C++ body-state renderer. Body genetics, cumulative native births/deaths, carrier/noncarrier life-time exposure, reproduction/death events, finite substrate uptake and all checkpoint lineages remain available. `genomePrunedLives`, `genomePrunedNodes`, `genomeRescuedLives`, `genomeRescuedNodes`, and intervention mode are included in native JSON state. These are experiment metadata; derived metrics do not drive behavior.
- **Public security boundary**: genome edit options are native CLI research-only. The public Node controller retains an explicit numeric-option allowlist and does not forward arbitrary checkpoint paths or genome edit command options. Public Render remains one active simulation slot (free plan), 350 cap, 12,000 tick cap and short wall-clock quota. The long experiments were run on the local C++ engine, not falsely represented as completed by visitors' browsers.

## Tests performed

- `bash native/build.sh`: C++17 native compiles.
- `npm test`: real C++ public server, same-origin rejection, two distinct sessions and compute slot quota.
- `npm run test:functional`: direct finite food/detritus debit and inherited organ mechanics.
- `npm run test:competition`: v10.9 same-checkpoint phenotypic organ ablation and lineage evidence.
- `npm run test:genomic`: actual root node deletion, donor-checkpoint SHA integrity, restoration of surviving reference-UID hereditary topologies, and reappearance of actual root-bearing nodes from restored genotype.
- `evidence/v1010_causal_study/`: 12 predetermined seed long-horizon (6,000 ticks) paired branch experiment, per-seed checkpoint hashes, 60 branch rows, 600 native trajectory observations, aggregated statistics with exploratory paired bootstrap intervals, and explicit limitations.
- `evidence/v1010_causal_study_3000/`: same design at 3,000 ticks (12 seeds). The two time horizons share seeds and are not statistically independent replicates.

### Core observed effects (6,000 tick, all 12 seeds, reference branch: no intervention)

| Model intervention | Mean difference in birth events vs baseline | Direction of birth effect across 12 seeds | Experimental meaning |
|---|---:|---|---|
| Root function disabled, body remains | −25.6 births | 4 higher, 8 lower | Phenotype-only suppression, genomic structure intact |
| Root organ nodes + descendants deleted | −70.5 births | 2 higher, 10 lower | Actual hereditary topology loss (can affect downstream tissue) |
| Root nodes deleted; reference topology restored at tick 330 | −17.4 births | 4 higher, 8 lower | Restored only tracked survivors; exact world history not reversed |

`end_pop` remained nearly constant due to population capacity. Outcome differences in birth/death events do **not** establish long-term adaptation or structural innovation by themselves. Variance across seeds is material: two genomic-deletion seeds produced *more* birth events, not fewer. At 6,000 ticks, an exploratory paired bootstrap 95% interval for the mean genomic-pruning birth effect was **[−116.0, −25.1]** events, within this specific model, configuration, seed set and post-checkpoint window; this is not a universal measure of the root organ's fitness.

## Running the study on Linux or local server

```bash
bash native/build.sh
npm run test:genomic
# Full study in short batches, to avoid losing evidence to job timeouts:
python3 tests/causal-study-v1010.py --start 0 --seeds 4 --horizon 6000 --out evidence/v1010_batches/6000_part0
python3 tests/causal-study-v1010.py --start 4 --seeds 4 --horizon 6000 --out evidence/v1010_batches/6000_part1
python3 tests/causal-study-v1010.py --start 8 --seeds 4 --horizon 6000 --out evidence/v1010_batches/6000_part2
python3 tests/combine-study-v1010.py --horizon 6000
```

The zipped release contains batches from a different 2+4+3+3 partition, and the aggregator accepts either partition via filenames `6000_part*`. To avoid stale duplicate results, remove old `6000_part*` directories before re-running with another partition. Python 3 and a Linux C++17 toolchain are required; Termux/Android may need a locally compiled Android native binary (the included ELF is for Linux/Render).

## Experimental validity boundaries

1. These are interventions within the simulator's chosen rules, not inference about real biological organisms.
2. Pruning target organ subtrees changes morphology, metabolic cargo placement and mechanics simultaneously. It isolates the consequence of that **specific structural perturbation**, not the marginal advantage of a single independent biochemical function.
3. The resurrection of earlier genetic structures is **researcher-performed rescue**, not proof of naturally re-evolved functions. Mutations may independently reintroduce root types later; separate genealogical tracing is needed to establish their origin.
4. Causal evidence for an effect on simulated births is **not** evidence for sustained open-ended adaptive innovation. This requires inherited novelty, persistence across ecological regimes, suitable sham controls, evolutionary contingency and additional generations/replicates.
5. The v10.9 GUI remains an observer; `--genome-prune` and `--genome-rescue` are not exposed to anonymous public web visitors, to preserve computational and access constraints.
6. Images are only concepts unless directly drawn from native simulation states. GPU observation does not synthesize scripted species or higher dimensional physics.

## GitHub/Render state

This ZIP is built locally; it is **not** automatically committed to GitHub or deployed. Use the verified repository `chongsheng20000/evolab`. A GitHub 403 integration write restriction previously prevented automated commits. Once pushed, verify Render deploy's **commit SHA**, not merely `Live` status; historical Render logs showed the initial v10.7 commit while the GitHub repository had newer startup fixes.
