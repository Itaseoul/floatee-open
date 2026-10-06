/*
 * Friendly Floaty (SEA:CUT) 드리프터 펌웨어 — LTE Cat.1 bis (A7670) + L76K GPS
 * 대상 보드: LILYGO T-A7670G R2 (With GPS L76K), ESP32-WROVER-E
 * FF-ID 스키마 v1.1 (data/schema.md) 준수. 펌웨어 v1.3 (변경 내역은 같은 폴더 CHANGELOG).
 *
 * 동작(v1.1, store-and-forward):
 *   깨어남 → L76K GPS fix(+UTC 시각) → 레코드를 플래시 저장소에 적재(seq 부여, ffid_store.h)
 *         → 모뎀 전원 → 망 접속(Cat.1 bis) → 버퍼의 미전송 레코드를 오래된 것부터 POST
 *         → 성공분 제거 → 딥슬립.
 *   ★v1.2(2026-09-27): 저장소를 RTC 메모리 16건에서 플래시(LittleFS)로 옮겼다.
 *     전원이 끊겨도 남고, 넘치면 처음 경로와 최근 위치를 지키고 가운데를 솎는다.
 *     음영 구간에서 쌓인 것은 여러 건을 한 번에(JSON 배열) 보낸다. 벤치 확인 전.
 *   ★v1.2 전력 최적화: 셀이 가득(4.05V↑)이면 30분 간격, CPU 80MHz, 접속 실패 시 재시도 간격을
 *     두 배씩(최대 6시간), GPS 가 연속 실패하면 60초만 기다리되 6번에 한 번은 끝까지 기다린다.
 *   ★v1.3(2026-09-28, 전문가 검수 260928 반영. 컴파일만 확인했고 보드 시험 전이다):
 *     방류본·벤치본 빌드 스위치(RELEASE_BUILD) · GNSS 대기 옵션과 20초 하늘 확인 ·
 *     모뎀 전원 차단 뒤 3~5초 확인하고 레일 차단 · LTE 전용·대역 고정 옵션 · 신호 확인 뒤 전송 ·
 *     세션 안 3회 재시도 · 약신호 5건 묶음 · 웨이크당 전송 상한 · 겨울 송신 보류 3.60 V ·
 *     전압 보정과 히스테리시스 · 저장 5,000건 · 리셋 뒤 이어 보내기 · 보 체류·지오펜스 플래그 ·
 *     통신 품질·GNSS 고정 시간·태양광 전압·ICCID 기록 · 유심 PIN 1회 입력.
 *   ★망이 안 잡혀도 fix는 버퍼에 남는다(하천 음영 구간). 다음 웨이크에 몰아 보낸다.
 *   ★도착 순서를 서버가 신뢰하지 않도록 각 레코드는 자기 ts_fix(GPS UTC)와 seq를 갖고 간다.
 *   ★저전압(배터리 방전)엔 모뎀 2A 피크를 피한다: fix만 버퍼에 남기고 전송을 미룬다(하단 저전압 가드).
 *   (선행사례 공통 교훈: Duncan·Merlino·Kang 모두 store-and-forward가 필수였다.
 *    docs/사례연구_기구제작_운영_데이터.md §3-16 참조.)
 *
 * ⚠ 흐름 변경 고지: 초기 벤치 검증본(v1.0)은 "모뎀 접속 → GPS" 순서였다. v1.1은
 *   버퍼링 정합(망 없어도 fix 저장)과 모뎀 유휴 소모 절감을 위해 "GPS → 모뎀" 순서다.
 *   프리미티브(핀맵·전원 시퀀스·라이브러리)는 검증본과 동일하나, 순서 변경분은
 *   벤치 재검증 후 방류에 쓴다(정직 원칙: 측정 전 개선 주장 금지).
 *
 * ★라이브러리 (반드시): lewisxhe/TinyGSM-fork (LILYGO 공식 포크). 스톡 vshymanskyy/TinyGSM
 *   에는 TINY_GSM_MODEM_A7670 매크로가 없어 컴파일이 깨진다. 스톡이 깔려 있으면 제거한다.
 *   그 외: TinyGPSPlus, ArduinoHttpClient, Preferences(ESP32 코어 내장).
 *   보드: ESP32 Arduino core 3.0.x, "ESP32 Dev Module".
 *
 * ★GPS 주의: 이 보드의 GPS는 A7670 모뎀 내장 GNSS가 아니라 별도 Quectel L76K 칩이다.
 *   모뎀 AT(AT+CGNSSPWR/AT+CGNSSINFO)로 읽으면 ERROR가 난다. L76K 전용 UART(Serial2)의
 *   NMEA를 TinyGPSPlus로 파싱한다. 핀은 LILYGO 공식 예제 ExternalGPS_A7670G_Only 기준.
 *
 * ⚠ 법적: 자작 셀룰러 기기의 국내 실증은 전파법 적합성평가 대상이다. 다수 실증 전에
 *    국립전파연구원 연구·기술개발용 면제확인(별지 제12호, 1,500대 이하)을 받는다.
 *    1대 PoC는 국내 데이터 유심 또는 글로벌 IoT SIM으로 접속 테스트만 수행한다.
 */

#define FW_VERSION "1.3.3"

// ── 빌드 종류 (v1.3, 2026-09-28) ──────────────────────────────────────────────
// 1 = 방류본(기본). USE_WIFI 0 · BENCH_HTTP 0 · USE_INA219 0 이 강제된다(LTE + HTTPS, 운영 서버).
// 0 = 벤치본. 아래 USE_WIFI · BENCH_HTTP 값을 쓴다(기본 Wi-Fi + 로컬 평문 서버).
// ★기본을 방류본으로 둔 이유: v1.2 까지 기본값이 Wi-Fi·벤치(1·1)라, 그대로 봉해 방류하면 LTE 를
//   아예 쓰지 않는 기기가 된다(전문가 검수 260928 03 문서 1장). 안전한 쪽을 기본으로 둔다.
// ★명령줄에서 고르기(파일을 고치지 않고):
//   방류본  arduino-cli compile -b esp32:esp32:esp32 <스케치 폴더>
//   벤치본  arduino-cli compile -b esp32:esp32:esp32 --build-property "build.defines=-DRELEASE_BUILD=0" <스케치 폴더>
//   (벤치 LTE 평문이면 "build.defines=-DRELEASE_BUILD=0 -DUSE_WIFI=0" 처럼 함께 준다.)
//   Arduino IDE 에서는 아래 한 줄을 0 으로 바꾼다.
#ifndef RELEASE_BUILD
#define RELEASE_BUILD 1
#endif

#if RELEASE_BUILD
  #undef  USE_WIFI
  #undef  BENCH_HTTP
  #undef  USE_INA219
  #define USE_WIFI   0
  #define BENCH_HTTP 0
  #define USE_INA219 0
#endif

// ── 통신 경로 토글 (벤치본에서만 뜻이 있다) ──────────────────────────────────
// 1 = Wi-Fi(ESP32-WROVER-E 내장). ★유심 없이 시험한다. 사무실 공유기나 휴대폰 핫스팟.
// 0 = LTE(A7670G 모뎀 + 유심). 야외 자율 운용의 정본 경로.
//
// ★Wi-Fi 가 있는 이유: 이 보드의 MCU 가 ESP32 라 Wi-Fi 가 칩에 들어 있다. LTE 모뎀은
//   그 위에 얹힌 별개 부품이다. 그래서 유심이 오기 전에도 GPS 와 서버 왕복을 증명할 수 있다.
// ★한계: 드리프터를 띄워 보내면 수십 미터에서 Wi-Fi 가 끊긴다. 손에 들고 걷는 시험까지다.
//   물에 띄워 흘려보내는 것은 LTE 라야 한다.
#ifndef USE_WIFI
#define USE_WIFI 1
#endif

// ── 전송 대상 토글 (벤치본에서만 뜻이 있다) ──────────────────────────────────
// 1 = 벤치 검증(로컬 ingest_server.py, HTTP 평문, /api/ping).  ★처음엔 이걸로.
// 0 = 운영 전송(floatee.caresea.kr, HTTPS 443, /api/drift/ping). ★2026-09-12 openc 에서 옮김.
#ifndef BENCH_HTTP
#define BENCH_HTTP 1
#endif

// ★조합 넷 — 무엇을 증명하려는지에 따라 고른다.
//   USE_WIFI 1 · BENCH_HTTP 1 → 같은 랜의 PC 사설 IP. **ngrok 이 필요 없다.** 첫 시험은 여기서.
//   USE_WIFI 1 · BENCH_HTTP 0 → 휴대폰 핫스팟으로 floatee. 하천에 들고 나갈 때.
//   USE_WIFI 0 · BENCH_HTTP 1 → ngrok tcp 로 로컬. LTE 자체를 평문으로 검증.
//   USE_WIFI 0 · BENCH_HTTP 0 → 운영. 방류. (= RELEASE_BUILD 1)

// ── CONFIG ────────────────────────────────────────────────────────────────
// ★기기 ID 는 상수가 아니다(v1.3.1, 2026-10-04). 칩 고유번호로 만든다(floatee_identity.h, "ff-" + 12자리).
//   예전 상수 "ff-kr-bs-u0001" 은 서버 등록부 형식(ff-[0-9a-f]{12})에 맞지 않아 계정에 등록할 수 없었다.
#define SITE_ID       "nakdong-hakjang"       // 사이트 태그(서버가 궤적에 태그)
const char* APN       = "";                   // 유심 사업자 APN. 꽂은 유심에 맞게 채운다.
                                              //  Soracom(권장 1대 PoC): "soracom.io"  (USER "sora" / PASS "sora"). 한국 KT/SKT 로밍.
                                              //  1NCE:                   "iot.1nce.net" (USER/PASS 없음). 한국 KT/SKT 로밍.
                                              //  국내 KT 알뜰폰 데이터심:  "lte.ktfwing.com"   (USER/PASS 없음)
                                              //  국내 SKT 알뜰폰 데이터심: "lte.sktelecom.com" (USER/PASS 없음)
                                              //  국내 LG U+·U+ 알뜰폰:    "internet.lguplus.co.kr"
                                              //  ※ 1NCE/Soracom 모두 LG U+ 로밍 미지원. 글로벌 IoT SIM은 KT 또는 SKT로만 붙는다.
const char* GPRS_USER = "";                   // Soracom이면 "sora"
const char* GPRS_PASS = "";                   // Soracom이면 "sora"

// 유심 PIN(v1.3). 방류 유심은 도난 대비로 PIN 을 걸고(휴대폰에서 먼저 설정) 여기에 같은 값을 적는다.
// ★펌웨어는 모뎀이 켜질 때 한 번만 넣는다. 틀리면 그 PIN 을 NVS 에 「실패」로 적고 다시는 넣지 않는다
//   (3회 틀리면 PUK 잠금이라 기기 스스로 유심을 잠그는 사고를 막는다). PIN 값을 바꿔 다시 구우면
//   새 값으로 한 번 더 시도한다. 비워 두면 PIN 을 넣지 않는다(잠긴 유심이면 접속 실패로 끝난다).
const char* SIM_PIN = "";

// Wi-Fi 접속 정보(USE_WIFI 1 일 때만 쓴다). 휴대폰 핫스팟이면 그 이름과 비밀번호를 적는다.
// ★핫스팟은 LG U+ 가입자라도 그대로 쓸 수 있다 — 휴대폰이 LTE 를 쓰고 보드는 Wi-Fi 로
//   그 휴대폰에 붙는 것이라, 자작 단말 등록을 요구하는 U+ 제약에 걸리지 않는다.
// ★2.4 GHz 를 켠다. ESP32 는 5 GHz 를 잡지 못한다(핫스팟 설정에 대역 항목이 있다).
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

