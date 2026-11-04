<p align="center">
  <img src="brand/friendly-floaty-logo.png" alt="Friendly Floaty" width="540">
</p>

<h1 align="center">Friendly Floaty</h1>
<p align="center"><b>SEA:CUT — open-source river-to-sea litter drifter</b></p>

Open-source, non-profit river-to-sea litter drifter. A low-cost GPS + cellular float you release into a river, track from anywhere, and recover. It maps how floating trash travels from source to sea, so communities know where to place booms and cleanups.

Built by 사단법인 이타서울 / GAA and contributors. Inspired by the open-hardware movement (Precious Plastic, RepRap, OpenMetBuoy).

License: firmware MIT · hardware CERN-OHL-S (reciprocal) · docs CC BY-SA 4.0 (ShareAlike) · data CC BY 4.0 (permissive, so pooled observations combine and cite freely). See [LICENSE-docs.md](LICENSE-docs.md). OSHWA open-source-hardware certification: planned.

SDG relevance: SDG 14.1 (reduce marine pollution, including marine debris) by showing where river litter travels and stops before it reaches the sea; SDG 6.3 (water quality) and SDG 11.6 (municipal waste management) through the same river data. Policies: [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) · [SECURITY.md](SECURITY.md) · [PRIVACY.md](PRIVACY.md) · [SAFETY_MINORS.md](SAFETY_MINORS.md).

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

## Current build: PET-bottle drifter (v4)

One build, no tiers: a 1.5 L round carbonated-drink PET bottle that floats on its side, about half submerged. **No 3D printer is needed.** Status: testing; the water test is not done yet, so parts and steps may change. Parts cost about KRW 100,000 per unit (2026-09 prices), SIM fees separate.

It is DIY. The body and part positions can change as long as the build meets the [eight conditions](https://floatee.caresea.kr/manual#diy).

| Part | Current default |
|---|---|
| Board | LILYGO T-A7670G R2 (External GPS), GPS and LTE antennas included |
| Power | Protected 18650 cell, 6 V 220 mA panel (84×112 mm), 5.6 V 5 W Zener |
| Body | 1.5 L round carbonated-drink PET bottle, cut and rejoined with a PVC 75A ring, self-fusing tape and two SUS316 hose clamps |
| Waterproofing | Two vacuum-bag layers, silica gel, humidity card |
| Weight | Two steel flat bars (about 407 g) in a closed-cell PE foam cradle, trim washers |
| Recovery and marking | Bottle neck and cap lip for gripping, steel washer inside the cap, contact label, reflective tape |

- 3D assembly: [https://floatee.caresea.kr/hardware](https://floatee.caresea.kr/hardware)
- Build manual (Korean): [https://floatee.caresea.kr/manual](https://floatee.caresea.kr/manual), [docs/조립_가이드.md](docs/조립_가이드.md)
- Parts list: [https://floatee.caresea.kr/welcome/parts](https://floatee.caresea.kr/welcome/parts), [hardware/BOM.md](hardware/BOM.md)

Reference board notes: cellular LTE Cat.1 bis plus a separate L76K GPS chip; pick the A7670 band variant for your region.

## Earlier designs (archived)

The tier table (phone, cellular bottle, robust case), the wide-mouth container build and the 3D-printed inserts in `hardware/cad/` were earlier drafts. They stay for reference and forks but are not the current build: [https://floatee.caresea.kr/archive/hardware-printed](https://floatee.caresea.kr/archive/hardware-printed).

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

지금 기본형: 1.5L 둥근 탄산음료 페트병을 옆으로 눕혀 띄우는 드리프터(페트병형 v4, 시험 중). 3D 프린터는 필요 없습니다. DIY 이므로 조건을 지키면 용기와 부품 위치를 바꿀 수 있습니다. 3D 조립도 https://floatee.caresea.kr/hardware , 제작 매뉴얼 https://floatee.caresea.kr/manual , 부품 목록 https://floatee.caresea.kr/welcome/parts .

티어 표, 광구 용기형, `hardware/cad/` 의 3D 프린트 부품은 초기 설계안으로 남겨 둔 기록입니다(https://floatee.caresea.kr/archive/hardware-printed).

전체 글로벌 가이드: [docs/OPEN_HARDWARE_BOM_global_ko.md](docs/OPEN_HARDWARE_BOM_global_ko.md).
