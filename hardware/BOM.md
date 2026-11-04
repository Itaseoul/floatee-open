# Bill of Materials (BOM)

Parts by function and generic spec first, then a globally shipping reference part, then local substitution. Prices are rough retail USD and vary by country. Full context and regional tables: [../docs/OPEN_HARDWARE_BOM_global.md](../docs/OPEN_HARDWARE_BOM_global.md).

Reference board is the LILYGO T-A7670G R2 with onboard L76K GNSS. It is cellular (LTE Cat.1 bis) plus a separate GPS chip. It is not satellite. Do not substitute the SIM7080G (LTE-M/NB-IoT), which has no verified attach on Korean commercial networks.

## Current build: PET-bottle v4 (2026-11-04)

The current default. Status: testing, water test not done yet. Parts about **KRW 100,000 per unit** (2026-09 prices), SIM fees separate. Korean list with shops: [https://floatee.caresea.kr/welcome/parts](https://floatee.caresea.kr/welcome/parts). It is DIY: the body can change as long as the build meets the [conditions](https://floatee.caresea.kr/manual#diy).

| Group | Part | Spec | Qty |
|---|---|---|---|
| Electronics | LILYGO T-A7670G R2 (External GPS) | ESP32-WROVER-E, A7670G, L76K; GPS patch and LTE antenna included | 1 |
| Electronics | Protected 18650 cell | button top, 18.5×69 mm, protection circuit built in | 1 |
| Electronics | Solar panel SZH-SPB017 | 6 V 220 mA, 84×112 mm | 1 |
| Electronics | Zener onsemi 1N5339BG | 5.6 V 5 W, panel input protection | 1 |
| SIM | Nano SIM | ordinary voice and data plan, any carrier | 1 |
| Body | Round carbonated-drink PET bottle | 1.5 L with cap, not a water bottle; or a household sealed container | 1 |
| Body | PVC pipe 75A VG2 ring | 89 mm outside diameter, cut to 30 mm | 1 |
| Body | SUS316 hose clamps | 80–100 mm | 2 |
| Body | Self-fusing silicone tape | 24 mm × 3.2 m | 2 rolls |
| Waterproofing | Vacuum bag roll | 20 cm wide, two layers | about 0.6 m |
| Waterproofing | Silica gel, humidity card | 30 g; mixed card | 2; 2 |
| Buoyancy | Closed-cell PE foam | 20 mm thick; floats, cradle, stops, strips | about 490 cm³ |
| Weight | Rolled steel flat bars | 54×40×12 mm, about 407 g together | 2 |
| Weight | M12 stainless flat washers | for trim | 0–5 |
| Winter charge protection | NTC 10K 3950 probe, chip resistor 24.9 kΩ 1% | see the manual before winter use | 1; 1 |
| Fixing, marking | Cable ties, contact labels, yellow reflective tape | 100 mm; 28×65 mm; 40 mm wide | 2; 4; about 0.6 m |
| Recovery | Zinc-plated steel M12 washer | inside the cap, for a magnet | 1 |

**Everything below is archived.** The tiers, the wide-mouth container kit and the printed inserts are earlier drafts kept for reference.

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
| Battery | 18650 cell + **protection circuit**, verify holder length | **Default (2026-10-30): a protected cell** (protection circuit built in; v4 reference 18.5×69 mm, button top). Wide-mouth branch alternative: **Samsung 30Q unprotected** in an **external 1-cell holder**, wired through a **PCM (protection module: cuts over-discharge, over-charge and short circuit; 3.7 V 1-cell, continuous ≥3 A)** to the board's rear holder terminals; **leave the board's own holder empty** (protected cells are 69–70 mm long and do not fit the 65 mm holder) | any protected 18650, or any unprotected 18650 + 1-cell PCM rated ≥3 A continuous | 5–8 |
| SIM | local data SIM (Korea) or global IoT roaming SIM (elsewhere) | **Korea: participant's own domestic SIM in their own name, any carrier** (KT, SKT, LG U+ and their MVNOs; the A7670G covers all three carriers' LTE bands, no carrier confirmed on this board yet). Choose a **voice-inclusive plan** (data-only plans can refuse unregistered devices) and **activate it in a phone first**, then move it to the board | global IoT roaming SIM (1NCE / Soracom) outside Korea | 5–14 |
| Antennas | LTE + GNSS, kept above water | board-included uFL antennas (L1 only; a dual-band GNSS option needs an L1+L5 antenna, see GNSS options below) | extend outside if sealing | 0–8 |
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