#if BENCH_HTTP
  // ★★보드는 Wi-Fi가 아니라 LTE(통신사 망)로 접속한다 → 통신사 NAT 뒤에서 사설 LAN IP
  //   (192.168.x.x)로는 어떤 경우에도 도달할 수 없다(QA 확정). 반드시 공인 노출:
  //   (a) `ngrok tcp 8770` → 발급된 호스트/포트를 아래에 (b) 공유기 포트포워딩+공인 IP.
  //   서버는 `python ingest_server.py 8770 0.0.0.0`으로 기동(기본은 127.0.0.1 루프백).
  //   ※ ngrok http(https 터널)는 TLS라 평문 클라이언트로 안 붙는다 → BENCH_HTTP 0으로.
  const char* SERVER_HOST = "0.tcp.ngrok.io";  // ★ngrok tcp 호스트(또는 공인 IP)로 교체
  const int   SERVER_PORT = 8770;             // ngrok tcp면 발급된 포트
  const char* SERVER_PATH = "/api/ping";
#else
  const char* SERVER_HOST = "floatee.caresea.kr";
  const int   SERVER_PORT = 443;              // https
  const char* SERVER_PATH = "/api/drift/ping";
#endif

const uint32_t SLEEP_MINUTES      = 30;        // 전송 주기(분). 벤치 확인은 5로 낮춰서
const uint32_t GPS_FIX_TIMEOUT_MS = 180000;    // L76K 콜드스타트 여유(야외 30초~수 분)

// ── 몰아 보내기(2026-09-27) ──
// 1 = 쌓인 기록을 JSON 배열로 BATCH_MAX 건씩 한 번에 POST(모뎀 켜진 시간을 줄인다).
//     번들 server/ingest_server.py 는 배열을 받는다. ★운영 서버(floatee /api/drift/ping)가
//     배열을 받는지 확인되기 전에는 운영(BENCH_HTTP 0)에서 0 으로 둔다.
#define BATCH_POST BENCH_HTTP
const uint32_t BATCH_MAX = 20;
// 접속 실패 뒤 모뎀 재시도 간격(수신 없는 자리에서 망 찾기로 배터리를 쓰지 않게).
// 연속 실패마다 두 배로 늘리고(60→120→240분), 좌초 의심이면 바로 최대치로 간다.
// GPS 기록은 매 웨이크 그대로 쌓는다. 접속이 한 번 되면 처음으로 돌아간다.
const uint32_t NET_BACKOFF_START_MINUTES = 60;
const uint32_t STRANDED_TRY_MINUTES = 360;   // 백오프 최대치

// ── 켠 직후(v1.3.2, 2026-10-06) ──
// 전원 투입(슬라이드 스위치 ON, 셀 넣기, 자석 떼기, USB 리셋)은 「새로 시작」으로 본다.
//  ① 접속 실패 백오프와 GPS 실패 횟수를 지운다. v1.3.1 까지는 이 값이 NVS 에 남아, 벤치에서 접속에
//     실패한 기기를 껐다 켜도 60분 이상 모뎀을 켜지 않았다(현장에서 「첫 신호」가 오지 않는다).
//  ② 켠 뒤 LAUNCH_FAST_MINUTES 동안은 LAUNCH_FAST_INTERVAL 분 간격으로 보낸다. 방류 직후 화면에서
//     흐르기 시작했는지 바로 보이게 한다. 전압이 60분 계단 이상(3.90 V↑)일 때만이며, 저전압 기록이
//     있는 기기(보호회로 차단 뒤 되살아남)는 하지 않는다.
const uint32_t LAUNCH_FAST_MINUTES  = 60;
const uint32_t LAUNCH_FAST_INTERVAL = 5;

// ── LTE 망·전송 (v1.3, 전문가 검수 03 문서 7장) ──
// LTE 만 쓴다(AT+CNMP=38). 2G·3G 탐색 시간을 없앤다. 모뎀이 설정을 기억하므로 다를 때만 쓴다.
#define LTE_ONLY 1
// 대역 고정 명령(선택). 비우면 고정하지 않는다. ★A7670 대역 고정 AT 명령과 인자는 SIMCom
//   A76XX AT Command Manual 에서 확인한 뒤 적는다(펌웨어 판마다 다르다. 확인하지 못하였다).
//   LG U+ LTE 는 B5(850 MHz)·B1(2.1 GHz)·B7(2.6 GHz)을 쓴다(2차 출처. 개통 유심으로 AT+CPSI? 에
//   찍히는 대역으로 확인한다). 앞의 "AT" 는 빼고 적는다. 예) "+CBANDCFG=\"LTE\",1,5,7"(형식 미확인).
const char* LTE_BAND_AT = "";
const uint32_t NET_WAIT_MS       = 60000;   // 망 등록 대기
const uint32_t NET_WAIT_SHORT_MS = 30000;   // 연속 실패(2회↑) 때 망 등록 대기. 망 없는 곳에서 60초×150mA 를 매번 쓰지 않게
const uint8_t  SIG_SAMPLES        = 3;      // 등록 뒤 신호를 몇 번 볼지(5초 간격). 병이 파도 마루에 올라갈 때를 기다린다
const uint32_t SIG_SAMPLE_GAP_MS  = 5000;
const int      SIG_GOOD_RSRP      = -105;   // 이 이상이면 더 기다리지 않고 보낸다(dBm)
const int      SIG_WEAK_RSRP      = -110;   // 이 미만이면 약신호: 한 번에 BATCH_WEAK 건
const uint32_t BATCH_WEAK         = 5;
const uint8_t  POST_TRIES         = 3;      // 한 묶음을 세션 안에서 몇 번까지 보낼지(PDP 유지)
const uint32_t POST_RETRY_GAP_MS  = 7000;   // 재시도 간격(5~10초)
const uint32_t MAX_RECS_PER_WAKE  = 200;    // 웨이크당 보내는 기록 상한(쌓인 수천 건을 한 번에 보내다 셀이 먼저 내려가지 않게)
const uint32_t MAX_POSTS_PER_WAKE = 12;     // 웨이크당 POST 횟수 상한(묶음 없이 한 건씩 보낼 때 이쪽이 먼저 걸린다)
const float    TX_CONTINUE_MIN_V  = 3.70f;  // 묶음 사이에 전압을 다시 보고 이 아래면 멈춘다
// 연안 모드: 접속 실패가 이어져 백오프가 최대치에 붙어 있으면 GNSS 간격을 이만큼 이상으로 늘린다
// (망이 없는 바다에서 30분 간격은 저장소만 빨리 채운다). 망이 돌아오면 원래 간격으로 돌아간다.
const uint32_t COASTAL_MINUTES = 120;
const uint8_t  COASTAL_AFTER_FAILS = 4;

// ── 벤치 전류 측정(INA219, 2026-09-27) ──
// 1 = INA219 를 셀 (+)선 중간에 끼웠을 때. 웨이크 한 번에 쓴 전기(mAh)·걸린 시간·최대 전류를
//     재어 다음 보고에 싣는다(wake_mah_prev·wake_s_prev·i_peak_ma_prev).
// ★방류 기기에서는 0(RELEASE_BUILD 1 이 강제한다). 측정 저항(0.1Ω) 때문에 LTE 2A 순간에 0.2V 가
//   떨어져 모뎀 꺼짐 경계에 닿는다.
// ★잘 때 전류는 칩이 자는 동안이라 이 방법으로 못 잰다. 멀티미터 mA 단으로 따로 잰다.
// ★최대 전류는 20ms 마다 읽은 값 중 최댓값이라 1ms 보다 짧은 송신 순간은 놓칠 수 있다.
#ifndef USE_INA219
#define USE_INA219 0
#endif
#define INA_SDA   32         // ★보드 실크에서 빈 핀인지 확인하고 바꾼다(IO21·22 는 GPS)
#define INA_SCL   33
#define INA_ADDR  0x40
const float INA_SHUNT_OHM = 0.1f;

// 성능 최적화(2026-09-27)
const uint32_t CPU_MHZ = 80;                 // 240→80MHz. GPS 대기 동안 칩 소모를 줄인다(Wi-Fi·모뎀 UART 는 80MHz 에서 동작)
const uint32_t GPS_SHORT_TIMEOUT_MS = 60000; // GPS 가 연속 실패하면(숲 그늘 등) 이만큼만 기다린다
const uint8_t  GPS_FAILS_FOR_SHORT  = 3;     // 연속 실패 몇 번부터 짧게 기다릴지
const uint8_t  GPS_FULL_TRY_EVERY   = 6;     // 짧게 기다리는 동안에도 이 횟수마다 한 번은 끝까지 기다린다

// ── GNSS 대기(v1.3, 전문가 검수 03 문서 8장) ──
// 1 = 잘 때 L76K 를 끄지 않고 WAKEUP 핀 LOW 로 대기(원문 20 µA)에 둔다 → 다음 웨이크가 온시동(원문 2초대).
//     이때 모뎀·GNSS 레일(IO12)도 잘 때 HIGH 로 남는다. 모뎀은 AT+CPOF 로 끈 상태라 누설(원문 20 µA)이 붙는다.
// 0 = v1.2 방식. 잘 때 IO12 를 LOW 로 내려 레일째 끊는다 → 매번 냉시동(원문 30초, 병 안에서는 40~90초 추정).
// ★기본 0: 외장 L76K 의 VCC 가 IO12 레일 뒤인지 상시 3.3 V 인지 확인하지 못하였다. 벤치 B1 에서 두 방식의
//   잠 전류와 GNSS 고정 시간(gnss_fix_s)을 나란히 잰 뒤 정한다. 60분 간격이면 하루 약 20 mAh 절약(계산).
#define GNSS_STANDBY 0
const uint32_t GNSS_WARM_MAX_MIN  = 240;    // 이보다 오래 잤으면 궤도 정보가 낡아 온시동을 기대하지 않는다(로그용)
// 하늘 확인: 이 시간이 지나도 NMEA 가 한 글자도 없거나, 보이는 위성(GSV)이 이 수 이하면 곧바로 포기한다.
// 교량 아래처럼 하늘이 막힌 자리에서 180초 × 60mA ≈ 3 mAh 를 매번 쓰지 않게 한다.
const uint32_t GPS_SKY_CHECK_MS   = 20000;
const uint8_t  GPS_SKY_MAX_POOR   = 2;      // 보이는 위성 0~2개면 포기

// ── 태양광 전압(v1.3) ──
// ★핀이 문서마다 어긋난다: LILYGO README 는 IO36, 회로도 조각 그림은 IO34 근처로 읽힌다(검수 03 문서 5장).
//   벤치 B7(패널 단자에 6 V 를 넣고 IO36·IO34 전압 측정)으로 확정한 뒤 이 값을 고친다. -1 이면 읽지 않는다.
// ★분압 배수도 확인하지 못하였다(초깃값 2.0). 멀티미터 값과 보고값을 맞대어 고친다.
#define SOLAR_ADC_PIN  36
const float SOLAR_ADC_DIV = 2.0f;

// ── 배터리 전압 보정(v1.3, 검수 03 문서 6장) ──
// ESP32 ADC 는 eFuse 보정 뒤에도 수십 mV 가 남고, 분압 2배를 곱하면 전지 쪽 ±60~80 mV(추정)다.
// 보드마다 두 점(3.60 V·4.10 V 부근)을 멀티미터로 재어 참값 = 보고값 × GAIN + OFFSET 으로 맞춘다.
//   GAIN = (참4.10 − 참3.60) ÷ (보고4.10 − 보고3.60), OFFSET = 참3.60 − 보고3.60 × GAIN
// 보드마다 다시 굽기 싫으면 NVS 키 "vcal_g"·"vcal_o"(float)에 넣어도 된다(있으면 이 상수보다 앞선다).
const float VBAT_CAL_GAIN     = 1.0f;
const float VBAT_CAL_OFFSET_V = 0.0f;
const float V_HYST = 0.03f;   // 전압 계단·송신 보류의 히스테리시스(±30 mV). 문턱 근처에서 간격이 오가지 않게

