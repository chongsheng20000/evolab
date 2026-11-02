# Evidence and interpretation — v10.8

`v108_observed_3seeds.csv` is native C++ output recorded every 100 ticks (three seeds, 2000 ticks each). Intake entries are **one-tick samples**, not integrated lifetime throughput. Populations are capped at 200; reaches to capacity cannot be interpreted as comparative fitness. `births` and `deaths` are actual global counters; a rising generation number alone does not prove an adaptive innovation. Carrier counts overlap because a genotype can express multiple organ types.

`tests/test-functional-v108.sh` uses a matched-background one-tick test on actual native Sim, changing only organ type and segment width; it verifies different substrate removal, root movement drag and mutation-free inheritance. The intervention establishes immediate mechanism causality, not long-term natural selection or open-ended evolution.

For a valid adaptive advantage claim, conduct pre-registered seed-replicated tournaments, matched initial conditions, removal-and-rescue interventions of each organ, lineage-level survival/reproduction comparisons, and a stable long-running world. Current measurements are a stepping stone.
