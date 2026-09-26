# From reference design to validated project — the critical path

Everything in this repo is a **reference design**. The design, the data standard, the governance, and
the honesty guards are in place. What is **not** yet in place is the one thing that turns a design into a
project: **one real deployment with recovered units and a citable dataset.** This page is the shortest
honest path to that, written so the maintaining org can hand it to whoever runs the first water day.

The world-class review said it plainly: *design/governance/honesty are strong, but with zero built units
and zero field validation this is still a blueprint.* Nothing below is AI-doable — it is bench time,
water time, and a deposit. That is the point.

## Stage 0 — one bench unit (days, indoors)

Build a single Tier 1 unit and run the bench gates already defined in the repo. No water, no permits.

- [ ] Assemble per [BUILD.md](BUILD.md) + [../hardware/BOM.md](../hardware/BOM.md); print the three inserts.
- [ ] **F1 soak** and **F2 swamp** ([../hardware/FLOAT_STANDARD.md](../hardware/FLOAT_STANDARD.md)) — mass gain < 5%, floats upright 72 h.
- [ ] **Firmware bench loop** ([../firmware/README.md](../firmware/README.md)) with `BENCH_HTTP 1`: fix → buffer → POST 200 → sleep → wake, `seq`+1; kill coverage 2–3 cycles, confirm the buffer flushes in one batch.
- [ ] **Power (G5):** measure sleep current and the transmit peak **on battery** and replace the estimates in [../hardware/ELECTRONICS_POWER.md](../hardware/ELECTRONICS_POWER.md) with measured numbers. This is the single biggest source of "estimate → fact."
- [ ] **Self-righting (T4):** float it, roll it, time the return to antenna-up. Compare to [../hardware/SELF_RIGHTING.md](../hardware/SELF_RIGHTING.md) (predicted GM ≈ +11 mm). Update the component masses in the calculator with your measured build.
- [ ] If HTTPS: enable **TLS auth** and watch a wrong cert get **rejected** ([../firmware/README.md](../firmware/README.md) → Security). Pin the TinyGSM-fork commit.

Exit criterion: F1+F2 pass → the unit is a **Verified Float** (`verified_build:true`), and the power/righting numbers are measured, not guessed.

## Stage 1 — legal + site (parallel with Stage 0)

Only the org can do these; start them early because they gate the water day.

- [ ] **Radio.** Korea (updated 2026-09-26): one unit per model imported for personal use needs no application; the RRA research/development exemption (≤ 1,500 units) is needed for **two or more units of the same model** or several units in the org's name. Elsewhere: your national radio authority, or lean on a global IoT roaming SIM. See [DEPLOY_responsibly.md](DEPLOY_responsibly.md).
- [ ] **Water manager.** Pick an **ordinary tributary reach** (managed by the city/district), not a national river / protected zone, for the first pilot. Get the sign-off. (Nakdong main channel + estuary = national + natural-monument = do **not** start there.)
- [ ] **Notify** nearby fishers / harbor / any water users so a sealed electronic box is expected, not an alarm.
- [ ] **Recovery plan.** Who retrieves, from where, with what (pole/net/boat). Recovery is the identity, not an afterthought.

## Stage 2 — the first release (one water day, a handful of units)

Small and honest. The goal is not coverage; it is **a recovered track and a measured recovery rate.**

- [ ] Release **3–10 units** over a short reach; log deploy point/time per unit ([../data/schema.md](../data/schema.md) §3 unit directory, `access_class`).
- [ ] Run the **PRELAUNCH checklist** ([../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md](../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md)); if minors are involved, **SAFETY_MINORS is mandatory** ([../SAFETY_MINORS.md](../SAFETY_MINORS.md)) — students never enter water, adults recover.
- [ ] Watch the live track; **recover every unit** (count-in = count-out). Record each `fate` with evidence.
- [ ] Measure: recovery rate, dwell time, real drift vs. the estimate, how often coverage dropped.

Exit criterion: ≥ 1 clean recovered trajectory, a **measured recovery rate**, and a `fate` for every unit.

## Stage 3 — make it citable (the deposit that ends "pending")

This is what lets anyone build on the work and what every "field validation pending / DOI pending" line
in this repo is waiting for.

- [ ] Export the run to standards: [../data/export_standards.py](../data/export_standards.py) → SensorThings JSON + CF-NetCDF (see [DATA_STANDARDS_CROSSWALK.md](DATA_STANDARDS_CROSSWALK.md)). Keep the `qc`/`provenance`/uncertainty flags.
- [ ] Deposit the dataset with a **DOI** (Zenodo, or SEANOE for marine) using [../CITATION.cff](../CITATION.cff). Now `obs_id` + DOI is a real citation.
- [ ] Write a short methods note (what was built, released, recovered, measured, and every honest limit from schema §4.5). A preprint or a data-paper is enough; it does not need a journal to be real.
- [ ] Update the repo: replace "estimate/pending" lines with measured values and the DOI; register the built variant ([../registry/](../registry/)).
- [ ] Only after `steward_verified` data exists is an EMODnet-style submission realistic.

## What "done" looks like

One reach, a handful of units, all recovered, one citable trajectory dataset with a DOI, and the repo's
estimates replaced by measurements. That is the moment this stops being a blueprint. Everything else in
the repo already supports it; this is the part that needs a person, a river, and an afternoon.
