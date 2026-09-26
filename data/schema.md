# Data schema (CC BY 4.0) — Friendly Floaty standard (FF-ID)

시민이 만든 하천쓰레기 표류 관측의 표준·인용 가능 포맷. 데이터는 열려 있고(CC BY), 허브의 힘은 데이터를 가두는 데 있지 않고 **인용 표준·프로버넌스·실시간 지도·커뮤니티**에 있다. 누구나 내려받아 쓰되 인용한다.

**Schema version: 1.1** — v1.0 → 1.1 변경: 디바이스 fix 시각(`ts_fix`)·레코드 시퀀스(`seq`)·샘플링 간격(`sample_interval_s`)·측위 품질(`fix_quality`)·GNSS 출처·좌초 상태(`motion_state`)·**유닛 메타데이터 디렉토리**·**QC/store-and-forward 규칙**을 추가. 근거는 NOAA GDP 메타데이터 관행과 선행사례 연구(`docs/사례연구_기구제작_운영_데이터.md`).

> **정직 가드**: `medium=freshwater`·`link=cellular`(위성 아님)는 설계 고정값. 위치·상태는 물리 계산이지 학습 AI 예측 아님. 조립 트랙·모든 표류 수치는 추정(`is_estimate`).

---

## 1. Ping (디바이스 → 서버) — 최소 페이로드

디바이스는 웨이크마다 최소 페이로드만 보낸다. 서버가 인용 id·프로버넌스를 부여하고 트랙을 조립하며 QC를 돌린다.

`POST /api/observations/drift/ping` (번들 `ingest_server.py`는 `POST /api/ping`)

```json
{"device_id":"river-001","site_id":"nakdong-hakjang","seq":1487,"lat":35.10560,"lon":128.94310,"batt":4.02,"ts_fix":"2026-07-10T09:00:00Z","sample_interval_s":900,"fix_quality":1.4,"gnss_source":"l76k"}
```

| field | type | required | note |
|---|---|---|---|
| device_id | string | yes | 디바이스 식별자(공개 스냅샷에선 익명화) |
| site_id | string | no | 사이트 태그 (예: `nakdong-hakjang`) |
| **seq** | integer | **yes** | **디바이스별 단조증가 레코드 카운터.** store-and-forward 핵심 — 버퍼링돼 늦게 온 핑도 도착순이 아니라 seq로 정렬·중복제거 |
| lat | number | **yes** | **EPSG:4326(WGS84), 소수 ≥5자리(≈1m).** 저정밀 절단은 거부/플래그 |
| lon | number | **yes** | 동일 |
| batt | number | no | **볼트** 단위 |
| **ts_fix** | string | **yes\*** | **디바이스 GPS fix 시각, ISO8601 UTC.** 실제 관측 시각(서버 수신시각과 구분) |
| **sample_interval_s** | integer | **yes** | **공칭 핑 간격(초).** 혼합 주기 데이터를 가중·층화하려면 필수. "펌웨어 자유의 조건 = 핑 간격 신고" |
| **fix_quality** | number | no | HDOP(없으면 sat_count). 없으면 down-weight |
| **gnss_source** | string | no | `l76k` / `phone` / `other` |
| motion_state | string | no | 디바이스 힌트 `afloat`/`stranded`/`unknown`(서버도 추론) |
| flags | integer | no | 회수 비트(0~255). `0x01` 회수 요청 · `0x02` 구역 이탈 · `0x04` 좌초 의심 · `0x08` 마지막 보고 · `0x10` 되살아남. 없거나 0 이면 평상. 번들 서버는 `GET /api/recovery.json` 으로 마지막 레코드가 0 이 아닌 단말을 모은다 |

\* `ts_fix` 결측 핑은 거부하지 않고 §2 `ts_source=server_recv`로 **강등 수용**한다(`is_estimate` 상향; 버퍼 지연 전송 시 관측시각 신뢰 불가). 단 디바이스는 반드시 채워 보내는 것이 표준이다.

---

## 2. Observation record (서버 부여, 인용 가능)

