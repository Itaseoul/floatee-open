/*
 * Friendly Floaty (SEA:CUT) 드리프터 펌웨어 — LTE Cat.1 bis (A7670) + L76K GPS
 * 대상 보드: LILYGO T-A7670G R2 (With GPS L76K), ESP32-WROVER-E
 * FF-ID 스키마 v1.1 (data/schema.md) 준수.
 *
 * 동작(v1.1, store-and-forward):
 *   깨어남 → L76K GPS fix(+UTC 시각) → 레코드를 RTC 버퍼에 적재(seq 부여)
 *         → 모뎀 전원 → 망 접속(Cat.1 bis) → 버퍼의 미전송 레코드를 오래된 것부터 POST
 *         → 성공분 제거 → 딥슬립.
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

// ── 통신 경로 토글 (2026-09-12 신설) ────────────────────────────────────────
// 1 = Wi-Fi(ESP32-WROVER-E 내장). ★유심 없이 시험한다. 사무실 공유기나 휴대폰 핫스팟.
// 0 = LTE(A7670G 모뎀 + 유심). 야외 자율 운용의 정본 경로.
//
// ★Wi-Fi 가 있는 이유: 이 보드의 MCU 가 ESP32 라 Wi-Fi 가 칩에 들어 있다. LTE 모뎀은
//   그 위에 얹힌 별개 부품이다. 그래서 유심이 오기 전에도 GPS 와 서버 왕복을 증명할 수 있다.
// ★한계: 드리프터를 띄워 보내면 수십 미터에서 Wi-Fi 가 끊긴다. 손에 들고 걷는 시험까지다.
//   물에 띄워 흘려보내는 것은 LTE 라야 한다.
#define USE_WIFI 1

// ── 전송 대상 토글 ──────────────────────────────────────────────────────────
// 1 = 벤치 검증(로컬 ingest_server.py, HTTP 평문, /api/ping).  ★처음엔 이걸로.
// 0 = 운영 전송(floatee.caresea.kr, HTTPS 443, /api/drift/ping). ★2026-09-12 openc 에서 옮김.
#define BENCH_HTTP 1

// ★조합 넷 — 무엇을 증명하려는지에 따라 고른다.
//   USE_WIFI 1 · BENCH_HTTP 1 → 같은 랜의 PC 사설 IP. **ngrok 이 필요 없다.** 첫 시험은 여기서.
//   USE_WIFI 1 · BENCH_HTTP 0 → 휴대폰 핫스팟으로 floatee. 하천에 들고 나갈 때.
//   USE_WIFI 0 · BENCH_HTTP 1 → ngrok tcp 로 로컬. LTE 자체를 평문으로 검증.
//   USE_WIFI 0 · BENCH_HTTP 0 → 운영. 방류.

// ── CONFIG ────────────────────────────────────────────────────────────────
#define DEVICE_ID     "ff-kr-bs-u0001"        // 기기 식별자(유닛 메타디렉토리 unit_id와 짝)
#define SITE_ID       "nakdong-hakjang"       // 사이트 태그(서버가 궤적에 태그)
const char* APN       = "";                   // 유심 사업자 APN. 꽂은 유심에 맞게 채운다.
                                              //  Soracom(권장 1대 PoC): "soracom.io"  (USER "sora" / PASS "sora"). 한국 KT/SKT 로밍.
                                              //  1NCE:                   "iot.1nce.net" (USER/PASS 없음). 한국 KT/SKT 로밍.
                                              //  국내 KT 알뜰폰 데이터심:  "lte.ktfwing.com"   (USER/PASS 없음)
                                              //  국내 SKT 알뜰폰 데이터심: "lte.sktelecom.com" (USER/PASS 없음)
                                              //  ※ 1NCE/Soracom 모두 LG U+ 로밍 미지원. 글로벌 IoT SIM은 KT 또는 SKT로만 붙는다.
const char* GPRS_USER = "";                   // Soracom이면 "sora"
const char* GPRS_PASS = "";                   // Soracom이면 "sora"

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

// ── 저전압 가드(재QA: LTE 송신 2A 피크가 방전 셀에서 브라운아웃을 일으켜 버퍼·seq를 날린다) ──
// 임계값은 벤치 실측(T3 냉수·부하시험)으로 확정한다. 아래는 18650 방전곡선 기준 보수적 초깃값.
const float    BATT_SKIP_TX_V = 3.50f;   // 이 이하: GPS fix는 남기되 모뎀(2A 피크) 생략 → 다음 주기에 몰아 전송
const float    BATT_PARK_V    = 3.30f;   // 이 이하: fix도 생략하고 장주기 park(심방전·셀 손상 방지)
const uint32_t PARK_MULT      = 4;       // park 시 슬립 배수(30분×4=2시간). 전압 회복 대기
// ───────────────────────────────────────────────────────────────────────────

// ── 전압 기반 간격 + 회수 모드 (2026-09-26 추가, 설계서 「투하 전 점검과 회수 펌웨어」) ──
// 전압이 곧 남은 충전량이다. 깰 때마다 재서 다음 잠 시간을 정한다. 값은 벤치 뒤 조정.
const bool     ADAPTIVE_INTERVAL = true;  // false면 SLEEP_MINUTES 고정(벤치용)
const float    V_TIER_1H   = 3.90f;       // 이상: 60분
const float    V_TIER_2H   = 3.70f;       // 이상: 120분
const float    V_TIER_6H   = 3.50f;       // 이상: 360분, 미만: 720분
const float    V_RECOVER   = 3.60f;       // 미만이면 보고에 회수 요청 표시
const float    V_LAST      = 3.55f;       // 이 아래로 처음 내려가면 「마지막 보고」 1회(3.50 전송 보류 직전)
const float    V_REVIVE    = 3.75f;       // 저전압 기록이 있는 기기가 이 위로 회복하면 「되살아남」 보고
// 회수 구역(원). 반경 0이면 끈다. 방류 지점마다 굽는다(부산 다대포 예: 35.046, 128.964).
const double   ZONE_LAT = 0.0, ZONE_LON = 0.0;
const float    ZONE_RADIUS_M      = 0.0f;
const uint32_t OUT_ZONE_MINUTES   = 30;   // 구역 밖이고 전압 넉넉하면 30분 간격
// 좌초 의심: 연속 보고 위치가 이 반경 안에 이 시간 이상 머묾
const float    STRAND_RADIUS_M    = 50.0f;
const uint32_t STRAND_MINUTES     = 360;  // 6시간
// 보고 플래그(서버 스키마 flags 비트)
#define F_RECOVER   0x01   // 회수 요청
#define F_OUTZONE   0x02   // 구역 이탈
#define F_STRANDED  0x04   // 좌초 의심
#define F_LAST      0x08   // 마지막 보고(이후 스스로 멈춤 가능)
#define F_REVIVED   0x10   // 저전압 뒤 되살아남
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
#define BOARD_GPS_WAKEUP_PIN  19   // L76K wakeup(공식 예제는 미토글, 여기선 방어적 HIGH)
#define GPS_BAUDRATE          9600

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
Preferences    prefs;
bool           modem_on = false;   // SerialAT.begin 전 modem.poweroff() 방지 가드

// ── FF-ID v1.1 store-and-forward 버퍼 (RTC slow memory: 딥슬립 생존, 전원상실 시 소실) ──
// 전원상실(브라운아웃·배터리 교체)엔 버퍼가 사라진다 — seq는 NVS로 복원되므로 서버가
// seq 갭으로 결측을 안다(스키마 §4). NVS에 매 레코드 저장은 마모·복잡도 대비 이득이 작아
// 채택하지 않는다(정직 트레이드오프). ★저전압 가드가 송신 브라운아웃을 선제 차단해 이 소실을 줄인다.
struct PingRec {
  double   lat, lon;
  float    batt;
  float    hdop;        // <0 = 무효(필드 생략)
  uint32_t seq;
  char     ts_fix[24];  // GPS UTC ISO8601. 빈 문자열 = 시각 무효
  uint8_t  flags;       // F_* 비트
  uint16_t interval_m;  // 이 레코드 다음 잠 시간(분)
};
#define BUF_MAX 16
RTC_DATA_ATTR PingRec  rtc_buf[BUF_MAX];
RTC_DATA_ATTR uint8_t  rtc_buf_n = 0;
RTC_DATA_ATTR uint32_t rtc_seq   = 0;   // 단조증가 레코드 카운터(딥슬립 생존)
// 좌초 판정용 기준점(딥슬립 생존). 기준점에서 STRAND_RADIUS_M 안에 머문 누적 분.
RTC_DATA_ATTR double   rtc_anchor_lat = 0, rtc_anchor_lon = 0;
RTC_DATA_ATTR uint32_t rtc_anchor_min = 0;
RTC_DATA_ATTR bool     rtc_anchor_ok  = false;
RTC_DATA_ATTR uint32_t rtc_last_sleep_m = 0;   // 직전 잠 시간(누적 계산용)

// 두 좌표 거리(m), 하버사인
double distM(double la1, double lo1, double la2, double lo2) {
  const double R = 6371000.0, d2r = PI / 180.0;
  double dla = (la2 - la1) * d2r, dlo = (lo2 - lo1) * d2r;
  double a = sin(dla/2)*sin(dla/2) + cos(la1*d2r)*cos(la2*d2r)*sin(dlo/2)*sin(dlo/2);
  return 2 * R * atan2(sqrt(a), sqrt(1 - a));
}

// 저전압 기록은 NVS에 둔다: 보호회로가 전원을 끊으면 RTC 메모리가 사라지기 때문
bool nvsGetBool(const char* k) { prefs.begin("ffid", true); bool v = prefs.getBool(k, false); prefs.end(); return v; }
void nvsSetBool(const char* k, bool v) { prefs.begin("ffid", false); prefs.putBool(k, v); prefs.end(); }

// 전압(과 구역 이탈)으로 다음 잠 시간을 정한다
uint32_t chooseIntervalM(float v, bool outZone) {
  if (!ADAPTIVE_INTERVAL || v < 1.0f) return SLEEP_MINUTES;   // USB 급전·측정 무효면 고정
  if (v >= V_TIER_1H) return outZone ? OUT_ZONE_MINUTES : 60;
  if (v >= V_TIER_2H) return 120;
  if (v >= V_TIER_6H) return 360;
  return 720;
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
String buildPingBody(const PingRec &r) {
  String b = String("{\"device_id\":\"") + DEVICE_ID + "\",\"site_id\":\"" + SITE_ID + "\"";
  b += ",\"seq\":" + String(r.seq);
  b += ",\"lat\":" + String(r.lat, 6) + ",\"lon\":" + String(r.lon, 6);
  b += ",\"batt\":" + String(r.batt, 2);
  b += ",\"sample_interval_s\":" + String((uint32_t)r.interval_m * 60);
  b += ",\"flags\":" + String(r.flags);   // F_* 비트. 서버는 0 이면 평상으로 읽는다
  if (r.hdop >= 0) b += ",\"fix_quality\":" + String(r.hdop, 1);
  b += ",\"gnss_source\":\"l76k\"";
  if (r.ts_fix[0]) { b += ",\"ts_fix\":\"" + String(r.ts_fix) + "\",\"ts\":\"" + String(r.ts_fix) + "\""; }
  b += "}";
  return b;
}

// 배터리 전압(V): 100k/100k 분압 가정. analogReadMilliVolts로 eFuse Vref 교정(재QA #3:
// raw*3.3/4095는 ADC 비선형·Vref 편차로 저전압 판정이 수십 mV 어긋난다 → 밀리볼트 API가 교정 내장).
// 저전압 컷오프가 이 값에 걸리므로 교정이 가드의 전제다.
float readBatteryV() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(BOARD_BAT_ADC);
  mv /= 16;
  return (mv / 1000.0f) * 2.0f;  // 분압 2배
}

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

// 망에 붙는다. Wi-Fi 와 LTE 의 차이를 여기 한 곳에 가두어 setup 이 같은 모양을 유지하게 한다.
// 실패하면 false — 호출부는 관측을 버퍼에 남긴 채 슬립한다(store-and-forward).
bool netConnect() {
#if USE_WIFI
  if (!WIFI_SSID[0]) { Serial.println("[FF] WIFI_SSID 가 비었다"); return false; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[FF] Wi-Fi 접속 시도 %s\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 30000) delay(200);
  if (WiFi.status() != WL_CONNECTED) {
    // ★2.4 GHz 인지 먼저 본다. ESP32 는 5 GHz 를 아예 잡지 못하고, 휴대폰 핫스팟은
    //   기본이 5 GHz 인 기종이 있다(설정에 대역 항목이 있다).
    Serial.println("[FF] Wi-Fi 실패(2.4GHz·비밀번호 확인) → 버퍼 보존, 슬립");
    return false;
  }
  Serial.print("[FF] 접속 IP "); Serial.println(WiFi.localIP());
  #if !BENCH_HTTP
  // 벤치 단계에서는 서버 인증서를 검증하지 않는다. 루트 CA 를 굽는 것은 운영(LTE) 경로의
  // 과제이고, 여기서 막히면 정작 보려던 GPS·왕복 검증이 멈춘다.
  // ★운영 방류에는 이 설정을 쓰지 않는다.
  client.setInsecure();
  #endif
  return true;
#else
  modemPowerOn();
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);  // 공식 예제와 동일(RX,TX 순서)
  delay(3000);
  Serial.println("[FF] 모뎀 초기화");
  if (!modem.testAT(10000)) {
    // 무응답이면: (1) 18650 배터리 장착(USB만 급전 금지) (2) BOARD_POWERON HIGH (3) baud 115200
    Serial.println("[FF] 모뎀 무응답 → 관측은 버퍼에 보존, 슬립");
    return false;
  }
  if (!modem.waitForNetwork(60000)) { Serial.println("[FF] 망 등록 실패 → 버퍼 보존, 슬립"); return false; }
  if (!modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) { Serial.println("[FF] PDP 실패(APN 확인) → 버퍼 보존, 슬립"); return false; }
  Serial.print("[FF] 접속 IP "); Serial.println(modem.getLocalIP());
  return true;
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

// L76K GPS(Serial2)에서 NMEA를 읽어 위경도 취득. 성공 시 true.
// ★모뎀 AT가 아니라 L76K UART다. 실내에서는 대부분 fix 안 됨 → 창가/야외에서 확인.
// ★GPS 전원은 BOARD_POWERON 레일에 걸려 있어 fix 전에 POWERON을 먼저 HIGH로 올린다.
bool getFix(double &lat, double &lon, uint32_t timeoutMs = GPS_FIX_TIMEOUT_MS) {
  pinMode(BOARD_POWERON, OUTPUT); digitalWrite(BOARD_POWERON, HIGH);  // GPS 레일 확보(모뎀보다 먼저 필요)
  pinMode(BOARD_GPS_WAKEUP_PIN, OUTPUT); digitalWrite(BOARD_GPS_WAKEUP_PIN, HIGH);
  SerialGPS.begin(GPS_BAUDRATE, SERIAL_8N1, BOARD_GPS_RX_PIN, BOARD_GPS_TX_PIN);
  uint32_t start = millis();
  while (millis() - start < timeoutMs) {
    while (SerialGPS.available()) gps.encode(SerialGPS.read());
    bool locOk  = gps.location.isValid() && gps.location.age() < 3000 && gps.satellites.value() >= 4;
    bool timeOk = gps.date.isValid() && gps.time.isValid() && gps.date.year() >= 2025;
    if (locOk && timeOk) {           // RMC(날짜)는 같은 에포크에 오므로 지연 최대 ~1초.
      lat = gps.location.lat();      // 날짜 검증 없이 리턴하면 ts_fix가 비어 서버가 server_recv로
      lon = gps.location.lng();      // 강등 — 버퍼 지연 레코드는 관측시각이 통째로 틀어짐(검수 발견)
      return true;
    }
    delay(10);
  }
  // 타임아웃 폴백: 위치만이라도 유효하면 관측은 살린다(ts_fix만 결측 → 서버가 server_recv 강등)
  if (gps.location.isValid() && gps.location.age() < 3000 && gps.satellites.value() >= 4) {
    lat = gps.location.lat(); lon = gps.location.lng();
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

// 버퍼 적재. 가득 차면 가장 오래된 것을 버리고 최신을 지킨다(현재 위치 우선, 스키마 §4가
// seq 갭으로 결측을 안다). GDP식 운명 기록은 서버·정산 로그가 담당.
void bufPush(const PingRec &r) {
  if (rtc_buf_n >= BUF_MAX) {
    memmove(&rtc_buf[0], &rtc_buf[1], sizeof(PingRec) * (BUF_MAX - 1));
    rtc_buf_n = BUF_MAX - 1;
  }
  rtc_buf[rtc_buf_n++] = r;
}

// 버퍼 앞에서부터(오래된 순) POST. 2xx면 제거, 실패하면 중단(남은 건 다음 웨이크에).
// 반환: 보낸 개수.
int flushBuffer() {
  int sent = 0;
  while (rtc_buf_n > 0) {
    String body = buildPingBody(rtc_buf[0]);
    Serial.println("[FF] POST " + body);
    http.beginRequest();
    http.post(SERVER_PATH);
    http.sendHeader("Content-Type", "application/json");
    http.sendHeader("Content-Length", body.length());
    http.beginBody();
    http.print(body);
    http.endRequest();
    int status = http.responseStatusCode();
    if (status > 0) http.responseBody();   // 유효 응답만 소진(오류코드면 재차 30초 대기 회피)
    Serial.print("[FF] 서버 응답 "); Serial.println(status);
    if (status >= 200 && status < 300) {
      memmove(&rtc_buf[0], &rtc_buf[1], sizeof(PingRec) * (rtc_buf_n - 1));
      rtc_buf_n--; sent++;
    } else {
      http.stop();
      break;               // 실패: 배터리 아끼고 다음 주기에 재시도
    }
  }
  return sent;
}

// 지정 분(minutes)만큼 딥슬립. 모뎀·L76K 전원을 끊고(POWERON LOW) GPIO를 홀드해 슬립 중
// 누설을 막는다 — L76K 저전력화는 별도 백업명령이 아니라 이 POWERON 레일 차단으로 달성(절감폭 실측 전).
void deepSleepFor(uint32_t minutes) {
#if USE_WIFI
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);   // 슬립 전 라디오를 끈다. 안 끄면 딥슬립 진입이 늦고 전류가 남는다
#else
  if (modem_on) modem.poweroff();
#endif
  // 딥슬립 중 일반 GPIO는 플로팅 → GPS_WAKEUP이 HIGH로 남으면 L76K가 슬립 내내 켜져
  // 배터리를 지배할 수 있다(공식 DeepSleep 예제 방식으로 홀드). 절감폭은 실측 전.
  pinMode(BOARD_GPS_WAKEUP_PIN, OUTPUT); digitalWrite(BOARD_GPS_WAKEUP_PIN, LOW);
  gpio_hold_en((gpio_num_t)BOARD_GPS_WAKEUP_PIN);
  pinMode(MODEM_RST, OUTPUT);            digitalWrite(MODEM_RST, LOW);
  gpio_hold_en((gpio_num_t)MODEM_RST);
  pinMode(BOARD_POWERON, OUTPUT);        digitalWrite(BOARD_POWERON, LOW);
  gpio_hold_en((gpio_num_t)BOARD_POWERON);
  gpio_deep_sleep_hold_en();
  rtc_last_sleep_m = minutes;
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
  gpio_hold_dis((gpio_num_t)BOARD_POWERON);
  Serial.begin(115200);
  analogReadResolution(12);
  // 리셋 사유 로깅(재QA #5): 브라운아웃(ESP_RST_BROWNOUT=6)이 반복되면 저전압 가드·전원설계 재검토 신호
  Serial.printf("[FF] reset_reason=%d (3=SW 4=panic 6=BROWNOUT 8=deepsleep)\n", (int)esp_reset_reason());

  // ★저전압 park(모뎀·GPS 켜기 전에 먼저): 심방전 구간이면 아무 동작 없이 장주기로 자 셀을 보호한다.
  float vbat0 = readBatteryV();
  if (vbat0 > 1.0f && vbat0 < BATT_PARK_V) {   // >1.0V = 측정 유효(USB 급전·미장착 오검 배제)
    uint32_t parkM = ADAPTIVE_INTERVAL ? 720 : SLEEP_MINUTES * PARK_MULT;
    Serial.printf("[FF] 저전압 park %.2fV<%.2fV → %lu분 슬립\n", vbat0, BATT_PARK_V, (unsigned long)parkM);
    nvsSetBool("lowbatt", true);           // 되살아남 판정용(보호회로 차단으로 RTC가 지워져도 남는다)
    deepSleepFor(parkM);
  }
  g_next_sleep_m = chooseIntervalM(vbat0, false);

  // ① GPS fix 먼저(망 없어도 관측은 남긴다 — store-and-forward의 요점)
  double lat = 0, lon = 0;
  bool fixed = getFix(lat, lon);
  if (fixed) {
    PingRec r;
    r.lat = lat; r.lon = lon;
    r.batt = readBatteryV();
    r.hdop = gps.hdop.isValid() ? (float)gps.hdop.hdop() : -1.0f;
    r.seq  = nextSeq();
    gpsIso8601(r.ts_fix, sizeof(r.ts_fix));

    // ── 회수 판정 ──
    uint8_t f = 0;
    bool outZone = (ZONE_RADIUS_M > 0) && distM(lat, lon, ZONE_LAT, ZONE_LON) > ZONE_RADIUS_M;
    if (outZone) f |= F_OUTZONE;
    if (!rtc_anchor_ok || distM(lat, lon, rtc_anchor_lat, rtc_anchor_lon) > STRAND_RADIUS_M) {
      rtc_anchor_lat = lat; rtc_anchor_lon = lon; rtc_anchor_min = 0; rtc_anchor_ok = true;
    } else {
      rtc_anchor_min += rtc_last_sleep_m;   // 같은 자리에 머문 시간 누적
    }
    if (rtc_anchor_min >= STRAND_MINUTES) f |= F_STRANDED;
    if (r.batt > 1.0f && r.batt < V_LAST && !nvsGetBool("lastsent")) {
      f |= F_LAST; nvsSetBool("lastsent", true); nvsSetBool("lowbatt", true);
    }
    if (r.batt > V_REVIVE && nvsGetBool("lowbatt")) {
      f |= F_REVIVED; nvsSetBool("lowbatt", false); nvsSetBool("lastsent", false);
    }
    if ((r.batt > 1.0f && r.batt < V_RECOVER) || (f & (F_OUTZONE | F_STRANDED | F_LAST))) f |= F_RECOVER;
    r.flags = f;
    g_next_sleep_m = chooseIntervalM(r.batt, outZone);
    r.interval_m = (uint16_t)g_next_sleep_m;

    bufPush(r);
    Serial.printf("[FF] fix seq=%lu lat=%.6f lon=%.6f hdop=%.1f sats=%lu batt=%.2fV flags=0x%02X next=%lum buf=%u\n",
                  (unsigned long)r.seq, lat, lon, r.hdop, gps.satellites.value(), r.batt, f,
                  (unsigned long)g_next_sleep_m, rtc_buf_n);
  } else {
    Serial.println("[FF] GPS fix 실패(실내면 창가/야외로). 버퍼 있으면 전송만 시도");
  }

  if (rtc_buf_n == 0) {         // 보낼 것도 없음 → 바로 슬립
    Serial.println("[FF] 전송할 레코드 없음 → 슬립");
    deepSleep();
  }

  // ★저전압이면 모뎀(2A 피크)을 켜지 않는다: fix는 버퍼에 남았으니 전압 회복 후 다음 주기에 몰아 보낸다.
  //   송신 도중 브라운아웃이 버퍼·seq를 통째로 날리는 것보다, 늦더라도 관측을 지키는 편이 낫다(재QA).
  float vbat = readBatteryV();
  if (vbat > 1.0f && vbat < BATT_SKIP_TX_V) {
    Serial.printf("[FF] 저전압 %.2fV<%.2fV → 전송 보류(버퍼 %u건 보존), 슬립\n",
                  vbat, BATT_SKIP_TX_V, rtc_buf_n);
    deepSleep();
  }

  // ② 망 접속 → 버퍼 플러시
  if (!netConnect()) deepSleep();

  // ★★TLS-AUTH 훅(운영 HTTPS): 첫 connect 전에 CA를 심고 서버 인증서 검증을 켠다.
  //   아래는 하드웨어 검증 전이라 주석 처리 — README '보안·공급망' 절차대로 켜고 벤치에서
  //   잘못된 인증서 '거부'를 확인한 뒤에만 실데이터를 보낸다. authmode/SNI는 포크 SSL API에 따름.
  // #if !BENCH_HTTP
  //   if (PROD_CA_PEM[0]) client.setCACert(PROD_CA_PEM);   // 없으면 인증 미검증(MITM 취약)
  //   // + AT+CSSLCFG "authmode"=서버검증, "sni"/"servername"=SERVER_HOST (포크 SSL 설정)
  // #endif

  int sent = flushBuffer();
  Serial.printf("[FF] 전송 %d건, 잔여 %u건\n", sent, rtc_buf_n);

  netDisconnect();
  Serial.println("[FF] 완료 → 딥슬립");
  deepSleep();
}

void loop() { /* 딥슬립 사용, loop 미사용 */ }
