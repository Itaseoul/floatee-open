# Self-righting — computed, not just claimed

The drifter must ride **antenna-up** or it goes deaf: a submerged antenna kills GPS and cellular
(FLOAT_STANDARD, Rule 2 of the four rules). The design achieves this with belly ballast. This page
turns "self-rights antenna-up" from an assertion into a number, with an explicit statement of what
the number does and does **not** prove.

Calculator: [cad/self_righting.py](cad/self_righting.py) (pure stdlib; defaults mirror `cad/params.scad`).

```
$ python3 hardware/cad/self_righting.py
  mass 153 g  ->  submerged 13%  waterline 16.6 mm  freeboard 73.4 mm
  KB 9.8  BM 35.2  ->  KM 45.0 mm (~= axis R=45, circular-section property)
  KG 33.9 mm  (must be < axis 45 for antenna-up)   ->  GM = R - KG = 11.1 mm
  righting arm GZ(phi) = GM*sin(phi):  10deg=1.9  30deg=5.6  45deg=7.9  60deg=9.6  90deg=11.1 mm
  VERDICT: PASS  (need GM>5 mm and KG<42 mm; antenna-up single-equilibrium confirmed only by tank test)
```

## The model

The float lies **horizontal** (schema calibration `form.attitude`), so this is **roll stability of a
partly submerged circular cylinder**, radius R = 45 mm (bottle_id 90). Standard hydrostatics:

- Displaced volume `V = mass / ρ_water`; solve the waterline height `h` so the submerged circular
  segment × body length equals V. At the ~153 g reference mass the bottle floats high (~13% submerged,
  73 mm freeboard) — it is buoyancy-rich, as intended (a sealed 1 L bottle has ~4.8× reserve).
  **Correction 2026-09-26:** the 1 L bottle is no longer recommended (the 84×112 mm panel does not fit). The default housing is a 1.5 L wide-mouth screw-lid container, and the panel, PCM, external holder and steel plate change the mass. All 1 L numbers here must be recalculated for the 1.5 L wide-mouth container once the product is chosen.
- **KB** = centroid of the submerged area (numeric integration). **BM** = `I_waterplane / V`.
- **KG** = mass-weighted centroid of the component budget (ballast low, board/antenna high).
- **GM = KB + BM − KG** — the metacentric height. GM > 0 ⇒ initially stable in roll.

## The clean result (why a bottle is a good hull)

For a prismatic **circular** section the metacenter sits on the section **axis at any draft**:
`KM = KB + BM = R`. The calculator prints `KM 45.0 ≈ R 45` as a sanity check. Therefore

> **GM = R − KG**, and the large-angle righting arm is the pendulum `GZ(φ) = GM·sin(φ)`.

That collapses to one memorable, exact design rule:

> **Keep the combined center of gravity below the bottle's central axis.** (KG < R.)

Belly ballast (40 g in the lowest pocket) does exactly this. At the reference build KG ≈ 34 mm < 45 mm,
GM ≈ +11 mm, and GZ is positive through 90° — the hull returns antenna-up from any roll. Move the
ballast up toward the axis (e.g. `--ballast-z 85`) and the tool correctly reports **FAIL** (GM < 0).

## What this proves — and what it does not

- ✅ **Initial roll stability and the antenna-up preference** are computed positive with margin, and the
  design rule (CG below axis) is exact for this geometry. A gross error (ballast in the wrong place,
  top-heavy payload) is caught here, cheaply, before cutting foam.
- ❌ It does **not** prove the full behaviour. Small-angle metacentric theory and a uniform prismatic
  section ignore the neck/cap taper, the ends, free-surface effects if partially flooded, wave dynamics,
  and dynamic overshoot. The **single stable equilibrium = antenna-up**, the recovery time, and behaviour
  in chop are confirmed only by the **tank/pool test** (VERIFIED_BUILD_TEST **T4**, re-run as **T7** with
  the solar payload installed). Masses here are **estimates — measure your build and edit them.**

Per the project ethic: no improvement claim before measurement. This calculation sizes and de-risks the
design; the water has the last word.
