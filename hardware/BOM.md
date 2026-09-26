# Bill of Materials (BOM)

Parts by function and generic spec first, then a globally shipping reference part, then local substitution. Prices are rough retail USD and vary by country. Full context and regional tables: [../docs/OPEN_HARDWARE_BOM_global.md](../docs/OPEN_HARDWARE_BOM_global.md).

Reference board is the LILYGO T-A7670G R2 with onboard L76K GNSS. It is cellular (LTE Cat.1 bis) plus a separate GPS chip. It is not satellite. Do not substitute the SIM7080G (LTE-M/NB-IoT), which has no verified attach on Korean commercial networks.

## Participant kit (current, 2026-09-26)

The tiers below are kept as the reference design. What participants build now is **one kit, no tiers**, each participant buying one unit in their own name. Total about **KRW 92,908** (electronics 60,808 · SIM 8,800 · housing and waterproofing 21,500 · recovery 1,800; KRW 42,950 without the board). Board, cell, PCM, panel and SIM are retail prices; the rest are estimates, and the container product is not yet chosen.

| Group | Part | Spec |
|---|---|---|
| Electronics | LILYGO T-A7670G R2 (External GPS) | ESP32-WROVER-E + A7670G LTE Cat.1 + L76K GPS, antennas and PH2.0 power lead included |
| Electronics | Samsung 30Q 18650, **unprotected** | 3000 mAh; in an **external holder**, through a PCM to the board's rear holder terminals; board holder stays empty |
| Electronics | PCM (protection module, cuts over-discharge, over-charge and short circuit) | 3.7 V 1-cell, continuous ≥3 A (LTE peak about 2 A) |
| Electronics | Solar panel SZH-SPB017 | 6 V 220 mA, 84×112 mm, outside on the lid, into board JST P1 (SOLAR_IN) |
| Electronics | Zener 1N5339B | 5.6 V 5 W, across the panel leads; protects the on-board CN3065 (absolute max 6.5 V) |
| SIM | Participant's own domestic nano SIM | recommended HelloMobile Slim 500MB (LG U+), voice-inclusive plan, activate in a phone first |
| Housing | 1.5 L wide-mouth container | screw lid + gasket, flat top that seats the 84×112 panel; PG7 gland; neutral-cure silicone; epoxy |
| Moisture | 5 g silica gel + humidity indicator card | sealed by default; ePTFE vent M12 optional |
| Recovery | 430 stainless plate Ø40×1 mm, retro-reflective tape, contact label, paracord loop, orange/yellow tape | the plate is for a drone electromagnet (see [PRELAUNCH_RECOVERY_CHECKLIST.md](PRELAUNCH_RECOVERY_CHECKLIST.md)) |
| Optional | NTC 10 kΩ B3950 + 28.7 kΩ | 0 ℃ charge cutoff, replaces R9; before Incheon / winter releases |

## Tier 0 — Phone-in-a-Bottle (USD 5–35)