// ── 저전압 가드(재QA: LTE 송신 2A 피크가 방전 셀에서 브라운아웃을 일으켜 버퍼·seq를 날린다) ──
// 임계값은 벤치 실측(T3 냉수·부하시험)으로 확정한다. 아래는 18650 방전곡선 기준 보수적 초깃값.
const float    BATT_SKIP_TX_V    = 3.50f;  // 이 이하: GPS fix는 남기되 모뎀(2A 피크) 생략 → 다음 주기에 몰아 전송
const float    WINTER_TX_RAISE_V = 0.10f;  // 11~3월(GPS UTC 월)은 송신 보류·마지막 보고 문턱을 이만큼 올린다(3.50 → 3.60)
const float    DROOP_LIMIT_V     = 0.35f;  // 직전 송신 중 강하가 이보다 크면 다음 번 문턱을 DROOP_RAISE_V 올린다
const float    DROOP_RAISE_V     = 0.10f;
const float    BATT_PARK_V       = 3.30f;  // 이 이하: fix도 생략하고 장주기 park(심방전·셀 손상 방지)
const uint32_t PARK_MULT         = 4;      // park 시 슬립 배수(30분×4=2시간). 전압 회복 대기
// ───────────────────────────────────────────────────────────────────────────

// ── 전압 기반 간격 + 회수 모드 (2026-09-26 추가, 설계서 「방류 전 점검과 회수 펌웨어」) ──
// 전압이 곧 남은 충전량이다. 깰 때마다 재서 다음 잠 시간을 정한다. 값은 벤치 뒤 조정.
const bool     ADAPTIVE_INTERVAL = true;  // false면 SLEEP_MINUTES 고정(벤치용)
const float    V_FULL      = 4.05f;       // 이상: 30분. 가득 찬 셀은 충전이 멈춰 남는 햇빛을 더 잦은 보고에 쓴다
const uint32_t FULL_MINUTES = 30;
const float    V_TIER_1H   = 3.90f;       // 이상: 60분
const float    V_TIER_2H   = 3.70f;       // 이상: 120분
const float    V_TIER_6H   = 3.50f;       // 이상: 360분, 미만: 720분
const float    V_RECOVER   = 3.60f;       // 미만이면 보고에 회수 요청 표시
const float    V_LAST      = 3.55f;       // 이 아래로 처음 내려가면 「마지막 보고」 1회(3.50 전송 보류 직전). 겨울엔 +0.10
const float    V_REVIVE    = 3.75f;       // 저전압 기록이 있는 기기가 이 위로 회복하면 「되살아남」 보고
// 회수 구역(원). 반경 0이면 끈다. 방류 지점마다 굽는다(부산 다대포 예: 35.046, 128.964).
const double   ZONE_LAT = 0.0, ZONE_LON = 0.0;
const float    ZONE_RADIUS_M      = 0.0f;
const uint32_t OUT_ZONE_MINUTES   = 30;   // 구역 밖(또는 지오펜스 안)이고 전압 넉넉하면 30분 간격
// 좌초 의심: 연속 보고 위치가 이 반경 안에 이 시간 이상 머묾
const float    STRAND_RADIUS_M    = 50.0f;
const uint32_t STRAND_MINUTES     = 360;  // 6시간

// ── 보 체류 판정(v1.3, 검수 01 문서 3·4장) ──
// 수중보·낙차공 바로 아래 되돌이 흐름에 갇히면 수 시간~수 일 잠겼다 떴다 한다. 보 좌표 반경 안에서
// WEIR_DWELL_MIN 넘게 머물면 F_WEIR 를 세우고 보고 간격을 WEIR_MINUTES 이상으로 늘려 전지를 아낀다.
// 고장 판정이 아니다. ★좌표는 대략값이다. 방류 전에 지도에서 보 위치를 다시 찍는다.
struct WeirPoint { double lat, lon; float radius_m; };
const WeirPoint WEIRS[] = {
  { 37.6140, 126.7930, 500.0f },   // 한강 신곡수중보(김포대교 하류, 대략값. 확인 전)
};
const uint32_t WEIR_DWELL_MIN = 180;   // 3시간
const uint32_t WEIR_MINUTES   = 120;

// ── 지오펜스(v1.3, 검수 01 문서 4-4) ──
// 들어가면 안 되거나 들어가면 곧 회수해야 하는 구역을 좌표 사각형으로 적는다. 안에 들어가면
// F_GEOFENCE·F_RECOVER 를 세우고 전압이 넉넉하면 30분 간격으로 보고해 회수를 돕는다.
// 예: 한강 신곡수중보 하류 → 조강(남북 중립수역 방향). 한강 경로는 신곡보 상류에서 회수로 끝낸다.
// ★사각형은 대략값이다(육지도 일부 포함한다). 방류 계획마다 지도에서 다시 그린다.
struct GeoBox { double lat_min, lat_max, lon_min, lon_max; };
const GeoBox FENCES[] = {
  { 37.6000, 37.8200, 126.4500, 126.7850 },   // 신곡수중보 하류~조강·강화 북단(대략값)
};

// 보고 플래그(서버 스키마 flags 비트)
#define F_RECOVER   0x01   // 회수 요청
#define F_OUTZONE   0x02   // 구역 이탈
#define F_STRANDED  0x04   // 좌초 의심
#define F_LAST      0x08   // 마지막 보고(이후 스스로 멈춤 가능)
#define F_REVIVED   0x10   // 저전압 뒤 되살아남
#define F_WEIR      0x20   // 보 체류 의심(v1.3)
#define F_GEOFENCE  0x40   // 지오펜스 안(v1.3)
#define F_RESET     0x80   // 이번 부팅이 예기치 않은 리셋(브라운아웃·패닉·송신 중 끊김 등)에서 왔다(v1.3)
// ───────────────────────────────────────────────────────────────────────────

// ★lewisxhe/TinyGSM-fork 전제(스톡엔 A7670 매크로 없음). TinyGsmClientSecure는
//   A76XXSSL 매크로에서만 typedef된다 → 벤치/운영을 매크로로 분기(검수에서 잡은 컴파일 불능).
#if !USE_WIFI
  #if BENCH_HTTP
    #define TINY_GSM_MODEM_A7670      // 벤치: v1.0 검증본과 동일(평문 HTTP)
  #else
    #define TINY_GSM_MODEM_A76XXSSL   // 운영: TinyGsmClientSecure 포함(HTTPS). 모뎀 클래스가 바뀌므로 벤치 재검증 필수
  #endif
  #define TINY_GSM_RX_BUFFER 1024
  #include <TinyGsmClient.h>
#else
  #include <WiFi.h>
  #include <WiFiClientSecure.h>
#endif
#include <ArduinoHttpClient.h>
#include <TinyGPSPlus.h>
#include <Preferences.h>
#include "floatee_identity.h"   // 기기 ID · 등록 코드 · 설치 화면 질의 응답(v1.3.1)
floatee::Identity ident;
#include <driver/gpio.h>

// LILYGO T-A7670G R2 핀맵 (공식 utilities.h / ExternalGPS_A7670G_Only 기준)
#define MODEM_TX          26
#define MODEM_RX          27
#define BOARD_PWRKEY      4
#define BOARD_POWERON     12   // 모뎀·주변 전원 게이트. 동작 내내 HIGH 유지
#define MODEM_RST         5
#define MODEM_DTR         25
#define BOARD_BAT_ADC     35   // 배터리 전압 분압 ADC
// L76K GPS(별도 칩, Serial2). begin(baud, cfg, RX, TX) 순서 주의
#define BOARD_GPS_RX_PIN      22   // ESP32가 GPS NMEA를 받는 핀
#define BOARD_GPS_TX_PIN      21   // ESP32가 GPS로 보내는 핀
#define BOARD_GPS_WAKEUP_PIN  19   // L76K wakeup. HIGH = 동작, LOW = 대기(공식 DeepSleep 예제)
#define GPS_BAUDRATE          9600

// 모뎀 전원 차단 순서(v1.3, 검수 03 문서 4장 / A7670 Hardware Design Table 11):
// AT+CPOF 뒤 최소 MODEM_OFF_MIN_MS 기다리고, AT 응답이 끊긴 것을 확인하거나 MODEM_OFF_MAX_MS 가 지나면
// 그때 IO12 를 내린다. 명령을 끝내기 전에 레일을 끊으면 모뎀이 비정상 상태로 남을 수 있다.
const uint32_t MODEM_OFF_MIN_MS = 3000;
const uint32_t MODEM_OFF_MAX_MS = 5000;

#define SerialAT  Serial1
#define SerialGPS Serial2
#if USE_WIFI
  #if BENCH_HTTP
WiFiClient           client;         // 평문 HTTP(같은 랜의 로컬 서버)
  #else
WiFiClientSecure     client;         // HTTPS(floatee). setInsecure() 는 netConnect 에서
  #endif
#else
TinyGsm        modem(SerialAT);
  #if BENCH_HTTP
TinyGsmClient        client(modem);   // 평문 HTTP
  #else
TinyGsmClientSecure  client(modem);   // HTTPS(포크 SSL). 벤치 통과 후 승격
  #endif
// ★★TLS-AUTH 공백(firmware/README '보안·공급망' 참조): TinyGsmClientSecure는 CA 인증서를
//   설정하지 않으면 서버 인증서를 검증하지 않는다 → 암호화는 되나 인증이 없어 MITM에 열려 있다.
//   실배포 전 setup()의 TLS-AUTH 훅에서 client.setCACert(PROD_CA_PEM)+authmode 검증을 켜고
//   ★하드웨어에서 잘못된 인증서를 '거부'하는지 실제로 확인한다(측정 전 개선 주장 금지).
static const char PROD_CA_PEM[] = "";  // ← 서버 CA/root PEM(예: ISRG Root X1). 비면 인증 미검증.
#endif
HttpClient     http(client, SERVER_HOST, SERVER_PORT);
TinyGPSPlus    gps;
// 보이는 위성 수(GSV 3번째 칸). L76K 는 위성계마다 따로 GSV 를 낸다. 하늘 확인에 쓴다.
TinyGPSCustom  gsvGP(gps, "GPGSV", 3), gsvGL(gps, "GLGSV", 3), gsvGB(gps, "GBGSV", 3),
               gsvBD(gps, "BDGSV", 3), gsvGA(gps, "GAGSV", 3);
Preferences    prefs;
bool           modem_on = false;   // SerialAT.begin 전 modem.poweroff() 방지 가드

// ── FF-ID v1.1 store-and-forward 레코드 ──
// v1.2 부터 기록은 플래시 저장소(ffid_store.h)에 쌓는다. 전원이 끊겨도 남는다.
// ★구조체 배치가 곧 파일 형식이다. 필드를 바꾸면 ffid_store.h 의 파일 이름(v3)을 올리고 옮겨 적기를 더한다.
// ★「_prev」 칸은 직전 웨이크(직전 망 접속)의 값이다. 기록은 GPS 직후, 모뎀을 켜기 전에 만들어지기 때문이다.
struct PingRec {
  double   lat, lon;
  float    batt;
  float    hdop;        // <0 = 무효(필드 생략)
  uint32_t seq;
  char     ts_fix[24];  // GPS UTC ISO8601. 빈 문자열 = 시각 무효
  uint8_t  flags;       // F_* 비트
  uint16_t interval_m;  // 이 레코드 다음 잠 시간(분)
  float    wake_mah_prev;   // 직전 웨이크에 쓴 전기(mAh). <0 = 측정 안 함(USE_INA219 0)
  float    wake_s_prev;     // 직전 웨이크 시간(초)
  float    i_peak_ma_prev;  // 직전 웨이크 최대 전류(mA, 20ms 표본)
  // ── v1.3 ──
  int16_t  rsrp_prev;       // 직전 접속 RSRP(dBm, AT+CPSI?). INT16_MIN = 없음
  int16_t  rssi_prev;       // 직전 접속 RSSI(dBm, AT+CPSI? 또는 AT+CSQ 환산 −113+2×CSQ). INT16_MIN = 없음
  int8_t   rsrq_prev;       // 직전 접속 RSRQ(AT+CPSI? 원시값, dB 로 본다. 단위는 벤치에서 확인). INT8_MIN = 없음
  uint8_t  net_stage_prev;  // 직전 접속이 어디까지 갔나(NS_* 값). 0 = 시도 안 함
  uint8_t  sats;            // 고정에 쓴 위성 수(GGA)
  uint8_t  reset_reason;    // 이번 부팅의 리셋 사유(esp_reset_reason). 8 = 딥슬립 웨이크(평상)
  uint32_t cell_id_prev;    // 직전 접속 셀 ID(AT+CPSI? SCellID). 0 = 없음
  uint16_t reg_s_prev;      // 직전 접속 망 등록에 걸린 초. 0xFFFF = 없음
  uint16_t gnss_fix_s;      // 이번 GNSS 고정에 걸린 초. 0xFFFF = 없음
  uint16_t solar_mv;        // 태양광 전압(mV). 0xFFFF = 없음
  uint16_t brownouts;       // 지금까지 브라운아웃 리셋 누적 횟수(NVS)
  uint16_t vmin_tx_mv_prev; // 직전 송신 중 가장 낮았던 배터리 전압(mV, 20ms 표본). 0 = 없음
};
static_assert(sizeof(PingRec) == 96, "PingRec 크기가 바뀌면 ffid_store.h 파일 이름과 STORE_MAX 근거를 함께 고친다");

