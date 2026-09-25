# Floatee Open — river-to-sea drifter (SEA:CUT)

> Renamed from `Itaseoul/seacut` on 2026-09-26. Old links redirect here. **Floatee** is the public name; **SEA:CUT** remains the name of the underlying technology.

Open-source, non-profit river-to-sea litter drifter. A low-cost GPS + cellular float you release into a river, track from anywhere, and recover. It maps how floating trash travels from source to sea, so communities know where to place booms and cleanups.

Built by 사단법인 이타서울 / GAA and contributors. Inspired by the open-hardware movement (Precious Plastic, RepRap, OpenMetBuoy).

License: firmware MIT · hardware CERN-OHL-S (reciprocal) · docs CC BY-SA 4.0 (ShareAlike) · **data CC BY 4.0** (permissive, so pooled observations combine and cite freely). Design/doc improvements stay open; data stays freely reusable. See [LICENSE-docs.md](LICENSE-docs.md). OSHWA certification: planned.

한국어는 아래 [한국어](#한국어) 참조.

---

## What it does

Wake on a timer, get a GPS fix, POST one small JSON ping over the cellular network, sleep. The device sends a single coordinate; a server assembles the track. No user-built gateway is required, the mobile network is the infrastructure.

```
[drifter] --GPS fix--> --cellular (LTE) POST--> [ingest server] --> [public map / open data]
   sleep <-------------------------------------------- 200 OK
```

This is cellular (LTE), not satellite. Coverage is river, estuary, and near-coast where mobile networks reach. Open-sea tracking would need a separate satellite backhaul and is out of scope for this build.

### Where this fits (and what it is not)

This is the **upstream measurement layer**, not an interceptor. It does not remove trash and makes no ocean-scale claim. Big open-ocean interception projects (e.g. The Ocean Cleanup) act at the mouth and in the gyres; this drifter works **before** that — at the sub-river, source side — to answer *where does floating litter actually concentrate, and where should a boom or a cleanup go?* It is complementary to those projects, not a replacement for them. Its output is an **effort/path signal and a citable trajectory**, not a pollution flux, total, or a learned-AI prediction. Treat every buoyancy, power, and drift number here as an **estimate until bench + field measurement**; this repo is a reference design, and field validation plus a DOI-deposited dataset are pending. The shortest honest path from blueprint to validated project — bench gates, one small release, a citable deposit — is [docs/VALIDATION_CAMPAIGN.md](docs/VALIDATION_CAMPAIGN.md).

## The four rules (they travel with the design)

Open-source hardware spreads on a few strong universal rules plus local adaptation, not on the absence of caveats.

1. Recover what you release. A device left in the water is the very problem we fight. Design for recovery, track it, go get it.
2. Safe power, no toxics. Protected Li-ion, no charging below 0 C or cooking above 45 C, contain a cell failure, non-metallic housing.
3. Do no harm to the water or its users. Smooth non-entangling shapes, avoid protected areas and seasons, tell nearby water users.
4. Own your deployment, follow your local rules. See [docs/DEPLOY_responsibly.md](docs/DEPLOY_responsibly.md).

## Choose your tier

| | Tier 0 Phone-in-a-Bottle | Tier 1 Cellular Bottle (default) | Tier 2 Reusable Robust |
|---|---|---|---|
| Brain | reused smartphone + GPSLogger | ESP32 cellular board | ESP32 cellular board |
| Housing | upcycled PET bottle + dry bag | upcycled PET bottle | bolt-sealed IP case |
| Approx cost | USD 5–35 | USD 50–70 | USD 90–130 |
| Best for | first pipeline test, any budget | the default upcycled float | long / cold / repeated campaigns |

Full parts list: [hardware/BOM.md](hardware/BOM.md). Reference board is the LILYGO T-A7670G R2 with onboard L76K GNSS (cellular LTE Cat.1 bis + separate GPS chip). Pick the A7670 band variant for your region.

**Tier 1.5 (solar-assisted, long-dwell estuary)** — wrap-around thin-film cells extend reporting life for units that linger in-coverage for weeks; no extra printed part. ★Solar extends power, not coverage (open sea still needs satellite). See [hardware/TIER1_5_SOLAR_DRIFTER.md](hardware/TIER1_5_SOLAR_DRIFTER.md).

## 3D printed parts (parametric, OpenSCAD)

The housing is an upcycled bottle, so only three small printed inserts are needed. Sources are parametric OpenSCAD, so you measure your own bottle and edit one file.

- `hardware/cad/electronics_bracket.scad` holds the board and cell, self-locating in the bottle
- `hardware/cad/ballast_keel.scad` carries lead-free weight low for self-righting
- `hardware/cad/recovery_loop.scad` a smooth closed loop for pole/net recovery

Each ships as `.scad` (editable source) and `.stl` (print + GitHub renders it in 3D); export `.3mf` for slicers. (OpenSCAD is mesh-based, so no true STEP — see [hardware/cad/README.md](hardware/cad/README.md); the foam template exports `.dxf`.) Preview: `hardware/cad/assembly.png`.

## Firmware

[firmware/](firmware/) — ESP32 + A7670 + L76K, single ping per wake. Library lewisxhe/TinyGSM-fork + TinyGPSPlus. Default 30-minute interval.

## Data

[data/schema.md](data/schema.md) — ping and track schema (FF-ID v1.1), CC BY. Run your own ingest server or POST to the shared community map. Standards interop: [docs/DATA_STANDARDS_CROSSWALK.md](docs/DATA_STANDARDS_CROSSWALK.md) maps FF-ID onto CF-NetCDF (trajectory), OGC SensorThings, and EMODnet Physics — a drifter track is a trajectory, **not** a GBIF/Darwin Core occurrence — with a zero-dependency exporter ([data/export_standards.py](data/export_standards.py)).

## Contribute

Build a unit, release and recover it in your river, POST trajectories, add your river and your locale notes. Fork, improve, translate. Open a pull request.

---

## 한국어

하천에서 바다로 가는 쓰레기의 길목을 재는 오픈소스 비영리 드리프터. 강에 띄우고 어디서나 추적하다 회수하는 저비용 GPS + 셀룰러 부유체다. 인증받은 완제품을 배포하지 않는다. 당신이 만들고, 배포를 당신이 책임지고, 지역 규정을 따른다.

동작: 타이머로 깨어나 GPS fix, 셀룰러(LTE)로 ping 하나 POST, 잔다. 서버가 궤적을 조립한다. 게이트웨이 불필요. 이 스택은 셀룰러이지 위성이 아니다. 커버 범위는 이동통신망이 닿는 하천·하구·근연안이다.

네 가지 규칙: (1) 회수한다 (2) 안전 전원·무독성 (3) 물·이용자·야생동물 무해 (4) 네 배포는 네가 책임·지역 규정 확인. 상세 [docs/DEPLOY_responsibly.md](docs/DEPLOY_responsibly.md).

티어: Tier 0 폰+페트병(USD 5–35) / Tier 1 셀룰러+페트병(USD 50–70, 기본) / Tier 2 재사용 견고(USD 90–130). 부품표 [hardware/BOM.md](hardware/BOM.md). 기준 보드 LILYGO T-A7670G R2(온보드 L76K).

3D: 하우징이 폐페트병이라 프린트 부품은 3개(브래킷·밸러스트 킬·회수 고리)뿐. 파라메트릭 OpenSCAD라 병 규격만 바꾸면 된다. 전력·신뢰성 상세는 [hardware/ELECTRONICS_POWER.md](hardware/ELECTRONICS_POWER.md)·[hardware/RELIABILITY.md](hardware/RELIABILITY.md). 이 리포는 레퍼런스 설계이며 현장 검증(대량 방류 실증)은 준비 중이다.

전체 글로벌 가이드: [docs/OPEN_HARDWARE_BOM_global_ko.md](docs/OPEN_HARDWARE_BOM_global_ko.md).