| Function | Generic spec | Reference | Local substitution | ~USD |
|---|---|---|---|---|
| Compute + GPS + modem | old Android phone with GPS and 3G/4G | reused phone | any working phone, ideally reused e-waste | 0–30 |
| Logger / uplink | app that POSTs GPS to a URL | GPSLogger (open source) | any GPS logger with custom URL POST | 0 |
| SIM | prepaid data SIM | local data SIM (Korea: participant's own domestic SIM in their own name) | global IoT SIM (1NCE / Soracom) | 5–15 |
| Housing | RF-transparent float, sealed | upcycled PET bottle + dry bag | any clear plastic bottle | 0–5 |
| Ballast + visibility + recovery | bottom weight, bright tape, contact label, loop | sinker + tape + QR | any dense non-toxic weight | 2–5 |

## Tier 1 — Cellular Bottle (default, USD 50–70)

| Function | Generic spec | Reference | Local substitution | ~USD |
|---|---|---|---|---|
| Compute + modem + GPS | ESP32 + LTE Cat.1/Cat.1 bis + onboard GNSS | LILYGO T-A7670G R2 "With GPS (L76K)" — ★pick the **A7670G (2G 4G)** variant, not "4G No SMS"; ★the A7670 **modem has no GNSS**, so "with GPS" simply means an L76K chip wired to the ESP32 UART. If that variant is out of stock, a "Without GPS" board **+ a separate UART GNSS module** is the same architecture — wiring, the critical power-gating rule, and the extra gates are in [GPS_MODULE_WIRING.md](GPS_MODULE_WIRING.md) | ESP32 + A76xx board with real GNSS; pick A7670 band variant for your region | 27–33 |
| Battery | 18650 cell + **protection circuit**, verify holder length | **Samsung 30Q unprotected** in an **external 1-cell holder**, wired through a **PCM (protection module: cuts over-discharge, over-charge and short circuit; 3.7 V 1-cell, continuous ≥3 A)** to the board's rear holder terminals; **leave the board's own holder empty** (protected cells are 69–70 mm long and do not fit the 65 mm holder) | any unprotected 18650 + 1-cell PCM rated ≥3 A continuous | 5–8 |
| SIM | local data SIM (Korea) or global IoT roaming SIM (elsewhere) | **Korea: participant's own domestic SIM in their own name**, recommended HelloMobile Slim 500MB (LG U+ network, NFC SIM KRW 8,800, KRW 1,700/month). Choose a **voice-inclusive plan** (data-only plans can refuse unregistered devices) and **activate it in a phone first**, then move it to the board | global IoT roaming SIM (1NCE / Soracom) outside Korea | 5–14 |
| Antennas | LTE + GNSS, kept above water | board-included uFL antennas | extend outside if sealing | 0–8 |
| Housing | RF-transparent float, sealed, antenna above waterline | upcycled PET bottle | any clear plastic bottle | 0–5 |
| Ballast (self-righting) | low weight so antenna rights up | sinker at belly + printed keel | any dense non-toxic weight | 2–5 |
| Sealant | marine/silicone sealant + grommet | marine silicone | any waterproof sealant | 3–8 |
| Visibility + recovery | bright tape, waterproof QR/contact label, loop | tape + QR + paracord | local equivalents | 2–5 |
| Printed inserts | bracket + keel + loop | `cad/*.scad` | print locally | 1–3 |

## Tier 1.5 — Solar-assisted (long-dwell / recovery-insurance) — now standard in the participant kit

For missions where a unit **dwells in-coverage for weeks** (estuary retention — the Han-estuary study shows most litter lingers 1 month+), a battery-only unit dies before it can be recovered. Add a **6 V flat solar panel on the lid** to extend reporting life so the unit stays trackable and recoverable. ★**Solar extends power, not coverage** — a unit that leaves cellular range goes silent regardless of charge (that is satellite / Class 3). Not for short recover-in-days missions (use Tier 1). Full spec, the self-righting-preserving design, and the extra bench gates (G5 solar harvest, T7 self-right with the panel fitted): [TIER1_5_SOLAR_DRIFTER.md](TIER1_5_SOLAR_DRIFTER.md).

| Function | Generic spec | Reference (orderable) | ~USD |
|---|---|---|---|
| Solar panel | **6 V flat panel** mounted **outside on the container lid**, cable through a PG7 gland, epoxy on the panel back. (The earlier wrap-around thin-film design is no longer used: cut film changes voltage/current, loses busbars and leaks at the cut.) | SZH-SPB017, 6 V 220 mA, 84×112 mm (open-circuit voltage not yet measured) | ~2.5 |
| Charge control | ★**on-board CN3065** via the board's white JST **P1 (SOLAR_IN)**; a **5.6 V 5 W zener (1N5339B)** across the panel leads clamps the input, because the CN3065 absolute maximum is 6.5 V. No external charger module | 1N5339B zener | ~0.4 |
| Low-temp cutoff | **optional**: 0 ℃ charge cutoff with an NTC; fit it before Incheon / winter releases (January mean minimum −4.8 ℃ in Incheon) | NTC 10 kΩ B3950 + 28.7 kΩ 1 % resistor, replaces R9 | 0.4 |
| Isolation diode | not in the participant kit BOM; whether one is needed is to be confirmed on the bench (확인 필요) | — | — |
| Vent | **optional**. Default is **sealed + 5 g silica gel + humidity indicator card**, closed indoors in dry air; decide on an ePTFE vent (M12) after the bench immersion test | ePTFE vent M12 | ~2 |
| Extra foam | buoyancy must be **recalculated for the 1.5 L wide-mouth container** once the product is chosen (panel, PCM, holder and steel plate add mass) | closed-cell EVA sheet | ~1 |
| Battery (optional 2nd) | ★**matched + isolation diode/balancing**, belly placement; else one larger protected cell | matched protected 18650 | 8–11 |

Housing for this kit is a **1.5 L wide-mouth screw-lid container** (lid + gasket, a single leak path; product not yet chosen). A cut-and-resealed PET bottle is for demos and short runs only; a 1 L bottle is too small for the 84×112 mm panel. Everything else is identical to Tier 1. Keep the battery sized to survive the planned dwell on its own; solar is margin, not a design input, until bench G5 measures real harvest.

## Tier 2 — Reusable Robust (upgrade, USD 90–130)

Adds on top of Tier 1: bolt-sealed IP-rated opaque enclosure with gasket, IP-rated cable glands, external waterproof antennas via bulkhead, temperature-protected charging (NTC/JEITA) for cold water, bulk capacitor plus supercap for the modem's ~2 A transmit peak, printed internal frame, cell-failure containment. Use only for repeated, cold, or harsh campaigns. Power sizing and the reliability rationale for these additions: [ELECTRONICS_POWER.md](ELECTRONICS_POWER.md) and [RELIABILITY.md](RELIABILITY.md). A field-validated, fully costed robust build is pending first deployment — this repo publishes the reference design, not a proven 50-unit run.

## Notes (all cellular tiers)

- The board's GNSS is a separate L76K chip read over its own UART (NMEA via TinyGPSPlus), not the modem's AT GNSS.
- Firmware library must be lewisxhe/TinyGSM-fork; stock TinyGSM lacks the A7670 macro.
- Always test with the battery installed. USB-only power browns out on the transmit peak and looks like a modem fault.
- Housing must be non-metallic and the antenna must stay above the waterline, or GPS and cellular both die.
- Power budget (ping interval vs 18650 endurance), mission device-classes (river / estuary / open-ocean), and the solar option: see [ELECTRONICS_POWER.md](ELECTRONICS_POWER.md). Battery-only + recover-to-recharge is the default. The solar panel is now **standard in the participant kit** ([Tier 1.5](TIER1_5_SOLAR_DRIFTER.md)). **Tier 3 = Iridium satellite**, research only, not adopted before a KC radio-certification check.