// 망 접속 단계(net_stage_prev)
enum NetStage : uint8_t {
  NS_NONE = 0, NS_NO_AT = 1, NS_SIM = 2, NS_PIN_BAD = 3, NS_REG = 4, NS_PDP = 5,
  NS_UP = 6, NS_POSTED = 7, NS_WIFI_FAIL = 8, NS_POST_FAIL = 9
};

void pingRecInit(PingRec &r) {
  memset(&r, 0, sizeof(r));
  r.hdop = -1.0f;
  r.wake_mah_prev = r.wake_s_prev = r.i_peak_ma_prev = -1.0f;
  r.rsrp_prev = INT16_MIN; r.rssi_prev = INT16_MIN; r.rsrq_prev = INT8_MIN;
  r.net_stage_prev = NS_NONE;
  r.reset_reason = 0;
  r.cell_id_prev = 0;
  r.reg_s_prev = 0xFFFF; r.gnss_fix_s = 0xFFFF; r.solar_mv = 0xFFFF;
  r.brownouts = 0; r.vmin_tx_mv_prev = 0;
}
#include "ffid_store.h"

RTC_DATA_ATTR uint32_t rtc_seq   = 0;   // 단조증가 레코드 카운터(딥슬립 생존)
RTC_DATA_ATTR bool     rtc_gnss_kept = false;       // 직전 잠에서 L76K 를 대기로 살려 두었나(GNSS_STANDBY)
RTC_DATA_ATTR float    rtc_prev_wake_mah = -1, rtc_prev_wake_s = -1, rtc_prev_i_peak = -1;
RTC_DATA_ATTR char     rtc_iccid[24] = "";
RTC_DATA_ATTR uint32_t rtc_launch_left_m = 0;   // 켠 직후 빠른 보고가 남은 분(v1.3.2)

// ── 리셋에도 살아야 하는 상태(v1.3 「순단 복구」) ──
// 딥슬립 웨이크에서는 RTC 메모리가 남지만, 브라운아웃·패닉·보호회로 차단 뒤에는 RTC 가 지워질 수 있다.
// 그래서 잠들기 직전마다 같은 내용을 NVS 에 적어 두고(웨이크당 한 번, 약 80바이트. NVS 가 마모를 나눈다),
// 딥슬립이 아닌 리셋으로 깨면 NVS 에서 되살린다. 좌초·보 체류 누적 시간과 백오프가 리셋으로 0 이 되지 않는다.
struct PersistState {
  uint32_t magic;
  double   anchor_lat, anchor_lon;
  uint32_t anchor_min;
  uint32_t weir_min;
  uint32_t last_sleep_m;
  uint32_t min_since_try;
  uint32_t cell_id_prev;
  float    tx_raise_v;          // 직전 송신 강하로 올린 문턱(0 또는 DROOP_RAISE_V)
  int16_t  rsrp_prev, rssi_prev;
  uint16_t reg_s_prev, vmin_tx_mv_prev;
  int8_t   rsrq_prev;
  uint8_t  net_stage_prev;
  uint8_t  net_fails, gps_fails, gps_short_n;
  uint8_t  tier;                // 전압 계단(0 가득 … 4 바닥). 255 = 아직 없음
  uint8_t  month;               // 마지막으로 안 GPS UTC 월(1~12). 0 = 모름
  bool     anchor_ok, last_net_fail, last_stranded, tx_held;
};
static const uint32_t PERSIST_MAGIC = 0xFF130001;
RTC_DATA_ATTR PersistState st = { PERSIST_MAGIC, 0, 0, 0, 0, 0, 0, 0, 0.0f,
                                  INT16_MIN, INT16_MIN, 0xFFFF, 0, INT8_MIN, NS_NONE,
                                  0, 0, 0, 255, 0, false, false, false, false };

// 이번 부팅에서만 쓰는 값
esp_reset_reason_t g_reset = ESP_RST_UNKNOWN;
bool     g_unexpected_reset = false;   // 딥슬립·첫 전원 투입이 아닌 리셋으로 깼나
bool     g_tx_interrupted   = false;   // 직전 웨이크가 송신 도중 끊겼나(NVS "txing")
uint16_t g_brownouts        = 0;
float    g_vcal_gain = VBAT_CAL_GAIN, g_vcal_off = VBAT_CAL_OFFSET_V;
uint32_t g_gnss_fix_s = 0xFFFF;

// 두 좌표 거리(m), 하버사인
double distM(double la1, double lo1, double la2, double lo2) {
  const double R = 6371000.0, d2r = PI / 180.0;
  double dla = (la2 - la1) * d2r, dlo = (lo2 - lo1) * d2r;
  double a = sin(dla/2)*sin(dla/2) + cos(la1*d2r)*cos(la2*d2r)*sin(dlo/2)*sin(dlo/2);
  return 2 * R * atan2(sqrt(a), sqrt(1 - a));
}

// ── INA219 웨이크 에너지 측정(벤치 전용) ──
#if USE_INA219
#include <Wire.h>
volatile float g_wake_mah = 0, g_i_peak = 0;
volatile bool  g_ina_ok = false;
// 분로 전압 레지스터(0x01, LSB 10uV) → mA. 기본 설정(±320mV)이라 0.1Ω 에서 ±3.2A 까지 잰다.
bool inaReadMa(float &ma) {
  Wire.beginTransmission(INA_ADDR); Wire.write(0x01);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(INA_ADDR, 2) != 2) return false;
  int16_t raw = (Wire.read() << 8) | Wire.read();
  ma = (raw * 10.0f) / 1000.0f / INA_SHUNT_OHM;   // uV → mV → mA
  return true;
}
void inaTask(void*) {
  uint32_t last = millis();
  for (;;) {
    float ma;
    uint32_t now = millis();
    if (inaReadMa(ma)) {
      g_ina_ok = true;
      g_wake_mah += ma * (now - last) / 3600000.0f;
      if (ma > g_i_peak) g_i_peak = ma;
    }
    last = now;
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
void inaStart() {
  Wire.begin(INA_SDA, INA_SCL);
  xTaskCreate(inaTask, "ina", 2048, nullptr, 1, nullptr);
}
#endif
void inaFinishWake() {   // 딥슬립 직전에 부른다
#if USE_INA219
  rtc_prev_wake_mah = g_ina_ok ? g_wake_mah : -1;
  rtc_prev_wake_s   = millis() / 1000.0f;
  rtc_prev_i_peak   = g_ina_ok ? g_i_peak : -1;
#endif
}

// 저전압 기록은 NVS에 둔다: 보호회로가 전원을 끊으면 RTC 메모리가 사라지기 때문
bool nvsGetBool(const char* k) { prefs.begin("ffid", true); bool v = prefs.getBool(k, false); prefs.end(); return v; }
void nvsSetBool(const char* k, bool v) { prefs.begin("ffid", false); prefs.putBool(k, v); prefs.end(); }

void persistSave() {
  st.magic = PERSIST_MAGIC;
  prefs.begin("ffid", false);
  prefs.putBytes("st", &st, sizeof(st));
  prefs.end();
}
bool persistLoad() {
  PersistState t;
  prefs.begin("ffid", true);
  size_t n = prefs.getBytes("st", &t, sizeof(t));
  prefs.end();
  if (n != sizeof(t) || t.magic != PERSIST_MAGIC) return false;
  st = t;
  return true;
}

// ── ADC 잠금(v1.3.3) ──
// 송신 중에는 본 작업(readBatteryV)과 전압 감시 작업(vmonTask)이 ADC 를 함께 읽는다. 동시에 읽으면 ADC 드라이버가
// 오류 로그를 찍고, 그 printf 가 감시 작업의 2 KB 스택을 넘어 패닉이 났다(2026-10-06 1호기 벤치, Stack canary vmon).
// 읽기를 한 번에 하나씩만 하게 잠근다.
SemaphoreHandle_t g_adc_mtx = xSemaphoreCreateMutex();
uint32_t adcMv(int pin) {
  xSemaphoreTake(g_adc_mtx, portMAX_DELAY);
  uint32_t mv = analogReadMilliVolts(pin);
  xSemaphoreGive(g_adc_mtx);
  return mv;
}

// ── 배터리 전압 ──
// 100k/100k 분압 가정. analogReadMilliVolts로 eFuse Vref 교정(재QA #3:
// raw*3.3/4095는 ADC 비선형·Vref 편차로 저전압 판정이 수십 mV 어긋난다 → 밀리볼트 API가 교정 내장).
// 저전압 컷오프가 이 값에 걸리므로 교정이 가드의 전제다. v1.3 부터 보드별 보정(GAIN·OFFSET)을 더한다.
float readBatteryV() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += adcMv(BOARD_BAT_ADC);
  mv /= 16;
  float v = (mv / 1000.0f) * 2.0f;  // 분압 2배
  if (v < 1.0f) return v;           // 측정 무효(USB 급전·미장착)는 보정하지 않는다(가드가 1.0 V 로 거른다)
  return v * g_vcal_gain + g_vcal_off;
}

// 태양광 전압(mV). 핀이 -1 이면 0xFFFF(없음).
uint16_t readSolarMv() {
#if SOLAR_ADC_PIN >= 0
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) mv += adcMv(SOLAR_ADC_PIN);
  mv /= 8;
  float v = mv * SOLAR_ADC_DIV;
  return v > 65000 ? 65000 : (uint16_t)v;
#else
  return 0xFFFF;
#endif
}

// 겨울(11~3월)인가. 월은 GPS UTC 에서 얻고, 모르면 마지막으로 안 월을 쓴다. 한 번도 모르면 겨울로 본다(보수적).
bool isWinter() {
  uint8_t m = st.month;
  if (m == 0) return true;
  return m >= 11 || m <= 3;
}
// 이번 웨이크의 송신 보류 문턱
float skipTxV() {
  float v = BATT_SKIP_TX_V + (isWinter() ? WINTER_TX_RAISE_V : 0.0f) + st.tx_raise_v;
  if (g_reset == ESP_RST_BROWNOUT || g_tx_interrupted) v += DROOP_RAISE_V;   // 방금 송신에서 쓰러졌으면 한 번 더 보수적으로
  return v;
}
float lastReportV() { return V_LAST + (isWinter() ? WINTER_TX_RAISE_V : 0.0f) + st.tx_raise_v; }

// 전압 계단(0 = 가득 30분, 1 = 60분, 2 = 120분, 3 = 360분, 4 = 720분). 계단 사이를 오갈 때 ±V_HYST 를 둔다.
static const float    TIER_LOW[4] = { V_FULL, V_TIER_1H, V_TIER_2H, V_TIER_6H };   // 계단 t 의 아래 문턱
static const uint32_t TIER_MIN[5] = { FULL_MINUTES, 60, 120, 360, 720 };
uint8_t voltageTier(float v) {
  uint8_t t = st.tier;
  if (t > 4) {                 // 처음: 히스테리시스 없이 정한다
    t = 4;
    for (uint8_t i = 0; i < 4; i++) if (v >= TIER_LOW[i]) { t = i; break; }
    return t;
  }
  while (t > 0 && v >= TIER_LOW[t - 1] + V_HYST) t--;   // 위 계단으로 오르려면 문턱 + 30 mV
  while (t < 4 && v <  TIER_LOW[t]     - V_HYST) t++;   // 아래 계단으로 내리려면 문턱 − 30 mV
  return t;
}

