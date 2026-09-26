# CAD — full real-size assembly (parametric OpenSCAD)

The housing is an upcycled PET bottle. Three printed inserts + one cut part (foam collar) + real-size mockups of everything else, so the **complete Tier-1 drifter exists as a real-dimension 3D assembly**. All parametric: open `params.scad`, set your measured bottle inner diameter and board/cell sizes, and everything updates.

## Open CAD — three layers

1. **Web viewer** (rotate · explode slider · per-part toggle): https://uploads.caresea.kr/seacut/hardware/index.html
2. **GitHub STL** — every `.stl` below renders in 3D in the browser.
3. **OpenSCAD parametric source** — the editable master (OSHWA prefers editable originals).

## Files

| File | Part | Role |
|---|---|---|
| `params.scad` | shared parameters | ★measure your bottle and edit here first |
| `electronics_bracket.scad` | electronics bracket (print, ≤60% infill) | holds board + 18650, self-locates on the bottle wall |
| `ballast_keel.scad` | ballast keel (print) | lead-free weight at the belly for self-righting |
| `recovery_loop.scad` | recovery loop (print) | smooth closed loop, wildlife-safe |
| `foam_collar.scad` | **foam collar ×2 (CUT master, not printed)** | flooded-reserve buoyancy — 2 collars ≈ 160 cm³ closed-cell EVA keep a flooded unit afloat (RELIABILITY §4). Cut/stack EVA sheet to this shape |
| `foam_collar_2d.scad` → `foam_collar_template.svg` / `.dxf` | **1:1 cut template** | true-mm SVG (print at 100 %) and DXF (laser/CNC). Printable A4 page with calibration ruler + layer table: https://uploads.caresea.kr/seacut/hardware/foam_template.html |
| `mockups.scad` | bottle · LILYGO board · 18650 · ballast · antennas | real-size visual stand-ins for the assembly |
| `assembly.scad` | **full assembly** | everything in deployed position; `-D explode=1` for the exploded view |
| `export_part.scad` | per-part STL export | assembled-coordinate STLs for the web viewer |
| `asm_*.stl` | 10 assembled-position parts | what the web viewer and GitHub render |
| `assembly.png` / `assembly_exploded.png` | renders | assembled / exploded |

## Edit and export

Install OpenSCAD (free, open source). Edit `params.scad`, then:

```
# one printed part (STL prints, and GitHub renders STL in-browser)
openscad -o electronics_bracket.stl electronics_bracket.scad
# full assembly previews
openscad -o assembly.png --autocenter --viewall --imgsize=1400,1000 assembly.scad
openscad -o assembly_exploded.png -D explode=1 --autocenter --viewall --imgsize=1400,1000 assembly.scad
# all viewer STLs (assembled coordinates)
for p in bracket keel loop foam_a foam_b board antennas cell ballast bottle; do
  openscad -o asm_$p.stl -D "part=\"$p\"" export_part.scad; done
# 3MF (mesh interchange for slicers/CAD)
openscad -o electronics_bracket.3mf electronics_bracket.scad
```

**About STEP**: OpenSCAD is mesh-based CSG and **cannot export true (BREP) STEP** — an earlier note here claiming `--export-format=step` was wrong. If you need STEP, import the STL/3MF into FreeCAD and convert (mesh-based, lossy) or remodel; the **OpenSCAD source stays the editable master** (which is what OSHWA asks for).

The committed `.stl` files are reference geometry. Measure your bottle and parts, adjust `params.scad`, re-export. Mockup dimensions (bottle profile, board, cell) are visual estimates — replace with your measured values.

Units are millimeters. Print in a durable, non-toxic filament (PETG recommended for water contact), **infill ≤60%** (`print_infill_max` — keeps the 160 cm³ foam spec valid; solid prints need ~175 cm³). No supports needed if oriented flat.

**Solar variant (Tier 1.5)** — ★**Update 2026-09-26:** the current design is a **6 V flat panel (84×112 mm) on the lid of a 1.5 L wide-mouth container**, not a thin-film wrap, so this mock and the housing 3D must be redrawn once the container product is chosen (not yet chosen). The notes below describe the older wrap mock. [`solar_variant.scad`](solar_variant.scad) + `assembly.scad` with `-D solar_enable=1` (render [`assembly_solar.png`](assembly_solar.png)). The [long-dwell solar drifter](../TIER1_5_SOLAR_DRIFTER.md) adds **no new printed part** — flexible thin-film cells wrap the body and the charge controller/NTC/vent are COTS. `solar_variant.scad` is a **viz-only mock (exports no printable STL)**. CAD-side gates: (1) keep the ballast keel low and **ballast at 40 g unchanged** — never trade it down to offset solar mass (raises CG, kills T7 self-right); (2) re-cut the foam collar with the **density-aware +6.7 cm³/10 g** (panel ρ≈1.64, *not* the ballast 7.9→11.7); (3) the RF window is a top +Z sector kept film-free — ★the local→global axis map is easy to invert, so **render and confirm the film-free wedge sits over the antennas** (done: `assembly_solar.png`); (4) cells must be **non-metal RF-transparent substrate** (B5), verified by T6 with cells installed. No STL change.
