# EvoLab Ω v10.9 — Real Functional Competition & Mobile Accessibility

This release **preserves the real native C++17 simulation, v10.7 genetics/ecology/checkpoint v23 and v10.8 substrate mechanics**, and improves observation and controlled intervention. Its goal is not a scripted technology tree. A conceptual anatomy image is NOT a simulation result or a final render target.

## What changes

1. **Mobile configuration drawer**: fixes the v9.8 drawer opacity/stacking/scroll bugs by hoisting the drawer to the document body when narrow. Its content has independent touch scrolling, visible close control, screen-width inputs and a 100dvh viewport. Original desktop settings toggle remains; the desktop panel is independently scrollable. An already-visible mobile quick-launch button remains functional.
2. **Direct real GPU + multiscale view**: the existing WebGL2 instanced capsule renderer consumes actual C++ per-life inherited nodes, current joint positions, finite radii, colors derived from organ types, plus original world relationships. This release adds 2-finger world pinch zoom, cursor-centered wheel zoom, pan and up to 256× camera zoom. Zoom is an observer transform, **not more simulation space, actual voxel resolution or synthetic life**. GPU fallback remains Canvas2D.
3. **Genuine in-world competition ledger**: full-population, per-tick at-risk exposure for carriers and noncarriers of each inherited organ type (absorber, digester, root); parent reproduction events, deaths, inherited offspring counts. `organCompetition[k]` is an 8-value array:
   `[noncarrier-life-ticks, carrier-life-ticks, noncarrier-parent-births, carrier-parent-births, noncarrier-deaths, carrier-deaths, noncarrier-genotype-births, carrier-genotype-births]`.
   Carrier classification is **genomic** (body node type), including immature individuals. It is not a species label. Groups overlap: a life may express multiple types. The browser compares births and deaths per 1000 life-ticks rather than raw population numbers; these are observational, *not causal estimates*.
4. **Controlled functional intervention**: optional `--organ-ablation none|absorber|digester|root`. This disables a node type's food/detritus uptake; root intervention additionally removes its anchoring traction. **Genotype/body remains intact**, so this is a *reversible phenotypic knockout*, not permanent deletion. `organAblation` shown in native state JSON. Browser world configuration adds this choice before new runs. For strong paired tests: save a binary checkpoint, start two branches from the **identical checkpoint** (one control and one knockout); compare resource flux, birth/death events, energy and lineages over multiple seeds. An intervention can cause harm, benefit, or no change; the model does not declare a winning strategy.
5. **Honest bounds**: exposure counters are in-memory observation and begin again on checkpoint restore; `organLedgerSinceTick` shows the start of each counter window. Existing checkpoint version is unchanged. One currently live Render instance has a **one-world compute slot**, 350 max capacity, 12,000 ticks, and ~8-minute wall-clock cap. Browser interface does **not** permit loading an arbitrary server checkpoint; CLI offers controlled same-checkpoint branches. The free instance can sleep and lose ephemeral files. This cannot guarantee higher intelligence, civilization or high dimensional organisms.

## Tests completed

- `bash native/build.sh` builds the native engine in standard Linux (C++17).
- `npm test` verifies real C++ online API, multiple isolated visitors, quota restrictions and a 100-tick public root intervention.
- `npm run test:functional` checks the original v10.8 substrate debit, root drag and matched genotype organ function tests.
- `npm run test:competition` starts a 300-tick reference world, branches at an **identical binary checkpoint** into a baseline replay, absorber knockout, digester knockout, and root knockout; checks same-checkpoint deterministic repeatability, zero native uptake of the disabled organ, and real difference in recorded world outcomes. It also inspects inherited carriers across three additional seeds at 850 ticks.
- Chromium viewport check (390×844) shows working drawer scrolling (4,705 px usable scroll span), and captures initial and bottom drawer states. No online Render live run of this release is claimed.

### Installing as upgrade of the current GitHub source

In Termux, ensure v10.9 ZIP was downloaded to `~/storage/downloads`:

```
cd ~/evolab
unzip -o ~/storage/downloads/EvoLab_Omega_v10_9_Competition_Mobile_Fix.zip -d .
git add -A
git commit -m "EvoLab v10.9 mobile settings and functional selection evidence"
git -c http.version=HTTP/1.1 push origin main
```

After successful push, open your Render service: https://dashboard.render.com/web/srv-dbjn58942hec73dhe9b0. Check Deploys: the live commit **must match the new GitHub commit**. Automatic Render webhook may not be working: previously its history only showed the initial v10.7 commit even when later GitHub files changed. Do not assume the live URL updated until that deployment is recorded.

Then use: https://evolab-omega-public.onrender.com/native.html

## Independent experimental use

For controlled, offline research on a Linux machine:

```
bash native/build.sh
npm run test:competition
```

The `tests/test-competition-v109.py` script uses the same native binary and checkpoint format as the web service. Compare per-class reproduction rate `parent birth events / life-ticks`, per-class mortality `deaths / life-ticks`, and direct grid-debited nutrient uptake. Repeat across seeds and experiment horizons. Account for population saturation, mutation, shared ancestry, spatial heterogeneity and changing environment: correlated trait prevalence is not a causal explanation; short paired knockout outcomes are not evidence of sustained adaptive innovation.