// 전압과 위치 조건으로 다음 잠 시간을 정한다
uint32_t chooseIntervalM(float v, bool fastReport, bool weir) {
  if (!ADAPTIVE_INTERVAL || v < 1.0f) return SLEEP_MINUTES;   // USB 급전·측정 무효면 고정
  uint8_t t = voltageTier(v);
  st.tier = t;
  uint32_t m = TIER_MIN[t];
  // 연안 모드: 망이 오래 없으면 저장소만 채우지 않게 늘린다(v1.3)
  if (st.last_net_fail && st.net_fails >= COASTAL_AFTER_FAILS && m < COASTAL_MINUTES) m = COASTAL_MINUTES;
  // 보 체류: 되돌이 흐름에 갇힌 동안은 전지를 아낀다(v1.3)
  if (weir && m < WEIR_MINUTES) m = WEIR_MINUTES;
  // 회수 구역 밖·지오펜스 안: 전압이 넉넉하면(60분 계단 이상) 30분 간격으로 회수를 돕는다
  if (fastReport && t <= 1 && m > OUT_ZONE_MINUTES) m = OUT_ZONE_MINUTES;
  return m;
}

// 켠 직후 빠른 보고(v1.3.2). chooseIntervalM 뒤에 부른다. 전압이 60분 계단 이상이거나 USB 급전일 때만.
uint32_t launchIntervalM(uint32_t m, float v) {
  if (rtc_launch_left_m == 0) return m;
  if (v >= 1.0f && st.tier > 1) return m;
  return m < LAUNCH_FAST_INTERVAL ? m : LAUNCH_FAST_INTERVAL;
}

// seq를 NVS와 동기화: 전원상실 후에도 단조증가 유지(중복 seq 방지가 목적, 갭은 허용)
uint32_t nextSeq() {
  prefs.begin("ffid", false);
  uint32_t nv = prefs.getUInt("seq", 0);
  if (nv > rtc_seq) rtc_seq = nv;       // 전원상실 직후: NVS가 최신
  rtc_seq++;
  prefs.putUInt("seq", rtc_seq);
  prefs.end();
  return rtc_seq;
}

// FF-ID v1.1 ping 본문 (data/schema.md §1).
// ts_fix = 디바이스 GPS fix 시각(관측 시각). 구서버 호환 위해 legacy "ts"도 같은 값으로 병송
// (스키마 1.0→1.1 매핑: ts → ts_fix). 서버 수신시각은 서버가 server_recv_ts로 따로 채운다.
// v1.3 칸은 값이 있을 때만 넣는다(서버는 없는 칸을 「없음」으로 읽는다).
String buildPingBody(const PingRec &r) {
  String b = String("{\"device_id\":\"") + ident.device_id + "\",\"site_id\":\"" + SITE_ID + "\"";
  b += ",\"seq\":" + String(r.seq);
  b += ",\"lat\":" + String(r.lat, 6) + ",\"lon\":" + String(r.lon, 6);
  b += ",\"batt\":" + String(r.batt, 2);
  b += ",\"sample_interval_s\":" + String((uint32_t)r.interval_m * 60);
  b += ",\"flags\":" + String(r.flags);   // F_* 비트. 서버는 0 이면 평상으로 읽는다
  if (r.hdop >= 0) b += ",\"fix_quality\":" + String(r.hdop, 1);
  b += ",\"gnss_source\":\"l76k\"";
  b += ",\"fw_version\":\"" FW_VERSION "\"";
  if (r.wake_mah_prev >= 0) {
    b += ",\"wake_mah_prev\":" + String(r.wake_mah_prev, 3);
    b += ",\"wake_s_prev\":" + String(r.wake_s_prev, 1);
    b += ",\"i_peak_ma_prev\":" + String(r.i_peak_ma_prev, 0);
  }
  if (r.sats)                     b += ",\"sats\":" + String(r.sats);
  if (r.gnss_fix_s != 0xFFFF)     b += ",\"gnss_fix_s\":" + String(r.gnss_fix_s);
  if (r.solar_mv != 0xFFFF)       b += ",\"solar_v\":" + String(r.solar_mv / 1000.0f, 2);
  if (r.net_stage_prev)           b += ",\"net_stage_prev\":" + String(r.net_stage_prev);
  if (r.rsrp_prev != INT16_MIN)   b += ",\"rsrp_prev\":" + String(r.rsrp_prev);
  if (r.rsrq_prev != INT8_MIN)    b += ",\"rsrq_prev\":" + String(r.rsrq_prev);
  if (r.rssi_prev != INT16_MIN)   b += ",\"rssi_prev\":" + String(r.rssi_prev);
  if (r.cell_id_prev)             b += ",\"cell_id_prev\":" + String(r.cell_id_prev);
  if (r.reg_s_prev != 0xFFFF)     b += ",\"reg_s_prev\":" + String(r.reg_s_prev);
  if (r.vmin_tx_mv_prev)          b += ",\"vmin_tx_prev\":" + String(r.vmin_tx_mv_prev / 1000.0f, 2);
  if (r.reset_reason)             b += ",\"reset_reason\":" + String(r.reset_reason);
  b += ",\"brownouts\":" + String(r.brownouts);
  if (rtc_iccid[0])               b += ",\"iccid\":\"" + String(rtc_iccid) + "\"";   // 보낼 때 꽂혀 있는 유심(도난 판정용)
  if (r.ts_fix[0]) { b += ",\"ts_fix\":\"" + String(r.ts_fix) + "\",\"ts\":\"" + String(r.ts_fix) + "\""; }
  b += "}";
  return b;
}

