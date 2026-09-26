# SEA:CUT / Friendly Floaty — Global Onboarding & Regional Selection

An open-source, non-profit river-to-sea litter drifter: a low-cost GPS + cellular float you release into a river, track from anywhere, and **recover**. It maps how floating trash travels from source to sea so communities know where to place booms and cleanups.

This is a movement, not a product. In the spirit of Precious Plastic, we publish the design so anyone, anywhere can build one from locally available parts, adapt it, and share the data. We do not ship certified finished units. You build, you own your deployment, you follow your local rules. What keeps that trustworthy is not the absence of caveats but a few strong, universal ones (Section 1).

**This page is the international companion to the canonical build docs — it does not restate them.** Read it for the ethic, how it works, and how to pick a radio/SIM/site for *your* country. For the actual parts list, the data format, and the field procedure, follow the single sources of truth:

| You want… | Canonical source (do not duplicate) |
|---|---|
| Parts list by tier (with local substitutions) | [../hardware/BOM.md](../hardware/BOM.md) |
| Data format (FF-ID v1.1) | [../data/schema.md](../data/schema.md) |
| Reference firmware | [../firmware/drifter_a7670_cat1/](../firmware/drifter_a7670_cat1/) |
| Ingest server | [../server/README.md](../server/README.md) |
| Float physics & hard gates | [../hardware/FLOAT_STANDARD.md](../hardware/FLOAT_STANDARD.md) |
| Build variance across countries | [DIY_DIVERSITY.md](DIY_DIVERSITY.md) |
| Deploy responsibly (localize) | [DEPLOY_responsibly.md](DEPLOY_responsibly.md) |

Licenses: firmware **MIT** · hardware design **CERN-OHL-S** (strongly reciprocal, see [../LICENSE-hardware.md](../LICENSE-hardware.md)) · documentation **CC BY-SA 4.0** · observation **data CC BY 4.0** (permissive, so pooled citizen-science data combines and cites freely). See [../LICENSE-docs.md](../LICENSE-docs.md).

---

## 1. The four rules that travel with the design

Precious Plastic did not spread by ignoring safety. It spread because it paired open designs with a handful of simple, non-negotiable rules (ventilate, never burn) and pushed everything else to the local operator. These are ours. They are values, not regulations, and they are what make the open-source spread credible instead of reckless.

1. **Recover what you release.** This is a marine-litter project. A device left in the water is the exact problem we fight. Design every unit for recovery (high visibility, contact label, a grab loop), track it, and go get it. If you cannot recover it, do not release it. Recovery is not a rule imposed on us — it is our identity.
2. **Safe power, no toxics.** Use a protected Li-ion cell. Do not charge below 0 °C or let the enclosure cook above 45 °C. Contain a single-cell failure. No burning, no toxic materials, non-metallic RF-transparent housing. Account for the device's own footprint.
3. **Do no harm to the water or its users.** Smooth shapes that will not entangle wildlife or snag nets and propellers. Avoid protected or sensitive areas and breeding seasons unless locally cleared. Tell nearby water users you are there.
4. **Own your deployment, follow your local rules.** Releasing into shared water and transmitting on cellular or radio is regulated differently in every country. We give you the design; the deployment is yours. Start tiny (a few units), learn, then scale. Section 5 is a generic localize checklist, with Korea as one worked example.

Deployments involving under-18 participants carry extra, mandatory rules — see [../SAFETY_MINORS.md](../SAFETY_MINORS.md).

---

## 2. How it works

Wake on a timer, get a GPS fix, POST one small JSON ping over the cellular network, sleep. The device sends a single coordinate; a server assembles the track. No user-built gateway or infrastructure is required — the mobile network is the infrastructure.

```
[drifter] --GPS fix--> --cellular POST--> [ingest server] --> [public map / open data]
   sleep <------------------------------------ 200 OK
```

The ping and track format is **FF-ID v1.1**, defined once in **[../data/schema.md](../data/schema.md)** (with `obs_id`, provenance, QC, and physical covariates). Do not hand-copy the JSON here — a duplicated schema drifts out of sync, which is the exact interoperability failure the standard exists to prevent. Run your own ingest server ([../server/README.md](../server/README.md)) or POST to a shared community endpoint. Data belongs to the community under **CC BY**.

---

## 3. Choose your tier (concept)

Pick the lowest tier that answers your question. A river survey with a handful of floats does not need the robust tier. Start cheap, prove the pipeline, then scale.

