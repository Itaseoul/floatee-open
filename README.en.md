<p align="center">
  <img src="brand/friendly-floaty-logo.png" alt="Friendly Floaty" width="540">
</p>

<h1 align="center">Friendly Floaty</h1>
<p align="center"><b>SEA:CUT — open-source river-to-sea litter drifter</b></p>

Open-source, non-profit river-to-sea litter drifter. A low-cost GPS + cellular float you release into a river, track from anywhere, and recover. It maps how floating trash travels from source to sea, so communities know where to place booms and cleanups.

Built by 사단법인 이타서울 / GAA and contributors. Inspired by the open-hardware movement (Precious Plastic, RepRap, OpenMetBuoy).

License: firmware MIT · hardware CERN-OHL-S (reciprocal) · docs and data CC BY-SA 4.0 (ShareAlike). Improvements to designs, docs, and data stay open. OSHWA open-source-hardware certification: planned.

한국어는 아래 [한국어](#한국어) 참조.

---

## What it does

Wake on a timer, get a GPS fix, POST one small JSON ping over the cellular network, sleep. The device sends a single coordinate; a server assembles the track. No user-built gateway is required, the mobile network is the infrastructure.

```
[drifter] --GPS fix--> --cellular (LTE) POST--> [ingest server] --> [public map / open data]
   sleep <-------------------------------------------- 200 OK
```

This is cellular (LTE), not satellite. Coverage is river, estuary, and near-coast where mobile networks reach. Open-sea tracking would need a separate satellite backhaul and is out of scope for this build. (Tier 3 = Iridium satellite is research only; it is not adopted before a KC radio-certification check.)

## The four rules (they travel with the design)

Open-source hardware spreads on a few strong universal rules plus local adaptation, not on the absence of caveats.

1. Recover what you release. A device left in the water is the very problem we fight. Design for recovery, track it, go get it.
2. Safe power, no toxics. Li-ion behind a protection circuit (PCM, cuts over-discharge, over-charge and short circuit), no charging below 0 C or cooking above 45 C, contain a cell failure, non-metallic housing.
3. Do no harm to the water or its users. Smooth non-entangling shapes, avoid protected areas and seasons, tell nearby water users.
4. Own your deployment, follow your local rules. See [docs/DEPLOY_responsibly.md](docs/DEPLOY_responsibly.md).

## Choose your tier

| | Tier 0 Phone-in-a-Bottle | Tier 1 Cellular Bottle (default) | Tier 2 Reusable Robust |
|---|---|---|---|
| Brain | reused smartphone + GPSLogger | ESP32 cellular board | ESP32 cellular board |
| Housing | upcycled PET bottle + dry bag | upcycled PET bottle | bolt-sealed IP case |
| Approx cost | USD 5–35 | USD 50–70 | USD 90–130 |
| Best for | first pipeline test, any budget | the default upcycled float | long / cold / repeated campaigns |

The tiers above are the historical reference. The current build for participants is **one participant kit** (about KRW 92,908 including the board; see [hardware/BOM.md](hardware/BOM.md)): recommended housing is a **1.5 L wide-mouth screw-lid container** (a PET bottle is for demos and short runs only), with a 6 V flat solar panel SZH-SPB017 (84×112 mm, 220 mA) mounted outside on the lid. The panel is included in the kit.

Full parts list: [hardware/BOM.md](hardware/BOM.md). Reference board is the LILYGO T-A7670G R2 with onboard L76K GNSS (cellular LTE Cat.1 bis + separate GPS chip). Pick the A7670 band variant for your region.

## 3D printed parts (parametric, OpenSCAD)

The housing is an upcycled bottle, so only three small printed inserts are needed. Sources are parametric OpenSCAD, so you measure your own bottle and edit one file.

- `hardware/cad/electronics_bracket.scad` holds the board and cell, self-locating in the bottle
- `hardware/cad/ballast_keel.scad` carries lead-free weight low for self-righting
- `hardware/cad/recovery_loop.scad` a smooth closed loop for pole/net recovery

Each ships as `.scad` (editable source), `.stl` (print + GitHub renders it in 3D), and can export `.step`/`.dxf`. Preview: `hardware/cad/assembly.png`.

## Firmware

[firmware/](firmware/) — ESP32 + A7670 + L76K, single ping per wake. Library lewisxhe/TinyGSM-fork + TinyGPSPlus. Report interval is voltage-adaptive by default (`ADAPTIVE_INTERVAL = true`): ≥3.9 V every 60 min, 3.7–3.9 V 120 min, 3.5–3.7 V 360 min, <3.5 V 720 min. Set `ADAPTIVE_INTERVAL = false` to use the fixed `SLEEP_MINUTES` (30 by default; 5 for bench tests).

## Data

[data/schema.md](data/schema.md) — ping and track schema, CC BY. Run your own ingest server or POST to the shared community map.

## Contribute

Build a unit, release and recover it in your river, POST trajectories, add your river and your locale notes. Fork, improve, translate. Open a pull request.

---

## 한국어

하천에서 바다로 가는 쓰레기의 길목을 재는 오픈소스 비영리 드리프터. 강에 띄우고 어디서나 추적하다 회수하는 저비용 GPS + 셀룰러 부유체다. 인증받은 완제품을 배포하지 않는다. 당신이 만들고, 배포를 당신이 책임지고, 지역 규정을 따른다.

동작: 타이머로 깨어나 GPS fix, 셀룰러(LTE)로 ping 하나 POST, 잔다. 서버가 궤적을 조립한다. 게이트웨이 불필요. 이 스택은 셀룰러이지 위성이 아니다. 커버 범위는 이동통신망이 닿는 하천·하구·근연안이다.

네 가지 규칙: (1) 회수한다 (2) 안전 전원·무독성 (3) 물·이용자·야생동물 무해 (4) 네 배포는 네가 책임·지역 규정 확인. 상세 [docs/DEPLOY_responsibly.md](docs/DEPLOY_responsibly.md).

티어: Tier 0 폰+페트병(USD 5–35) / Tier 1 셀룰러+페트병(USD 50–70) / Tier 2 재사용 견고(USD 90–130) / Tier 3 이리듐 위성형(연구용, KC 확인 전 채택 안 함). 지금 참여자에게 권하는 것은 티어 구분 없는 **참여자 키트 한 가지**(보드 포함 약 92,908원)다. 하우징은 **1.5L 광구 용기(나사 뚜껑)** 권장, 페트병은 시연·단기용. 뚜껑 위 바깥에 6V 평판 패널 SZH-SPB017(84×112 mm, 220 mA)을 얹으며 패널은 키트에 포함된다. 보고 간격은 전압에 따라 60~720분으로 바뀐다. 부품표 [hardware/BOM.md](hardware/BOM.md). 기준 보드 LILYGO T-A7670G R2(온보드 L76K).

3D: 하우징이 폐페트병이라 프린트 부품은 3개(브래킷·밸러스트 킬·회수 고리)뿐. 파라메트릭 OpenSCAD라 병 규격만 바꾸면 된다. 한국 실증 상세 검수는 상위 저장소의 `50기_방류_BOM_최종검수.md` 참조.

전체 글로벌 가이드: [docs/OPEN_HARDWARE_BOM_global_ko.md](docs/OPEN_HARDWARE_BOM_global_ko.md).