// ── 송신 중 최저 전압(v1.3) ──
// 망에 붙는 동안 20 ms 마다 ADC 를 읽어 가장 낮은 값을 남긴다. 겨울 송신 강하 판정에 쓴다.
// ★ESP32 ADC 표본이라 1 ms 보다 짧은 순간 강하는 놓칠 수 있다. 오실로스코프 값과 벤치 B3 에서 맞댄다.
volatile uint16_t g_vmin_mv = 0xFFFF;
volatile bool     g_vmon_run = false;
TaskHandle_t      g_vmon_task = nullptr;
void vmonTask(void*) {
  for (;;) {
    if (g_vmon_run) {
      float v = adcMv(BOARD_BAT_ADC) * 2 / 1000.0f;
      if (v > 1.0f) {
        v = v * g_vcal_gain + g_vcal_off;
        uint16_t m = (uint16_t)(v * 1000.0f);
        if (m < g_vmin_mv) g_vmin_mv = m;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
void vmonStart() {
  g_vmin_mv = 0xFFFF;
  if (!g_vmon_task) xTaskCreate(vmonTask, "vmon", 4096, nullptr, 1, &g_vmon_task);   // v1.3.3: 2048 → 4096(로그 한 줄에 넘쳤다)
  g_vmon_run = true;
}
void vmonStop() { g_vmon_run = false; }

#if !USE_WIFI
void modemPowerOn() {
  pinMode(BOARD_POWERON, OUTPUT); digitalWrite(BOARD_POWERON, HIGH);
  // ★T-A7670 계열 RST는 HIGH=리셋 어서트(공식 utilities.h MODEM_RESET_LEVEL=HIGH). 유휴=LOW.
  //   (검수에서 잡은 v1.0 계승 버그: HIGH로 방치하면 모뎀이 리셋에 잡혀 testAT 전패)
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);  delay(100);
  digitalWrite(MODEM_RST, HIGH); delay(2600);   // 하드 리셋 펄스(공식 시퀀스)
  digitalWrite(MODEM_RST, LOW);                 // 반드시 LOW로 종료
  pinMode(BOARD_PWRKEY, OUTPUT);
  digitalWrite(BOARD_PWRKEY, LOW);  delay(100);
  digitalWrite(BOARD_PWRKEY, HIGH); delay(1000);  // PWRKEY 펄스
  digitalWrite(BOARD_PWRKEY, LOW);
  modem_on = true;
}

// 모뎀을 명령으로 끈다(v1.3). AT+CPOF → 최소 3초 → AT 응답이 끊겼는지 확인(최대 5초).
// 이 함수가 끝난 뒤에야 IO12 레일을 내린다(deepSleepFor).
void modemShutdown() {
  if (!modem_on) return;
  uint32_t t0 = millis();
  modem.poweroff();                       // AT+CPOF
  delay(MODEM_OFF_MIN_MS);
  bool still = true;
  while (millis() - t0 < MODEM_OFF_MAX_MS) {
    if (!modem.testAT(300)) { still = false; break; }
    delay(200);
  }
  Serial.printf("[FF] 모뎀 전원 차단 %s (%lums)\n", still ? "응답 남음(시간 초과)" : "확인", (unsigned long)(millis() - t0));
  modem_on = false;
}

// AT+CPIN? 을 직접 읽는다: TinyGSM getSimStatus 는 「SIM PIN」과 「SIM PUK」을 같은 값으로 돌려준다.
// 반환 1 = READY, 2 = SIM PIN, 3 = SIM PUK, 0 = 그 밖·무응답
int8_t cpinState() {
  for (int i = 0; i < 5; i++) {
    modem.sendAT(GF("+CPIN?"));
    if (modem.waitResponse(2000, GF("+CPIN:")) == 1) {
      int8_t s = modem.waitResponse(2000, GF("READY"), GF("SIM PIN"), GF("SIM PUK"));
      modem.waitResponse();
      if (s >= 1 && s <= 3) return s;
      return 0;
    }
    delay(1000);
  }
  return 0;
}
uint32_t pinHash(const char* p) {   // FNV-1a. PIN 을 그대로 NVS 에 적지 않으려는 것
  uint32_t h = 2166136261u;
  for (; *p; p++) { h ^= (uint8_t)*p; h *= 16777619u; }
  return h ? h : 1;
}
// 유심을 쓸 수 있는 상태로 만든다(v1.3). PIN 은 모뎀이 켜질 때마다 한 번만, 한 번 실패한 PIN 은 다시 넣지 않는다.
bool simReady() {
  int8_t s = cpinState();
  if (s == 1) return true;
  if (s == 3) { Serial.println("[FF] 유심 PUK 잠김 — PIN 을 넣지 않는다(휴대폰에서 PUK 로 푼다)"); st.net_stage_prev = NS_PIN_BAD; return false; }
  if (s != 2) { Serial.println("[FF] 유심 없음·오류(AT+CPIN?)"); st.net_stage_prev = NS_SIM; return false; }
  if (!SIM_PIN[0]) { Serial.println("[FF] 유심이 PIN 으로 잠겼는데 SIM_PIN 이 비었다"); st.net_stage_prev = NS_PIN_BAD; return false; }
  uint32_t h = pinHash(SIM_PIN);
  prefs.begin("ffid", true);
  uint32_t bad = prefs.getUInt("pin_bad", 0);
  prefs.end();
  if (bad == h) {
    Serial.println("[FF] 이 SIM_PIN 은 전에 한 번 틀렸다 — 다시 넣지 않는다(PUK 잠금 방지). 값을 고쳐 다시 굽는다");
    st.net_stage_prev = NS_PIN_BAD;
    return false;
  }
  Serial.println("[FF] 유심 PIN 1회 입력");
  bool ok = modem.simUnlock(SIM_PIN);
  if (ok) {
    for (int i = 0; i < 10 && cpinState() != 1; i++) delay(1000);
    ok = (cpinState() == 1);
  }
  if (!ok) {
    // 틀린 PIN 이든 다른 오류든 같은 PIN 으로는 다시 시도하지 않는다(보수적).
    prefs.begin("ffid", false); prefs.putUInt("pin_bad", h); prefs.end();
    Serial.println("[FF] 유심 PIN 실패 — 기록하고 재시도하지 않는다");
    st.net_stage_prev = NS_PIN_BAD;
    return false;
  }
  return true;
}

// 신호 한 번 읽기. RSSI 는 AT+CSQ(CPSI 에 RSSI 가 있으면 그 값), RSRP·RSRQ·셀 ID 는 AT+CPSI? (LTE 일 때만).
// CPSI LTE 형식: <모드>,<동작>,<MCC-MNC>,<TAC>,<SCellID>,<PCellID>,<대역>,<EARFCN>,<dlbw>,<ulbw>,<RSRQ>,<RSRP>,<RSSI>,<RSSNR>
// ★A7670 판에 따라 칸 수·단위가 다를 수 있다. 벤치에서 원문 응답을 한 번 찍어 맞댄다(시리얼에 출력한다).
void readSignal(int &rsrp, int &rsrq, int &rssi, uint32_t &cell) {
  rsrp = INT16_MIN; rsrq = INT8_MIN; rssi = INT16_MIN; cell = 0;
  int16_t csq = modem.getSignalQuality();
  if (csq >= 0 && csq <= 31) rssi = -113 + 2 * csq;
  modem.sendAT(GF("+CPSI?"));
  String s;
  if (modem.waitResponse(3000, s) != 1) return;
  int i = s.indexOf("+CPSI:");
  if (i < 0) return;
  s = s.substring(i + 6);
  int e = s.indexOf('\n');
  if (e >= 0) s = s.substring(0, e);
  s.trim();
  Serial.printf("[FF] CPSI %s\n", s.c_str());
  if (!s.startsWith("LTE")) return;
  String f[14]; int n = 0, from = 0;
  while (n < 14) {
    int c = s.indexOf(',', from);
    if (c < 0) { f[n++] = s.substring(from); break; }
    f[n++] = s.substring(from, c);
    from = c + 1;
  }
  if (n >= 5)  cell = strtoul(f[4].c_str(), nullptr, 0);   // 10진·16진(0x) 모두
  if (n >= 12) { rsrq = f[10].toInt(); rsrp = f[11].toInt(); }
  if (n >= 13 && f[12].length()) rssi = f[12].toInt();
}
#endif

bool g_weak_signal = false;   // 이번 세션 약신호(RSRP < SIG_WEAK_RSRP)

// 망에 붙는다. Wi-Fi 와 LTE 의 차이를 여기 한 곳에 가두어 setup 이 같은 모양을 유지하게 한다.
// 실패하면 false — 호출부는 관측을 버퍼에 남긴 채 슬립한다(store-and-forward).
// 결과(단계·등록 시간·신호)는 st.*_prev 에 남아 다음 레코드에 실린다.
bool netConnect() {
  st.net_stage_prev = NS_NONE;
  st.reg_s_prev = 0xFFFF; st.rsrp_prev = INT16_MIN; st.rssi_prev = INT16_MIN;
  st.rsrq_prev = INT8_MIN; st.cell_id_prev = 0;
#if USE_WIFI
  if (!WIFI_SSID[0]) { Serial.println("[FF] WIFI_SSID 가 비었다"); st.net_stage_prev = NS_WIFI_FAIL; return false; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[FF] Wi-Fi 접속 시도 %s\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) delay(200);
  st.reg_s_prev = (millis() - t0) / 1000;
  if (WiFi.status() != WL_CONNECTED) {
    // ★2.4 GHz 인지 먼저 본다. ESP32 는 5 GHz 를 아예 잡지 못하고, 휴대폰 핫스팟은
    //   기본이 5 GHz 인 기종이 있다(설정에 대역 항목이 있다).
    Serial.println("[FF] Wi-Fi 실패(2.4GHz·비밀번호 확인) → 버퍼 보존, 슬립");
    st.net_stage_prev = NS_WIFI_FAIL;
    return false;
  }
  st.rssi_prev = WiFi.RSSI();
  Serial.print("[FF] 접속 IP "); Serial.println(WiFi.localIP());
  #if !BENCH_HTTP
  // 벤치 단계에서는 서버 인증서를 검증하지 않는다. 루트 CA 를 굽는 것은 운영(LTE) 경로의
  // 과제이고, 여기서 막히면 정작 보려던 GPS·왕복 검증이 멈춘다.
  // ★운영 방류에는 이 설정을 쓰지 않는다.
  client.setInsecure();
  #endif
  st.net_stage_prev = NS_UP;
  return true;
#else
  modemPowerOn();
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);  // 공식 예제와 동일(RX,TX 순서)
  delay(3000);
  Serial.println("[FF] 모뎀 초기화");
  if (!modem.testAT(10000)) {
    // 무응답이면: (1) 18650 배터리 장착(USB만 급전 금지) (2) BOARD_POWERON HIGH (3) baud 115200
    Serial.println("[FF] 모뎀 무응답 → 관측은 버퍼에 보존, 슬립");
    st.net_stage_prev = NS_NO_AT;
    return false;
  }
  if (!simReady()) { Serial.println("[FF] 유심 준비 실패 → 버퍼 보존, 슬립"); return false; }
  // ICCID: 도난 판정용(서버). 바뀌었을 때만 NVS 에 적는다.
  {
    String cc = modem.getSimCCID();
    cc.trim();
    if (cc.length() >= 10 && cc.length() < sizeof(rtc_iccid) && cc != String(rtc_iccid)) {
      strncpy(rtc_iccid, cc.c_str(), sizeof(rtc_iccid) - 1);
      rtc_iccid[sizeof(rtc_iccid) - 1] = 0;
      prefs.begin("ffid", false); prefs.putString("iccid", rtc_iccid); prefs.end();
    }
  }
  #if LTE_ONLY
  if (modem.getNetworkMode() != MODEM_NETWORK_LTE) {
    Serial.println("[FF] 망 방식을 LTE 전용으로(AT+CNMP=38)");
    modem.setNetworkMode(MODEM_NETWORK_LTE);
  }
  #endif
  if (LTE_BAND_AT[0]) {
    modem.sendAT(LTE_BAND_AT);
    if (modem.waitResponse(5000) != 1) Serial.println("[FF] 대역 고정 명령 실패(LTE_BAND_AT 확인)");
  }
  uint32_t waitMs = st.net_fails >= 2 ? NET_WAIT_SHORT_MS : NET_WAIT_MS;
  uint32_t t0 = millis();
  bool reg = modem.waitForNetwork(waitMs);
  st.reg_s_prev = (millis() - t0) / 1000;
  if (!reg) {
    Serial.printf("[FF] 망 등록 실패(%us) → 버퍼 보존, 슬립\n", (unsigned)st.reg_s_prev);
    st.net_stage_prev = NS_REG;
    return false;
  }
  // 신호를 5초 간격으로 최대 SIG_SAMPLES 번 본다. 충분히 좋으면 바로 보낸다. 가장 좋았던 값을 남긴다.
  int best = INT16_MIN;
  for (uint8_t k = 0; k < SIG_SAMPLES; k++) {
    int rsrp, rsrq, rssi; uint32_t cell;
    readSignal(rsrp, rsrq, rssi, cell);
    if (k == 0 || rsrp > best) {
      best = rsrp;
      st.rsrp_prev = (int16_t)rsrp;
      st.rsrq_prev = (rsrq == INT8_MIN) ? INT8_MIN : (int8_t)constrain(rsrq, -127, 127);
      st.rssi_prev = (int16_t)rssi;
      st.cell_id_prev = cell;
    }
    Serial.printf("[FF] 신호 %u/%u RSRP=%d RSRQ=%d RSSI=%d cell=%lu\n", k + 1, SIG_SAMPLES, rsrp, rsrq, rssi, (unsigned long)cell);
    if (rsrp != INT16_MIN && rsrp >= SIG_GOOD_RSRP) break;
    if (k + 1 < SIG_SAMPLES) delay(SIG_SAMPLE_GAP_MS);
  }
  g_weak_signal = (best != INT16_MIN && best < SIG_WEAK_RSRP);
  if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
    Serial.println("[FF] PDP 실패(APN 확인) → 버퍼 보존, 슬립");
    st.net_stage_prev = NS_PDP;
    return false;
  }
  Serial.print("[FF] 접속 IP "); Serial.println(modem.getLocalIP());
  st.net_stage_prev = NS_UP;
  return true;
#endif
}

// 세션이 살아 있나. PDP 가 끊겼으면 한 번만 다시 붙는다(재시도 사이).
bool netAlive() {
#if USE_WIFI
  return WiFi.status() == WL_CONNECTED;
#else
  if (modem.isGprsConnected()) return true;
  Serial.println("[FF] PDP 끊김 → 한 번 다시 연결");
  return modem.gprsConnect(APN, GPRS_USER, GPRS_PASS);
#endif
}

void netDisconnect() {
#if USE_WIFI
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
#else
  modem.gprsDisconnect();
#endif
}

// 보이는 위성 수(GSV 합). 한 번도 못 읽었으면 -1.
int satsInView() {
  int total = -1;
  TinyGPSCustom* g[] = { &gsvGP, &gsvGL, &gsvGB, &gsvBD, &gsvGA };
  for (auto* c : g) {
    if (c->isValid() && c->age() < 5000) { if (total < 0) total = 0; total += atoi(c->value()); }
  }
  return total;
}

// L76K GPS(Serial2)에서 NMEA를 읽어 위경도 취득. 성공 시 true.
// ★모뎀 AT가 아니라 L76K UART다. 실내에서는 대부분 fix 안 됨 → 창가/야외에서 확인.
// ★GPS 전원은 BOARD_POWERON 레일에 걸려 있어 fix 전에 POWERON을 먼저 HIGH로 올린다.
// ★v1.3: GPS_SKY_CHECK_MS 가 지나도 NMEA 가 없거나 보이는 위성이 GPS_SKY_MAX_POOR 개 이하면 곧바로 포기한다.
//   걸린 시간은 g_gnss_fix_s 에 남는다.
bool getFix(double &lat, double &lon, uint32_t timeoutMs = GPS_FIX_TIMEOUT_MS) {
  pinMode(BOARD_POWERON, OUTPUT); digitalWrite(BOARD_POWERON, HIGH);  // GPS 레일 확보(모뎀보다 먼저 필요)
  pinMode(BOARD_GPS_WAKEUP_PIN, OUTPUT); digitalWrite(BOARD_GPS_WAKEUP_PIN, HIGH);  // 대기였으면 깨운다
  SerialGPS.begin(GPS_BAUDRATE, SERIAL_8N1, BOARD_GPS_RX_PIN, BOARD_GPS_TX_PIN);
  uint32_t start = millis();
  bool skyChecked = false;
  g_gnss_fix_s = 0xFFFF;
  while (millis() - start < timeoutMs) {
    while (SerialGPS.available()) gps.encode(SerialGPS.read());
    bool locOk  = gps.location.isValid() && gps.location.age() < 3000 && gps.satellites.value() >= 4;
    bool timeOk = gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2025;
    if (locOk && timeOk) {           // RMC(날짜)는 같은 에포크에 오므로 지연 최대 ~1초.
      lat = gps.location.lat();      // 날짜 검증 없이 리턴하면 ts_fix가 비어 서버가 server_recv로
      lon = gps.location.lng();      // 강등 — 버퍼 지연 레코드는 관측시각이 통째로 틀어짐(검수 발견)
      g_gnss_fix_s = (millis() - start) / 1000;
      return true;
    }
    if (!skyChecked && millis() - start >= GPS_SKY_CHECK_MS) {
      skyChecked = true;
      int inView = satsInView();
      if (gps.charsProcessed() == 0) {
        Serial.println("[FF] GPS 20초 동안 NMEA 없음(전원·배선 확인) → 포기");
        return false;
      }
      if (inView >= 0 && inView <= GPS_SKY_MAX_POOR) {
        Serial.printf("[FF] GPS 20초에 보이는 위성 %d개 → 하늘이 막힌 자리로 보고 포기\n", inView);
        return false;
      }
    }
    delay(10);
  }
  // 타임아웃 폴백: 위치만이라도 유효하면 관측은 살린다(ts_fix만 결측 → 서버가 server_recv 강등)
  if (gps.location.isValid() && gps.location.age() < 3000 && gps.satellites.value() >= 4) {
    lat = gps.location.lat(); lon = gps.location.lng();
    g_gnss_fix_s = (millis() - start) / 1000;
    return true;
  }
  // 진단: chars=0이면 GPS 무전원/배선(POWERON 레일 전제 확인 — 벤치 1차 관문), >0이면 단순 미픽스
  Serial.printf("[FF] GPS fix 실패: NMEA chars=%lu\n", (unsigned long)gps.charsProcessed());
  return false;
}

// GPS UTC 시각을 ISO8601로. 유효하지 않으면 빈 문자열.
void gpsIso8601(char* buf, size_t n) {
  if (gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2025) {
    snprintf(buf, n, "%04d-%02d-%02dT%02d:%02d:%02dZ",
             gps.date.year(), gps.date.month(), gps.date.day(),
             gps.time.hour(), gps.time.minute(), gps.time.second());
  } else {
    buf[0] = '\0';
  }
}

// POST 한 번. 2xx 면 true.
bool postBody(const String &body) {
  http.beginRequest();
  http.post(SERVER_PATH);
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", body.length());
  // 등록 코드(v1.3.1). 서버가 계정 등록부와 대조한다. 등록 전이면 401 → 저장소에 남겨 두고 등록 뒤 보낸다.
  http.sendHeader("Authorization", String("Bearer ") + ident.secret);
  http.beginBody();
  http.print(body);
  http.endRequest();
  int status = http.responseStatusCode();
  if (status > 0) http.responseBody();   // 유효 응답만 소진(오류코드면 재차 30초 대기 회피)
  Serial.print("[FF] 서버 응답 "); Serial.println(status);
  if (status >= 200 && status < 300) return true;
  http.stop();
  return false;
}

// 저장소 앞에서부터(오래된 순) 보낸다. 성공한 만큼 저장소에서 지우고, 실패하면 멈춘다
// (남은 건 다음 웨이크에). BATCH_POST 면 BATCH_MAX 건씩(약신호면 BATCH_WEAK 건) JSON 배열로 보낸다.
// v1.3: 한 묶음을 세션 안에서 POST_TRIES 번까지 보내고, 웨이크당 상한(건수·POST 수)과 묶음 사이 전압을 본다.
// 반환: 보낸 개수.
int flushStore() {
  uint32_t sent = 0, posts = 0;
  uint32_t bmax = g_weak_signal ? BATCH_WEAK : BATCH_MAX;
  if (g_weak_signal && BATCH_POST) Serial.printf("[FF] 약신호 → 한 번에 %lu건\n", (unsigned long)bmax);
  while (storeCount() > 0) {
    if (sent >= MAX_RECS_PER_WAKE || posts >= MAX_POSTS_PER_WAKE) {
      Serial.printf("[FF] 웨이크당 전송 상한 도달(%lu건·%lu회) → 나머지는 다음 웨이크\n", (unsigned long)sent, (unsigned long)posts);
      break;
    }
    uint32_t k = 1;
    if (BATCH_POST) {
      k = storeCount();
      if (k > bmax) k = bmax;
      if (k > MAX_RECS_PER_WAKE - sent) k = MAX_RECS_PER_WAKE - sent;
    }
    String body = BATCH_POST ? "[" : "";
    PingRec r;
    uint32_t got = 0;
    for (; got < k; got++) {
      if (!storeRead(got, r)) break;
      if (BATCH_POST && got) body += ",";
      body += buildPingBody(r);
    }
    if (got == 0) break;
    if (BATCH_POST) body += "]";
    bool ok = false;
    for (uint8_t t = 0; t < POST_TRIES && !ok && posts < MAX_POSTS_PER_WAKE; t++) {
      if (t) {
        Serial.printf("[FF] 재시도 %u/%u (%lums 뒤)\n", t + 1, POST_TRIES, (unsigned long)POST_RETRY_GAP_MS);
        delay(POST_RETRY_GAP_MS);
        if (!netAlive()) break;
      }
      Serial.printf("[FF] POST %lu건 (%u바이트)\n", (unsigned long)got, body.length());
      ok = postBody(body);
      posts++;
    }
    if (!ok) { st.net_stage_prev = sent ? NS_POSTED : NS_POST_FAIL; break; }   // 실패: 다음 주기에 재시도
    storeDropFront(got);
    sent += got;
    st.net_stage_prev = NS_POSTED;
    float v = readBatteryV();
    if (v > 1.0f && v < TX_CONTINUE_MIN_V && storeCount() > 0) {
      Serial.printf("[FF] 묶음 사이 전압 %.2fV < %.2fV → 멈춤\n", v, TX_CONTINUE_MIN_V);
      break;
    }
  }
  return (int)sent;
}

// 지정 분(minutes)만큼 딥슬립.
// GNSS_STANDBY 0: 모뎀을 명령으로 끈 뒤(3~5초 확인) 모뎀·L76K 레일을 끊고(POWERON LOW) GPIO를 홀드한다.
// GNSS_STANDBY 1: 모뎀만 명령으로 끄고 레일은 살려 두며, L76K 는 WAKEUP LOW 로 대기에 둔다(온시동).
void deepSleepFor(uint32_t minutes) {
#if USE_WIFI
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);   // 슬립 전 라디오를 끈다. 안 끄면 딥슬립 진입이 늦고 전류가 남는다
#else
  modemShutdown();       // ★레일(IO12)을 내리기 전에 반드시 먼저(v1.3)
#endif
  vmonStop();
  st.last_sleep_m = minutes;
  persistSave();         // 리셋에도 남게(순단 복구)
  // 딥슬립 중 일반 GPIO는 플로팅 → GPS_WAKEUP이 HIGH로 남으면 L76K가 슬립 내내 켜져
  // 배터리를 지배할 수 있다(공식 DeepSleep 예제 방식으로 홀드). 절감폭은 실측 전.
  pinMode(BOARD_GPS_WAKEUP_PIN, OUTPUT); digitalWrite(BOARD_GPS_WAKEUP_PIN, LOW);
  gpio_hold_en((gpio_num_t)BOARD_GPS_WAKEUP_PIN);
  pinMode(MODEM_RST, OUTPUT);            digitalWrite(MODEM_RST, LOW);
  gpio_hold_en((gpio_num_t)MODEM_RST);
  pinMode(BOARD_PWRKEY, OUTPUT);         digitalWrite(BOARD_PWRKEY, LOW);   // 레일이 살아 있을 때 떠 있는 PWRKEY 가 모뎀을 켜지 않게
  gpio_hold_en((gpio_num_t)BOARD_PWRKEY);
  pinMode(BOARD_POWERON, OUTPUT);
#if GNSS_STANDBY
  digitalWrite(BOARD_POWERON, HIGH);     // L76K 대기를 위해 레일 유지(모뎀은 CPOF 로 꺼짐, 누설 약 20 µA)
  rtc_gnss_kept = true;
#else
  digitalWrite(BOARD_POWERON, LOW);      // 레일째 차단 → 다음 웨이크 냉시동
  rtc_gnss_kept = false;
#endif
  gpio_hold_en((gpio_num_t)BOARD_POWERON);
  gpio_deep_sleep_hold_en();
  inaFinishWake();
  esp_sleep_enable_timer_wakeup((uint64_t)minutes * 60ULL * 1000000ULL);
  esp_deep_sleep_start();
}