## GNSS options — position accuracy (added 2026-11-03, sources checked 2026-11-03, comparison not yet run)

The default L76K is single-frequency (L1). Unit 1 measured it on 2026-11-03: on an open rooftop six resting fixes fell within **5–9 m** of their median (20–25 satellites, firmware v1.4.1); indoors and at a window the spread was 30–240 m (reflections from walls). For small streams (2–30 m wide) that cannot tell the banks apart. Dual-frequency (L1+L5) receivers reject reflections much better. The options below replace **only the L76K**; the LILYGO board, modem and everything else stay the same.

| Option | Module | Bands | Module current (tracking) | Open-sky accuracy (maker, CEP 50%) | I/O voltage | Default UART | Status |
|---|---|---|---|---|---|---|---|
| A (default) | Quectel **L76K** | L1 | ~29–31 mA (sibling L76-L sheet; L76K sheet not checked) | a few m; **measured 5–9 m** (unit 1, rooftop) | 3.3 V | (as wired today) | in kit, water test pending |
| B | u-blox **NEO-F10N** | GPS L1+L5, Galileo E1+E5a, BeiDou B1C+B2a | 21 mA tracking, 26 mA acquisition (3.0 V, default) | 1 m multi-GNSS, 1.5 m GPS only | follows VCC 3.0–3.6 V, **fits ESP32 3.3 V** | **38400** 8N1 | **first to compare** |
| C | Quectel **LC29H (AA)** | L1+L5 | 24 mA (3.3 V) | 1 m | **2.8 V I/O** (forum; hardware design not checked) — **needs level shifting** on ESP32 TX → module RX | 115200 | candidate, needs adapter |
| D (later) | Quectel **LC29H (DA / EA)** RTK | L1+L5 + RTK | 30 mA | 1 m alone; 1 cm + 1 ppm with corrections | 2.8 V I/O | DA 115200, EA 460800 | research only |

Notes on the options:

- **NEO-F10N does not use GPS L5 by default** (it is still pre-operational); Galileo E5a and BeiDou B2a are used by default. Enabling GPS L5 needs a configuration change (Integration manual). Cold start 28 s, hot start 2 s, start-up current up to 100 mA. Datasheet UBX-23002117 R05.
- **LC29H variants:** AA (dual-band, no RTK), BA (RTK + dead reckoning), CA (dead reckoning), DA (RTK 1 Hz), EA (RTK 1–10 Hz), AI (no GPS L5, NavIC L5 only, 1.8 m), BS (base station only, heading to end of life). Quectel product page.
- **Other:** Allystar TAU1201 is L1+L5 (1 m CEP) but no 3.3 V carrier board was found; Quectel LC76G is **L1 only** and is not an option.

Orderable boards and antennas (prices seen 2026-11-03, single unit, change often):

| Item | Price | Where | Power / connector | Notes |
|---|---|---|---|---|
| SparkFun GNSS L1/L5 Breakout NEO-F10N, SMA (GPS-24114) | 41.50 USD; Korea 82,400 KRW (DeviceMart, overseas order ~1 week) | sparkfun.com/products/24114, devicemart.co.kr | 5 V or 3.3 V in (AP2112K), 3.3 V logic; SMA | 50.8 × 38.1 mm; USB-C (CH340C); **cut the USB-TX/RX jumpers** to use the MCU UART; RTC backup battery |
| Waveshare LC29H(AA) GPS HAT (SKU 25278) | 39.99–54.99 USD | waveshare.com | **5 V input only**, module on a 2.8 V regulator; IPEX + IPEX-SMA cable, L1+L5 antenna included | Raspberry Pi HAT; header UART level shifting not documented — **measure before wiring to ESP32** |
| Quectel LC29H(DA) bare module | 32,000 KRW (DeviceMart) | devicemart.co.kr | 2.8 V I/O | needs your own carrier board |
| Waveshare GNSS L1+L5 active antenna (SKU 25346) | 16.99 USD | waveshare.com | SMA, 3 m RG174; 3.0–18 V, ≤20 mA | 50 × 50 × 19.1 mm |
| u-blox ANN-MB5 (SparkFun GPS-25071) | 42.50 USD; Korea 72,500 KRW | sparkfun.com, devicemart.co.kr | SMA, 3 m; 3.0–5.0 V, ~17 mA (5 V) | 49.1 × 46.1 × 15.9 mm |

What changes when swapping A → B (or C):