서버가 수용된 핑마다 인용 레코드로 감싼다. 공개 스냅샷·다운로드가 노출하는 형태.

```json
{
  "obs_id": "FF-KR-BS-20260710-001524",
  "schema_version": "1.1",
  "ts_fix": "2026-07-10T09:00:00Z",
  "server_recv_ts": "2026-07-10T09:03:11Z",
  "ts_source": "device_fix",
  "seq": 1487,
  "device_id": "d1yo2o4q",
  "site_id": "nakdong-hakjang",
  "lat": 35.10560, "lon": 128.94310, "crs": "EPSG:4326",
  "batt": 4.02,
  "sample_interval_s": 900,
  "fix_quality": 1.4, "gnss_source": "l76k",
  "position_uncertainty_m": 3.5,
  "motion_state": "afloat",
  "env": { "wind_source": "era5", "wind_u_ms": -2.1, "wind_v_ms": 0.8, "hs_m": null, "stokes": "not measured — join from reanalysis; keep separate from windage" },
  "qc": { "velocity_4sigma": "pass", "is_estimate": false, "censoring": "none" },
  "provenance": { "source": "device", "verification": "raw", "operator": "gaa-kr-busan" },
  "calibration_ref": "ff-drifter-v1",
  "medium": "freshwater", "link": "cellular",
  "license": "CC BY 4.0"
}
```

| field | 의미 |
|---|---|
| obs_id | 전역 유일·인용 id. 형식 `FF-<country>-<region>-<YYYYMMDD>-<seq>`. **서버가 부여**(자체생성 금지). 이걸 인용 |
| ts_fix / server_recv_ts | 디바이스 fix 시각 / 서버 수신 시각. **분리 저장** |
| ts_source | `device_fix`(ts_fix 있음) / `server_recv`(없어 수신시각 대체 → `is_estimate` 상향). ★큰 지연은 Tier0 배치덤프의 "순간이동 가짜궤적" 신호 |
| **position_uncertainty_m** | 위치 불확실성(m). HDOP×UERE 또는 L76K CEP(~2.5m). ★저장정밀도(≈1m) ≠ GNSS 실측 정확도 |
| **env** | co-located 바람(실측/재분석 ERA5·GFS·KMA)·유의파고. **Stokes(파랑유도)는 windage와 분리**·미측정 시 명시(§4.5) |
| **qc.censoring** | 검열·선택 편향 플래그: `none`/`right_censored`(외해 이탈)/`nearshore`/`fair_weather`(§4.5) |
| qc.velocity_4sigma | 2단계 4σ 속도 QC 결과(§4) |
| motion_state | `afloat`/`stranded`/`unknown` — stranded는 표류 통계서 제외/태깅 |
| provenance.verification | `raw` / `auto_checked` / `steward_verified`(실제 스튜어드·기관 확인 시에만, 날조 금지) |
| medium/link | 고정값 `freshwater`/`cellular`(위성 아님) |

---

## 3. Unit metadata directory (유닛별, GDP식) — 신설

NOAA Global Drifter Program 관행 채택: 각 물리 유닛은 **방류·종료·운명(fate)·하드웨어** 디렉토리 레코드를 갖는다. 그래야 트랙 통계가 "조용한 유실"로 편향되지 않는다(회수-우선 정산과 연동).

```json
{
  "unit_id": "FF-KR-BS-U0042",
  "hardware": { "tier": "T1", "board": "lilygo-t-a7670g-r2", "housing": "pet-1L",
                "calibration_ref": "ff-drifter-v1", "verified_build": true },
  "deploy": { "ts": "2026-07-10T09:00:00Z", "lat": 35.132, "lon": 128.968,
              "session_id": "busan-hakjang-2026-07-10", "access_class": "B" },
  "end":    { "ts": "2026-07-10T16:30:00Z", "lat": 35.071, "lon": 128.947,
              "fate": "recovered", "recovery_evidence": { "last_gps": true, "heartbeat_ceased": true } },
  "epoch": "seconds since 1970-01-01T00:00:00Z"
}
```