- **Tier 0 — Phone-in-a-Bottle** (~USD 5–35): a reused smartphone + a GPS-logging app in a sealed bottle. Proves the whole chain (device → server → map) for almost nothing, anywhere, from e-waste. Use it first.
- **Tier 1 — Cellular Bottle** (~USD 50–70, **the movement default**): an ESP32 cellular board inside an upcycled PET bottle. The most diffusible, the cheapest that still runs unattended, and it is literally the litter it studies.
- **Tier 1.5 — Solar-assisted long-dwell** (Tier 1 + a 6 V flat panel on the container lid; updated 2026-09-26, the wrap-around thin-film cell is no longer used): for long-dwell monitoring where recovery may take weeks — see [../hardware/TIER1_5_SOLAR_DRIFTER.md](../hardware/TIER1_5_SOLAR_DRIFTER.md).
- **Tier 2 — Reusable Robust** (~USD 90–130): bolt-sealed IP-rated case, external antennas, temperature-protected charging, self-righting keel. For repeated, cold, or harsh campaigns — not the starting point.

**The full parts list for every tier, with per-function local substitutions, lives in [../hardware/BOM.md](../hardware/BOM.md).** That is the one table to keep current; this page intentionally does not copy it.

Three notes that apply to all cellular tiers (and are enforced in firmware and CAD):
- The reference board's GNSS is a **separate L76K chip** read over its own UART (NMEA via TinyGPSPlus), not the modem's AT GNSS. That separation is the whole point — GPS runs with the modem asleep, which is what makes store-and-forward battery life possible. The firmware library must be **lewisxhe/TinyGSM-fork** (stock TinyGSM lacks the A7670 macro).
- **Always test with the battery installed.** USB-only power browns out on the modem's transmit peak and looks like a modem fault.
- **Housing must be non-metallic and the antenna must stay above the waterline**, or GPS and cellular both die. (The solar variant adds a hard gate: a metal-substrate cell over the RF window is rejected in CAD — see FLOAT_STANDARD.)

---

## 4. Communications: pick your backhaul by region

Cellular is the simplest global option because the mobile network is already everywhere people are. Two things are regional: the LTE bands and the SIM.

Cellular board variant by region (A7670 family — pick for your bands):

| Variant | Typical LTE-FDD bands | Fits |
|---|---|---|
| A7670E | B1/B3/B5/B7/B8/B20 | Europe, Middle East, Africa, Korea, most of Asia, Australia |
| **A7670G** | global multiband (adds Americas bands, e.g. B2/B4/B12/B25/B66) | Americas and worldwide roaming — **the reference board** |
| A7670SA | Latin America bands | South/Central America |

The reference build uses the **T-A7670G R2** (global variant) to maximize band coverage. Match the variant to your carrier's bands; do not assume one variant is region-locked without checking the band list. **Do not** substitute the SIM7080G (LTE-M / NB-IoT) — it has no verified attach on Korean commercial networks.

SIM options, easiest first:
- **Global IoT SIM** (1NCE, Soracom): one SIM works across many countries; ideal for a hand-built device because it roams and sidesteps local device-registration. Confirm which local carriers it roams onto in your country (e.g. in Korea it roams on KT/SKT, not LG U+). Soracom only matters where its partner carriers have coverage — in a single-country pilot a local prepaid SIM is often simpler and cheaper.
- **Local prepaid data SIM**: cheapest per-country; confirm a hand-built (non-certified) device is allowed to attach to data. (Korea: a voice-inclusive plan on a domestic SIM, activated in a phone first; data-only plans can refuse unregistered devices.)

**No cellular coverage** (remote/rural rivers)? The reference firmware is cellular-only; LoRa is a *documented alternative backhaul, not part of the reference build*. Meshtastic runs license-exempt ISM bands (868 MHz EU/Africa · 915 MHz Americas/Australia · 923 MHz most of Asia · KR920 920–923 MHz Korea) but needs one or two community gateways with line of sight. It is out of scope for this repo's firmware; follow the upstream Meshtastic project docs if you take that path.

### Cost — why an integrated board, and what satellite really costs

Two questions builders always ask. Prices are single-unit import retail (vary by country, quantity, duty, shipping).

**"Three separate parts must be cheaper than the integrated board?"** Barely — and not once it is in water:

| Approach | Part | ~USD |
|---|---|---|
| Separate | ESP32 dev board | 1–5 |
| | A7670E LTE Cat.1 module (breakout) | 14–17 |
| | GNSS module (NEO-6M / ATGM336H class) | 3–5 |
| | **subtotal** | **~21–26** |
| Integrated | LILYGO T-A7670G R2 (with GNSS) | **~27** (manufacturer store) |

The **LTE modem (~$15) is the cost floor**, not the ESP32 (~$4) — you buy the modem either way, so avoiding integration saves little. The separate route's ~$3–5 paper saving is eaten by: 3× the solder joints and connectors (each a leak/failure point in a sealed water device), 2–3 antennas sourced separately, no built-in SIM slot / power path / UART level-shifting / USB programming, more volume and mass (worse for the bottle and for self-righting), and firmware pin-maps written for the reference board. Integration is cheaper *per reliable unit*. Real savings appear only at hundreds of units on your own PCB — at which point you have re-built the integrated board yourself. (Korea: you cannot dodge the modem cost with a cheaper LTE-M/NB-IoT part — the SIM7080G has no verified attach on Korean networks.) The manufacturer's own storefront (e.g. the maker's official AliExpress store, factory-direct from Shenzhen) is typically the cheapest source; resellers add margin and local-warehouse convenience.

**"How expensive is satellite (Class 3 — open ocean beyond cellular)?"** Roughly 10× the hardware plus recurring airtime:

| Satellite path | Hardware | Airtime | Note |
|---|---|---|---|
| Myriota / Astrocast | module ~$20–40 | low subscription | **store-and-forward, not real-time** (data on satellite pass, minutes–hours late). Fine for sparse telemetry, not live tracking |
| Swarm M138 | ~$150 | ~$60/yr | was the cheap two-way, but **new-device sales halted 2023** (SpaceX) — not a path for new builds |
| **Iridium RockBLOCK 9603** | **~$250–270** | line rental **~$15/mo** + per-message credits (1 credit / 50 bytes) | **truly global incl. open ocean, real-time** SBD — the standard for ocean drifters |

Against a ~$27 cellular unit, real-time open-ocean tracking (Iridium) is **~10× the hardware plus a recurring bill**. This is exactly why the reference design targets river→coastal water inside cellular coverage and separates satellite as Class 3: cellular goes silent offshore because base stations are on land — a physics limit no cheaper board fixes, only a satellite one, at the cost above.

### Sourcing — what to import, what to buy locally

Only two parts are spec-critical to source. Everything else is generic hardware you already have locally.

- **Cellular board.** Check a local distributor first — faster, no customs, local warranty, and the seller has already cleared type-approval. If it is not sold locally, the manufacturer's own official store (e.g. their factory-direct AliExpress store) is usually the cheapest source. Express couriers (DHL/FedEx) are fast but the shipping can equal the board on a single-unit order and may add a customs-brokerage fee even under your country's duty-free threshold; standard/postal is slower but often cheaper landed. A hand-built cellular device is regulated somewhere — see [DEPLOY_responsibly.md](DEPLOY_responsibly.md); buying a locally-stocked board sidesteps the personal-import approval question.
- **Solar (updated 2026-09-26).** The current design is a **flat 6 V panel** (reference SZH-SPB017, 84×112 mm, 220 mA) outside on the container lid, into the board's JST P1 (SOLAR_IN) and on-board CN3065, with a **5.6 V 5 W zener (1N5339B)** across the leads because the CN3065 absolute maximum is 6.5 V. The thin-film notes below are kept for builders who still use film.
- **Solar film (older wrap design) — the one part where a cheap local panel can silently break the design.** "Flexible thin-film" is a *form factor*, not a material, and the material decides RF and toxicity:
  - **a-Si (amorphous silicon), ETFE/plastic front — recommended.** Non-metal → RF-transparent (B5), non-toxic, cheap (small 5–6 V panels ~$5–20). The drifter's low power budget does not need high efficiency, so this is the rare case where the cheapest option is also the correct one.
  - **CIGS** — higher efficiency, but the common flexible CIGS is on a **stainless-steel** substrate (industry-standard, since CIGS needs >500 °C processing) = a metal RF shield over the antenna = disqualified. Only Cd-free CIGS on **polyimide** (non-metal) is acceptable, and it is less common and more expensive. Do not pay more for CIGS here.
  - **CdTe / perovskite — no** (toxicity / immaturity / RF).
  - Rule regardless of source: **verify the substrate is non-metal.** A steel-backed panel kills the radio even if everything else is perfect.
- **Everything else is local:** 18650 cell with a protection circuit (Korean kit: unprotected Samsung 30Q in an external holder + PCM, 3.7 V 1-cell, continuous ≥3 A), SIM, uFL antennas, the PET bottle, ballast (a fishing sinker), marine sealant, bright tape / QR label / recovery loop, and the printed inserts (print them yourself). None need importing.

---

## 5. Deploy responsibly — localize (generic, with one worked example)

We give you the design. The release is yours. Before you put units in the water, walk this generic checklist and apply your own country's rules. This is the "attach the caveats" step that lets an open-source project spread without becoming reckless. It is a habit, not a wall. The full version, with hard gates, is in **[DEPLOY_responsibly.md](DEPLOY_responsibly.md)** and **[DIY_DIVERSITY.md](DIY_DIVERSITY.md) §4.4**.

Generic checklist (applies everywhere):
- **Radio / telecom.** A hand-built cellular or radio transmitter is regulated somewhere. Check whether your country needs a type-approval, exemption, or license for a small research/DIY transmitter, and whether your SIM/band is allowed. Global IoT roaming SIMs often simplify this.
- **Water access.** Someone manages the water you release into (municipal river, national river, protected wetland, harbor, coastal zone). Find out who and ask. Protected/heritage/wetland areas usually need extra consent, so prefer an ordinary local stream for a first pilot.
- **Battery and e-waste.** Li-ion in water is a fire and pollution risk if lost. Protect, contain, and above all recover (Rule 1). Have a plan for cells at end of life.
- **Wildlife and other users.** Smooth, non-entangling shapes. Avoid breeding seasons and protected species areas. Tell fishers, harbor authority, and coastguard so an unknown sealed electronic box washing ashore does not become an alarm.
- **Start small and honest.** A few units, recover them all, learn recovery rate and drift behavior, then size a larger release from what you measured — not from a round number.

Worked example — Korea (one locale among many):
- **Radio (updated 2026-09-26):** one unit per model imported by an individual for personal use needs no application. The research/development exemption confirmation (RRA, up to 1,500 units) is needed only for **two or more units of the same model**, or several units in an organisation's name. Participants each buy one unit in their own name.
- **SIM:** each participant uses their **own domestic SIM in their own name**. Korea domestic route: HelloMobile Slim 500MB on the LG U+ network (NFC SIM KRW 8,800, KRW 1,700/month), voice-inclusive plan, activate in a phone first, then move it to the board. The A7670G covers all three Korean carriers' LTE bands. Global IoT roaming SIMs (KT/SKT only) remain an option.
- **Water:** the Nakdong main channel and estuary are a national river plus a natural-monument/wetland protected zone (national-level consent). An ordinary tributary stream (managed by the city/district) is the low-friction pilot site and is still genuinely "source to sea" because the tributary flows to the estuary.
- The rest (battery safety, recovery-first, wildlife, notify water users) is the same universal set above.

Other countries: replace the two locale-specific lines (radio authority, water manager) with your own, keep everything else. A second worked example — warm, monsoon-driven, high-UV — is [DEPLOY_tropical.md](DEPLOY_tropical.md) (Da Nang, Vietnam). If you build one, add your locale's notes to [DIY_DIVERSITY.md](DIY_DIVERSITY.md) so the next person in your country starts faster. That is how the movement compounds.

---

## 6. Join, contribute, share data

- Build a unit, release and recover it in your river, and POST FF-ID trajectories to your own ingest server or the shared map.
- Register your variant and add your river via the issue templates (`.github/ISSUE_TEMPLATE/`); add your locale notes to [DIY_DIVERSITY.md](DIY_DIVERSITY.md).
- Improve the design: better self-righting bottle geometry, cheaper regional part swaps, translations. Open a pull request — CI parses the CAD and checks the schema/links so your change stays interoperable.
- Data is **CC BY**. Attribution: *SEA:CUT / Friendly Floaty — 사단법인 이타서울 / GAA (Global Adopt-a-Beach Alliance) and the contributor.* See [../CITATION.cff](../CITATION.cff).

Built by 사단법인 이타서울 / GAA and contributors. Inspired by the open-hardware movement, including Precious Plastic and OpenMetBuoy. This is a **reference design** — field validation and a DOI-deposited dataset are pending; all figures are estimates until bench + field measurement.