// 이번 깨어남에서 정한 다음 잠 시간(전압 기반). setup 에서 채운다.
uint32_t g_next_sleep_m = SLEEP_MINUTES;
void deepSleep() { deepSleepFor(g_next_sleep_m); }

void setup() {
  // 웨이크 직후 홀드 해제(없으면 이후 digitalWrite가 홀드에 막혀 무효)
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)BOARD_GPS_WAKEUP_PIN);
  gpio_hold_dis((gpio_num_t)MODEM_RST);
  gpio_hold_dis((gpio_num_t)BOARD_PWRKEY);
  gpio_hold_dis((gpio_num_t)BOARD_POWERON);
  setCpuFrequencyMhz(CPU_MHZ);
#if USE_INA219
  inaStart();
#endif
  Serial.begin(115200);
  analogReadResolution(12);
  // 신원(v1.3.1). 딥슬립 웨이크가 아닐 때(전원 투입·USB 연결로 리셋)만 3초 동안 설치 화면 질의에 답한다.
  // 웹 화면이 포트를 열면 보드가 리셋되므로 그때 이 창이 열린다. 딥슬립 웨이크마다 기다리면 전기만 쓴다.
  floatee::loadOrCreate(ident);
  Serial.printf("[FF] device_id=%s  등록 https://floatee.caresea.kr/start\n", ident.device_id);
  if (esp_reset_reason() != ESP_RST_DEEPSLEEP) floatee::listenWindow(Serial, ident, FW_VERSION, 3000);
  Serial.printf("[FF] fw %s %s 빌드 (USE_WIFI=%d BENCH_HTTP=%d GNSS_STANDBY=%d)\n", FW_VERSION,
                RELEASE_BUILD ? "방류" : "벤치", USE_WIFI, BENCH_HTTP, GNSS_STANDBY);
#if !USE_WIFI && !BENCH_HTTP
  if (!PROD_CA_PEM[0]) Serial.println("[FF] 주의: PROD_CA_PEM 이 비었다. 서버 인증서를 검증하지 않는다(README 보안 절)");