- **Antenna:** must be an **L1+L5 active antenna**; the board-included GNSS antenna is L1 only. The antenna adds **~17–20 mA** while the GNSS is on.
- **Power:** module plus antenna is roughly 40 mA while tracking, somewhat more than the L76K. The LTE modem's transmit still dominates the battery budget, but redo the power calculation in [ELECTRONICS_POWER.md](ELECTRONICS_POWER.md) before a long deployment.
- **Wiring:** the same four wires (3.3 V, GND, TX, RX) for **B**; for **C** add a level shifter or divider on the ESP32 TX line. The power-gating rule is in [GPS_MODULE_WIRING.md](GPS_MODULE_WIRING.md). Check the module's 3.3 V rail can supply the 100 mA start-up peak.
- **Firmware:** NMEA parsing (TinyGPSPlus) is unchanged; the v1.4 satellite list reads GSV and works with any NMEA receiver. Skip the L76K-only `$PCAS` commands, set `GPS_BAUDRATE` (38400 for B, 115200 for C), and for B consider enabling GPS L5 with UBX-CFG-VALSET (keys to be confirmed from the Integration manual).
- **Antenna power on B:** VCC_RF is VCC − 0.1 V (≈3.2 V on 3.3 V), close to the ANN-MB5's 3.0 V minimum; check the SparkFun schematic.

RTK note (option D, research only): the National Geographic Information Institute (NGII) announced free internet RTK corrections on its platform (2024-02-26; RTS1 VRS and RTS2 FKP, port 2101); vendor sources say login became mandatory on 2025-08-01 (official notice not checked). Whether LC29H(DA/EA) accepts the NGII stream is **not confirmed** (forum: it takes RTCM3 1005/1006 and MSM4/MSM7, not 103x).

Review before buying (2026-11-03, engineering fit and public user reports; detail in the project's internal review): buy B with the **u-blox ANN-MB5** for the bench comparison (net gain inside the F10N's default LNA range, cellular-band rejection stated, IPX7); the Waveshare antenna's ~28 dB LNA needs the F10N low-gain LNA mode and one independent comparison found a same-price Waveshare L1/L5 pairing noticeably weaker. Neither 50 mm antenna fits the PET-bottle GNSS slot (44 mm): an in-bottle antenna must be a 25–35 mm L1/L5 active patch rated down to 2.7 V. Before wiring B to a drifter board, measure (1) the V3V rail rise time against the NEO-F10N's 25 µs/V minimum ramp (absolute maximum rating), (2) antenna current in software standby, (3) antenna voltage at the SMA (~3.0 V after VCC_RF and the 10 Ω feed resistor). Try configuration in RAM first; a wrong baud rate saved to flash has bricked boards.

Decision rule (proposed): run B next to unit 1 at the same open spot for 20 minutes. If its resting fixes stay within ~2 m, make B (or a cheaper carrier board for the same module) a **kit option for narrow streams**; keep A as the default for wide rivers and cost-sensitive teams.

## Tier 2 — Reusable Robust (upgrade, USD 90–130)

Adds on top of Tier 1: bolt-sealed IP-rated opaque enclosure with gasket, IP-rated cable glands, external waterproof antennas via bulkhead, temperature-protected charging (NTC/JEITA) for cold water, bulk capacitor plus supercap for the modem's ~2 A transmit peak, printed internal frame, cell-failure containment. Use only for repeated, cold, or harsh campaigns. Power sizing and the reliability rationale for these additions: [ELECTRONICS_POWER.md](ELECTRONICS_POWER.md) and [RELIABILITY.md](RELIABILITY.md). A field-validated, fully costed robust build is pending first deployment — this repo publishes the reference design, not a proven 50-unit run.

## Notes (all cellular tiers)

- The board's GNSS is a separate L76K chip read over its own UART (NMEA via TinyGPSPlus), not the modem's AT GNSS.
- Firmware library must be lewisxhe/TinyGSM-fork; stock TinyGSM lacks the A7670 macro.
- Always test with the battery installed. USB-only power browns out on the transmit peak and looks like a modem fault.
- Housing must be non-metallic and the antenna must stay above the waterline, or GPS and cellular both die.
- Power budget (ping interval vs 18650 endurance), mission device-classes (river / estuary / open-ocean), and the solar option: see [ELECTRONICS_POWER.md](ELECTRONICS_POWER.md). Battery-only + recover-to-recharge is the default. The solar panel is now **standard in the participant kit** ([Tier 1.5](TIER1_5_SOLAR_DRIFTER.md)). **Tier 3 = Iridium satellite**, research only, not adopted before a KC radio-certification check.
