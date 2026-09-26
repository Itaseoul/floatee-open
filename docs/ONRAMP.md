# On-ramp — from zero to your first drifter

> The shortest honest path into SEA:CUT. Pick where you start, follow the numbered steps, keep the
> four rules. Every step links to the canonical doc — this page only puts them in order.
> Kernel/shell rationale and the full tier table: [DIY_DIVERSITY.md](DIY_DIVERSITY.md). 한국어 요약은 맨 아래.

## 1. Pick your on-ramp (30 seconds)

Lower tiers are always open — start where your budget and skills are, move up later.

| You are | Start at | Cost (USD) | Solder? | First goal |
|---|---|---|---|---|
| School / citizen / low-resource | **Tier 0** phone-in-a-bottle | 5–35 | none | prove the pipeline with 0 lines of code |
| Maker beginner | **Tier 1** cellular bottle (default) | 40–80 | none (JST/uFL/USB) | the default upcycled float |
| Estuary / long-dwell | **Tier 1.5** solar-assisted (6 V flat panel on the lid) | 65–100 | yes (PCM wires, zener) | extend reporting life |
| City / repeated campaigns | **Tier 2** robust | 90–180 | none–little | durable reusable unit |
| Research / open sea | **Tier 3** = Iridium satellite (research only, not adopted before a KC check) | 180+ | yes | beyond cellular coverage |

Update 2026-09-26: the Korean participant build is **one kit** (about KRW 92,908) with the flat solar panel included, and a **deployment build needs soldering** (PCM wires to the external holder plates, zener across the solar JST leads). Only a bare bench test runs solder-free.

Full tier table and parts: [../README.md](../README.md) · [../hardware/BOM.md](../hardware/BOM.md).

## 2. The pipeline you are building

```
[drifter] --GPS fix--> --cellular (LTE) POST--> [ingest] --> [public map / open data]
```

One coordinate per wake; a server assembles the track. Cellular (LTE), so coverage is river,
estuary, and near-coast — **not** open sea (that needs satellite backhaul, out of scope here).

## 3. Build order (Tier 1, the default)

1. **Parts** — order from [../hardware/BOM.md](../hardware/BOM.md). Reference board is the LILYGO T-A7670G R2; pick the A7670 band variant for your region.
2. **A float that survives first** — meet buoyancy F1/F2 and non-toxic power *before* anything else: [../hardware/FLOAT_STANDARD.md](../hardware/FLOAT_STANDARD.md).
3. **Three printed inserts** — electronics bracket, ballast keel, recovery loop. Measure your bottle, edit one `.scad`: [../hardware/cad/README.md](../hardware/cad/README.md).
4. **Firmware** — flash ESP32 + A7670 + L76K, one ping per wake: [../firmware/](../firmware/).
5. **Payload** — POST the minimal JSON per schema (FF-ID v1.1): [../data/schema.md](../data/schema.md).
6. **Before you release** — clear the recovery gate (plan, owner, deadline, cross-check evidence): [../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md](../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md).
7. **Deploy responsibly** — your local rules, protected areas/seasons, tell nearby water users: [DEPLOY_responsibly.md](DEPLOY_responsibly.md).

**Fastest possible test:** Tier 0 skips steps 3–4 entirely (a reused phone + GPSLogger, tethered,
Class-A only) — the quickest way to exercise the whole pipeline with zero code.

**Staying out for weeks?** Add solar without a new printed part: [../hardware/TIER1_5_SOLAR_DRIFTER.md](../hardware/TIER1_5_SOLAR_DRIFTER.md);
power detail in [../hardware/ELECTRONICS_POWER.md](../hardware/ELECTRONICS_POWER.md). ★Solar extends power, not coverage.

## 4. The four rules (non-negotiable — they travel with the design)

1. **Recover what you release.** A device left in the water is the very problem we fight.
2. **Safe power, no toxics.** Protected Li-ion, no primary lithium, contain a cell failure, non-metallic housing.
3. **Do no harm to the water or its users.** Smooth non-entangling shapes; avoid protected areas and seasons.
4. **Own your deployment, follow local rules.** Wireless type-approval, SIM legality, water sign-off, and drone rules are hard gates.

One-line kernel test: if a change (1) breaks data pooling, (2) makes recovery hard or impossible,
(3) stops it floating / self-righting or submerges the antenna or endangers the cell, or (4) inflates
the claim — you touched the **kernel**, not the shell. Everything else is yours to adapt.

## 5. From blueprint to a validated project

This repo is a reference design; every buoyancy, power, and drift number is an **estimate until bench
and field measurement**. The shortest honest path — bench gates, one small release, a citable DOI
deposit — is [VALIDATION_CAMPAIGN.md](VALIDATION_CAMPAIGN.md).

## 6. Contribute

Build a unit, release and recover it in your river, POST trajectories, add your river and your locale
notes. Fork, improve, translate, open a pull request.

---

## 한국어 요약

처음 오는 사람을 위한 진입 순서입니다. **아래 티어로 내려갈 자유는 늘 열려 있으니**, 예산·기술에 맞는 곳에서
시작해 나중에 올라가면 됩니다.

- **Tier 0**(폰인병, 5~35달러, 코드 0줄) = 파이프라인을 가장 빨리 시험 · **Tier 1**(기본 업사이클 병, 40~80달러, 무납땜) = 표준 빌드.
- 빌드 순서: 부품([../hardware/BOM.md](../hardware/BOM.md)) → **부력 F1/F2·안전 전원 먼저 통과**([../hardware/FLOAT_STANDARD.md](../hardware/FLOAT_STANDARD.md)) → 3부품 프린트([../hardware/cad/README.md](../hardware/cad/README.md)) → 펌웨어([../firmware/](../firmware/)) → 스키마대로 POST([../data/schema.md](../data/schema.md)) → 방류 전 회수 게이트([../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md](../hardware/PRELAUNCH_RECOVERY_CHECKLIST.md)) → 책임 배포([DEPLOY_responsibly.md](DEPLOY_responsibly.md)).
- 장기 잔류는 솔라 보조([../hardware/TIER1_5_SOLAR_DRIFTER.md](../hardware/TIER1_5_SOLAR_DRIFTER.md), 솔라=전력↑이지 커버리지 아님).
- **네 규칙**(회수·안전전원·무해·현지규정 준수)은 설계와 함께 이동하는 커널입니다. 판별 한 줄과 티어 상세는 [DIY_DIVERSITY.md](DIY_DIVERSITY.md).
- 설계도 → 검증된 프로젝트 경로: [VALIDATION_CAMPAIGN.md](VALIDATION_CAMPAIGN.md).