- **fate 어휘**(회수 체크리스트와 통일): `recovered` · `stranded` · `sunk` · `signal_lost` · `drifted_out` · `retrieved_damaged`.
- **fate 결정규칙**(모호성 제거): `sunk`=마지막 fix 수면하 유속·급강하 후 신호 소멸 + 회수 시 침수 확인 / `signal_lost`=커버리지 내인데 heartbeat 소거(물리 유무 불명·재부팅 대기) / `drifted_out`=마지막 fix가 회수구간 밖 + 커버리지 경계 방향 / `stranded`=지속 근제로 변위. 판정 근거 = 마지막 유효 fix·마지막 heartbeat·회수 물증.
- ★`fate=recovered`는 **증거 필수**(마지막 GPS 종점 + heartbeat 소거). 증거 없으면 `unverified_recovery`로 표기(정산 위조 차단).
- `verified_build=true`는 [FLOAT_STANDARD.md](../hardware/FLOAT_STANDARD.md) F1·F2 통과 유닛만.

---

## 4. 품질관리(QC) — 신설/확장

- **2단계 4σ 속도 QC**(NOAA GDP, Hansen & Poulain 방식): 연속 fix 간 속도를 **전진·후진 유한차분** 양쪽으로 계산해, **둘 다** 국지 속도분포의 4σ를 넘으면 그 점을 bad로 플래그. GPS·셀룰러 이상치 점프에 강건.
- **좌초 탐지**: 지속적 근제로 변위 → `motion_state=stranded`. stranded 점군은 태깅·표류/이송 통계서 제외(둔치 좌초 유닛의 정적 점군이 분산·이동 통계를 오염시키지 않게).
- **store-and-forward 정렬**: 서버는 `ts_fix`로 정렬하고 `seq`로 중복제거·결측 탐지. **도착 순서를 신뢰하지 않음**(Duncan·Merlino·Kang 공통 검증).
- 원시 fix 저장 우선(GPS ≈ 10m 규칙 측위이므로 무거운 크리깅 생략, gap/error 플래그만 공개).

---

## 4.5 알려진 편향·물리 공변량 (관측과학 정직 경계 — 필수)

저건현 부유체는 **완전 라그랑주 추종자가 아니다**(van den Bremer & Breivik 2021: 부유 쓰레기는 해류에 windage·Stokes가 겹침). 이를 문서화하지 않으면 데이터는 물리적으로 해석 불가하다.

- **바람·파랑 공변량(`env`)**: 개별 fix에 windage를 적용하려면 co-located 바람이 필요 → 방류 중 실측 바람(가능 시 유의파고·주기) 또는 재분석(ERA5/GFS/KMA)을 **시각·좌표로 조인**한다. **Stokes(파랑유도) 표류는 windage와 분리**해 다루며, 미측정 시 `env.stokes="not measured"`로 명시한다.
- **위치 불확실성(`position_uncertainty_m`)**: HDOP×UERE 또는 L76K CEP(~2.5m)에서 유도하고, 속도 산출 시 불확실성을 전파한다. **좌표 저장정밀도(≥5자리≈1m)와 GNSS 실측 정확도(~2.5m CEP)는 다르다** — 혼동 금지.
- **검열·선택 편향(`qc.censoring`, 반드시 명명)**: (a) 셀룰러 이탈 = 외해 도달 유닛의 **우측 검열**(`right_censored`, 신호 끊긴 뒤 궤적 미관측) (b) 회수-우선 = **근안 편중**(`nearshore`) (c) 정화이벤트 기반 = **평수·호천후 과대표집**(`fair_weather`). 하천 쓰레기 수송은 실제로 **홍수 지배**(상위 5% 유량, Do et al. 2026)인데 평수 드리프터는 그 레짐을 표집하지 못한다. → 데이터 **주장 범위를 "하천 내·근안·평수 표층 표류"로 한정**하고 flux·오염총량으로 확대하지 않는다.
- **QC 적용 범위 주의**: 4σ 속도QC(Hansen & Poulain 1996)는 외해 평활·보간 데이터 기준이라, 원시·불규칙 하천 fix에 적용하면 **홍수 펄스·좌초를 이상치로 오탐**할 수 있다 → 하천에선 물리 상한(구간 유속)과 병용한다.