#endif
  storeBegin();

  // ── 리셋 사유와 순단 복구(v1.3) ──
  // 리셋 사유 로깅(재QA #5): 브라운아웃(ESP_RST_BROWNOUT=9 · IDF 5.1 헤더 확인 2026-10-04)이 반복되면 저전압 가드·전원설계 재검토 신호
  g_reset = esp_reset_reason();
  Serial.printf("[FF] reset_reason=%d (1=전원 3=SW 4=panic 6=태스크WDT 8=deepsleep 9=BROWNOUT)\n", (int)g_reset);
  prefs.begin("ffid", false);
  g_vcal_gain = prefs.getFloat("vcal_g", VBAT_CAL_GAIN);   // NVS 보정값이 있으면 상수보다 앞선다
  g_vcal_off  = prefs.getFloat("vcal_o", VBAT_CAL_OFFSET_V);
  g_brownouts = prefs.getUShort("bo_n", 0);
  g_tx_interrupted = prefs.getBool("txing", false);
  if (g_reset == ESP_RST_BROWNOUT && g_brownouts < 65535) prefs.putUShort("bo_n", ++g_brownouts);
  if (!rtc_iccid[0]) {
    String s = prefs.getString("iccid", "");
    strncpy(rtc_iccid, s.c_str(), sizeof(rtc_iccid) - 1);
    rtc_iccid[sizeof(rtc_iccid) - 1] = 0;
  }
  prefs.end();
  if (g_reset != ESP_RST_DEEPSLEEP) {
    g_unexpected_reset = (g_reset != ESP_RST_POWERON);   // 첫 전원 투입(셀 넣기)은 사고가 아니다
    if (persistLoad()) Serial.println("[FF] 리셋 뒤 상태를 NVS 에서 되살림(좌초·보 체류 누적, 백오프, 전압 계단)");
    if (g_tx_interrupted) { g_unexpected_reset = true; Serial.println("[FF] 직전 웨이크가 송신 도중 끊겼다 → 이어 보내기, 문턱 +0.10V"); }
  } else {
    g_tx_interrupted = false;   // 딥슬립으로 정상 종료했으면 송신 중 끊김이 아니다
  }
  nvsSetBool("txing", false);

  // ── 켠 직후(v1.3.2) ── 전원 투입이면 백오프·GPS 실패를 지우고 빠른 보고 창을 연다.
  // 좌초·보 체류 누적(anchor·weir)은 지우지 않는다. 보호회로 차단 뒤 되살아남도 전원 투입이라
  // 그 연속성을 끊지 않는다. 자리가 50 m 넘게 바뀌면 첫 고정에서 저절로 새로 잡힌다.
  if (g_reset == ESP_RST_POWERON) {
    st.last_net_fail = false; st.net_fails = 0; st.min_since_try = 0; st.last_sleep_m = 0;
    st.gps_fails = 0; st.gps_short_n = 0;
    rtc_launch_left_m = nvsGetBool("lowbatt") ? 0 : LAUNCH_FAST_MINUTES;
    Serial.printf("[FF] 전원 투입 → 접속 대기 기록 초기화, 빠른 보고 %lu분\n", (unsigned long)rtc_launch_left_m);
  } else if (g_reset == ESP_RST_DEEPSLEEP && rtc_launch_left_m > 0) {
    rtc_launch_left_m = st.last_sleep_m >= rtc_launch_left_m ? 0 : rtc_launch_left_m - st.last_sleep_m;
  }

  // ★저전압 park(모뎀·GPS 켜기 전에 먼저): 심방전 구간이면 아무 동작 없이 장주기로 자 셀을 보호한다.
  float vbat0 = readBatteryV();
  if (vbat0 > 1.0f && vbat0 < BATT_PARK_V) {   // >1.0V = 측정 유효(USB 급전·미장착 오검 배제)
    uint32_t parkM = ADAPTIVE_INTERVAL ? 720 : SLEEP_MINUTES * PARK_MULT;
    Serial.printf("[FF] 저전압 park %.2fV<%.2fV → %lu분 슬립\n", vbat0, BATT_PARK_V, (unsigned long)parkM);
    nvsSetBool("lowbatt", true);           // 되살아남 판정용(보호회로 차단으로 RTC가 지워져도 남는다)
    deepSleepFor(parkM);
  }
  g_next_sleep_m = launchIntervalM(chooseIntervalM(vbat0, false, false), vbat0);

  st.min_since_try += st.last_sleep_m;

  // ── 이어 보내기(순단 복구, v1.3) ──
  // 송신 도중 리셋이면 그 웨이크의 고정은 이미 저장돼 있다. 냉시동 GNSS 를 다시 돌리지 않고
  // (브라운아웃 → 재부팅 → 냉시동 → 또 송신의 고리를 끊는다) 올린 문턱으로 남은 것만 보낸다.
  bool resumeTx = g_tx_interrupted && storeCount() > 0;

  // ① GPS fix 먼저(망 없어도 관측은 남긴다 — store-and-forward의 요점)
  double lat = 0, lon = 0;
  bool fixed = false;
  if (!resumeTx) {
    uint32_t gpsTimeout = GPS_FIX_TIMEOUT_MS;
    if (st.gps_fails >= GPS_FAILS_FOR_SHORT) {
      st.gps_short_n++;
      if (st.gps_short_n % GPS_FULL_TRY_EVERY != 0) gpsTimeout = GPS_SHORT_TIMEOUT_MS;
    }
    if (GNSS_STANDBY && rtc_gnss_kept)
      Serial.printf("[FF] GNSS 대기에서 깸(직전 잠 %lu분, 온시동 기대 %s)\n", (unsigned long)st.last_sleep_m,
                    st.last_sleep_m <= GNSS_WARM_MAX_MIN ? "예" : "아니오");
    fixed = getFix(lat, lon, gpsTimeout);
    if (fixed) { st.gps_fails = 0; st.gps_short_n = 0; }
    else if (st.gps_fails < 255) st.gps_fails++;
  } else {
    Serial.println("[FF] GNSS 생략(이어 보내기)");
  }
  if (fixed) {
    if (gps.date.isValid() && gps.date.year() >= 2025) st.month = gps.date.month();
    PingRec r;
    pingRecInit(r);
    r.lat = lat; r.lon = lon;
    r.batt = readBatteryV();
    r.hdop = gps.hdop.isValid() ? (float)gps.hdop.hdop() : -1.0f;
    r.seq  = nextSeq();
    r.wake_mah_prev  = rtc_prev_wake_mah;
    r.wake_s_prev    = rtc_prev_wake_s;
    r.i_peak_ma_prev = rtc_prev_i_peak;
    gpsIso8601(r.ts_fix, sizeof(r.ts_fix));
    r.sats            = (uint8_t)(gps.satellites.value() > 255 ? 255 : gps.satellites.value());
    r.gnss_fix_s      = (uint16_t)(g_gnss_fix_s > 0xFFFE ? 0xFFFF : g_gnss_fix_s);
    r.solar_mv        = readSolarMv();
    r.reset_reason    = (uint8_t)g_reset;
    r.brownouts       = g_brownouts;
    r.net_stage_prev  = st.net_stage_prev;
    r.rsrp_prev       = st.rsrp_prev;
    r.rsrq_prev       = st.rsrq_prev;
    r.rssi_prev       = st.rssi_prev;
    r.cell_id_prev    = st.cell_id_prev;
    r.reg_s_prev      = st.reg_s_prev;
    r.vmin_tx_mv_prev = st.vmin_tx_mv_prev;

    // ── 회수 판정 ──
    uint8_t f = 0;
    bool outZone = (ZONE_RADIUS_M > 0) && distM(lat, lon, ZONE_LAT, ZONE_LON) > ZONE_RADIUS_M;
    if (outZone) f |= F_OUTZONE;
    if (!st.anchor_ok || distM(lat, lon, st.anchor_lat, st.anchor_lon) > STRAND_RADIUS_M) {
      st.anchor_lat = lat; st.anchor_lon = lon; st.anchor_min = 0; st.anchor_ok = true;
    } else {
      st.anchor_min += st.last_sleep_m;   // 같은 자리에 머문 시간 누적
    }
    if (st.anchor_min >= STRAND_MINUTES) f |= F_STRANDED;
    // 보 체류: 보 반경 안에 머문 누적 시간(좌초보다 넓은 반경이라 되돌이 흐름 안에서 오가도 센다)
    bool nearWeir = false;
    for (const auto &w : WEIRS)
      if (w.radius_m > 0 && distM(lat, lon, w.lat, w.lon) <= w.radius_m) { nearWeir = true; break; }
    if (nearWeir) st.weir_min += st.last_sleep_m; else st.weir_min = 0;
    bool weir = nearWeir && st.weir_min >= WEIR_DWELL_MIN;
    if (weir) f |= F_WEIR;
    // 지오펜스
    bool fenced = false;
    for (const auto &b : FENCES)
      if (lat >= b.lat_min && lat <= b.lat_max && lon >= b.lon_min && lon <= b.lon_max) { fenced = true; break; }
    if (fenced) f |= F_GEOFENCE;
    if (g_unexpected_reset) f |= F_RESET;
    if (r.batt > 1.0f && r.batt < lastReportV() && !nvsGetBool("lastsent")) {
      f |= F_LAST; nvsSetBool("lastsent", true); nvsSetBool("lowbatt", true);
    }
    if (r.batt > V_REVIVE && nvsGetBool("lowbatt")) {
      f |= F_REVIVED; nvsSetBool("lowbatt", false); nvsSetBool("lastsent", false);
    }
    if ((r.batt > 1.0f && r.batt < V_RECOVER) || (f & (F_OUTZONE | F_STRANDED | F_LAST | F_GEOFENCE))) f |= F_RECOVER;
    r.flags = f;
    g_next_sleep_m = launchIntervalM(chooseIntervalM(r.batt, outZone || fenced, weir), r.batt);
    r.interval_m = (uint16_t)g_next_sleep_m;

    storePush(r);
    st.last_stranded = (f & F_STRANDED);
    Serial.printf("[FF] fix seq=%lu lat=%.6f lon=%.6f hdop=%.1f sats=%u fix=%us batt=%.2fV solar=%umV flags=0x%02X next=%lum buf=%u\n",
                  (unsigned long)r.seq, lat, lon, r.hdop, (unsigned)r.sats, (unsigned)r.gnss_fix_s, r.batt,
                  (unsigned)r.solar_mv, f, (unsigned long)g_next_sleep_m, (unsigned)storeCount());
  } else if (!resumeTx) {
    Serial.println("[FF] GPS fix 실패(실내면 창가/야외로). 버퍼 있으면 전송만 시도");
  }

  if (storeCount() == 0) {         // 보낼 것도 없음 → 바로 슬립
    Serial.println("[FF] 전송할 레코드 없음 → 슬립");
    deepSleep();
  }

  // ★저전압이면 모뎀(2A 피크)을 켜지 않는다: fix는 버퍼에 남았으니 전압 회복 후 다음 주기에 몰아 보낸다.
  //   송신 도중 브라운아웃이 버퍼·seq를 통째로 날리는 것보다, 늦더라도 관측을 지키는 편이 낫다(재QA).
  //   v1.3: 11~3월 +0.10 V, 직전 송신 강하가 컸으면 +0.10 V, 보류 중이면 풀 때 +30 mV(히스테리시스).
  float vbat = readBatteryV();
  float skipV = skipTxV() + (st.tx_held ? V_HYST : 0.0f);
  if (vbat > 1.0f && vbat < skipV) {
    st.tx_held = true;
    Serial.printf("[FF] 저전압 %.2fV<%.2fV(%s) → 전송 보류(버퍼 %u건 보존), 슬립\n",
                  vbat, skipV, isWinter() ? "겨울 문턱" : "평시 문턱", (unsigned)storeCount());
    deepSleep();
  }
  st.tx_held = false;

  // ② 망 접속 → 버퍼 플러시
  // 접속 실패가 이어지면 모뎀 재시도 간격을 늘린다(좌초 의심이면 바로 최대치). 이어 보내기는 백오프를 건너뛴다.
  uint32_t backoffM = 0;
  if (st.last_net_fail && !resumeTx) {
    backoffM = NET_BACKOFF_START_MINUTES << min((int)st.net_fails - 1, 3);
    if (st.last_stranded || backoffM > STRANDED_TRY_MINUTES) backoffM = STRANDED_TRY_MINUTES;
  }
  if (backoffM && st.min_since_try < backoffM) {
    Serial.printf("[FF] 접속 실패 %u회 → 모뎀 생략(%lu/%lu분), 저장 %u건\n",
                  st.net_fails, (unsigned long)st.min_since_try, (unsigned long)backoffM, (unsigned)storeCount());
    deepSleep();
  }
  st.min_since_try = 0;
  nvsSetBool("txing", true);       // 여기서 리셋되면 다음 부팅이 「송신 중 끊김」으로 안다
  vmonStart();
  if (!netConnect()) {
    st.last_net_fail = true;
    if (st.net_fails < 255) st.net_fails++;
    vmonStop();
    st.vmin_tx_mv_prev = g_vmin_mv == 0xFFFF ? 0 : g_vmin_mv;
    nvsSetBool("txing", false);
    deepSleep();
  }
  st.last_net_fail = false;
  st.net_fails = 0;

  // ★★TLS-AUTH 훅(운영 HTTPS): 첫 connect 전에 CA를 심고 서버 인증서 검증을 켠다.
  //   아래는 하드웨어 검증 전이라 주석 처리 — README '보안·공급망' 절차대로 켜고 벤치에서
  //   잘못된 인증서 '거부'를 확인한 뒤에만 실데이터를 보낸다. authmode/SNI는 포크 SSL API에 따름.
  // #if !BENCH_HTTP
  //   if (PROD_CA_PEM[0]) client.setCACert(PROD_CA_PEM);   // 없으면 인증 미검증(MITM 취약)
  //   // + AT+CSSLCFG "authmode"=서버검증, "sni"/"servername"=SERVER_HOST (포크 SSL 설정)
  // #endif

  int sent = flushStore();
  Serial.printf("[FF] 전송 %d건, 잔여 %u건\n", sent, (unsigned)storeCount());

  netDisconnect();
  vmonStop();
  // 송신 강하 판정: 시작 전압 − 최저 전압이 DROOP_LIMIT_V 를 넘으면 다음 번 문턱을 올린다.
  st.vmin_tx_mv_prev = g_vmin_mv == 0xFFFF ? 0 : g_vmin_mv;
  if (st.vmin_tx_mv_prev && vbat > 1.0f) {
    float droop = vbat - st.vmin_tx_mv_prev / 1000.0f;
    st.tx_raise_v = droop > DROOP_LIMIT_V ? DROOP_RAISE_V : 0.0f;
    Serial.printf("[FF] 송신 중 최저 %.2fV(강하 %.2fV) → 다음 문턱 +%.2fV\n",
                  st.vmin_tx_mv_prev / 1000.0f, droop, st.tx_raise_v);
  }
  nvsSetBool("txing", false);
  Serial.println("[FF] 완료 → 딥슬립");
  deepSleep();
}

void loop() { /* 딥슬립 사용, loop 미사용 */ }