---

## 5. Calibration commons

전이 가능한 과학 자산. 각 드리프터 형상은 하천이 아니라 물체의 성질인 windage/drag 계수를 가져 하천 간 이동한다.

```json
{"calibration_id":"ff-drifter-v1","form":{"attitude":"horizontal (belly-ballasted, self-rights antenna-up)","freeboard_ratio":0.2,"submerged_ratio":0.8},"windage_alpha":0.012,"windage_alpha_sd":null,"validated_sites":[{"site":"nakdong-hakjang","windage_alpha":0.013,"alpha_sd":0.003}],"notes":"alpha=BORROWED seed (Kang 2025), NOT self-calibrated. drag_area_ratio omitted: a dragless bottle is not an SVP holey-sock drogue (~40); form-derived drag pending bench+field."}
```

- windage 초기값은 **Kang 2025 제주 하구 실증(α 0.012~0.014, 저건현 PET는 해류 지배)** 을 근거로 낮게 잡고, 정점별 α를 실증 보정해 커먼즈에 되돌린다. 새 사이트는 이 프로파일을 재사용해 적은 드리프터로 보정한다.
- ★**정직 경계**: 이 α는 **차용 시드**일 뿐 **자체 보정 전**이라 `windage_alpha_sd=null`이다 — "이전 가능 커먼즈"로 과대라벨 금지. `form.attitude`는 **단일값 고정**(수평·벨리밸러스트 자기복원)이며, 드로그 없는 병에 SVP식 `drag_area_ratio`(~25.6, 드로그형 ~40에 근접)를 쓰지 않는다(형상유도 drag는 벤치+현장 후 도입). 형상(자세·건현·침수비)이 다른 물체는 **반드시 새 calibration_id로 fork**(geometry_hash 바인딩, DIY_DIVERSITY §5.1). 레지스트리 변형에 `form` 필드로 등록해 다른 물리의 물체가 같은 프로파일로 섞이지 않게 한다.

---

## 6. Self-host / share

자체 서버(`ingest_server.py`)를 돌리거나 공유 커뮤니티 지도에 POST한다. 둘 다 같은 스키마라 데이터가 상호운용된다. **스키마 미준수 = 공유 데이터셋 미포함**(강제 아님, 게이트).

국제표준 상호운용: FF-ID를 **CF-NetCDF(DSG trajectory)·OGC SensorThings·EMODnet Physics**에 매핑한 crosswalk와 무의존성 exporter가 있다 — [../docs/DATA_STANDARDS_CROSSWALK.md](../docs/DATA_STANDARDS_CROSSWALK.md), [export_standards.py](export_standards.py). ★드리프터 트랙은 **trajectory이지 GBIF/Darwin Core occurrence가 아니다**(생물다양성 레지스트리에 넣지 않는다).

## 7. Citation

`obs_id`로 인용한다. 예:

> Friendly Floaty (SEA:CUT) Open Data (2026), observation FF-KR-BS-20260710-001524, CC BY 4.0.

## 8. Changelog · 하위호환

- **1.0 → 1.1**: `ts`(모호) → `ts_fix`+`server_recv_ts`로 분리(1.0 `ts`는 `ts_fix`로 매핑). 신규 필수 `seq`·`sample_interval_s`; 신규 선택 `fix_quality`·`gnss_source`·`motion_state`·`qc`·`crs`. 유닛 메타데이터 디렉토리(§3)·QC(§4) 신설.
- **1.1 추가(2026-09-26)**: 선택 필드 `flags`(회수 비트). 하위호환, 구버전 단말은 평상으로 읽는다.
- 혼합 버전 집계 규칙: `schema_version` 별로 필드 매핑 후 합친다. 1.0 레코드는 `ts_source=device_fix` 가정하되 `sample_interval_s` 없으면 `raw`로 강등(가중 불가).
