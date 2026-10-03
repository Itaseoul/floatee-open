// Floatee GPS 벤치 진단 (LILYGO T-A7670G R2)
// 통신 모듈은 켜지 않고 GPS 전원만 올린 뒤, 2초마다 수신 상태를 시리얼(115200 bps)로 보고한다.
//
// 보는 값
//   chars    GPS 모듈에서 받은 글자 수. 계속 늘면 GPS 전원·배선 정상. 0 이면 멈춘다.
//   inView   하늘에 보이는 위성 수          tracked  신호가 잡힌 위성 수
//   maxSnr   가장 센 신호(dB-Hz)            sats     좌표 계산에 쓰인 위성 수(4 이상이면 좌표)
//   loc      OK 가 되면 lat·lon 이 함께 나온다
//   | GP:보임/잡힘/최대신호  GL:…  BD:…  위성 체계별 값(GPS·GLONASS·BeiDou)
//
// 핀은 drifter_a7670_cat1.ino 와 같다. 필요한 라이브러리: TinyGPSPlus
// 보드 설정: ESP32 Dev Module. 출고 펌웨어를 지우고 올라가며, 본 펌웨어는 drifter_a7670_cat1 에 있다.
#include <TinyGPSPlus.h>

#define BOARD_POWERON         12   // 모뎀·GPS 공용 전원 게이트
#define BOARD_GPS_RX_PIN      22   // ESP32 가 NMEA 를 받는 핀
#define BOARD_GPS_TX_PIN      21
#define BOARD_GPS_WAKEUP_PIN  19
#define BOARD_BAT_ADC         35
#define GPS_BAUDRATE          9600

TinyGPSPlus gps;
uint32_t lastReport = 0;
String nmea;

// 위성 체계(GP·GL·BD·GA 등)별로 가장 최근 GSV 한 주기의 값을 보관한다
struct Talker { char id[3]; int inView; int tracked; int maxSnr; };
Talker tk[6];
int ntk = 0;

Talker* getTalker(const String &t) {
  for (int i = 0; i < ntk; i++) if (t == tk[i].id) return &tk[i];
  if (ntk >= 6) return nullptr;
  strncpy(tk[ntk].id, t.c_str(), 2); tk[ntk].id[2] = 0;
  tk[ntk].inView = tk[ntk].tracked = tk[ntk].maxSnr = 0;
  return &tk[ntk++];
}

void parseGsv(const String &s) {
  int star = s.indexOf('*');
  String body = star > 0 ? s.substring(0, star) : s;
  String f[40]; int nf = 0, start = 0;
  while (nf < 40) {
    int c = body.indexOf(',', start);
    if (c < 0) { f[nf++] = body.substring(start); break; }
    f[nf++] = body.substring(start, c);
    start = c + 1;
  }
  if (nf < 4) return;
  Talker *t = getTalker(s.substring(1, 3));
  if (!t) return;
  if (f[2].toInt() == 1) { t->inView = f[3].toInt(); t->tracked = 0; t->maxSnr = 0; }  // 새 주기 시작
  for (int k = 4; k + 3 < nf; k += 4) {
    int snr = f[k + 3].length() ? f[k + 3].toInt() : 0;
    if (snr > 0) { t->tracked++; if (snr > t->maxSnr) t->maxSnr = snr; }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("[DIAG] Floatee GPS diag start (modem stays off)");
  pinMode(BOARD_POWERON, OUTPUT);        digitalWrite(BOARD_POWERON, HIGH);
  pinMode(BOARD_GPS_WAKEUP_PIN, OUTPUT); digitalWrite(BOARD_GPS_WAKEUP_PIN, HIGH);
  Serial2.begin(GPS_BAUDRATE, SERIAL_8N1, BOARD_GPS_RX_PIN, BOARD_GPS_TX_PIN);
}

void loop() {
  while (Serial2.available()) {
    char c = Serial2.read();
    gps.encode(c);
    if (c == '\n') {
      if (nmea.length() > 6 && nmea[0] == '$' && nmea.substring(3, 6) == "GSV") parseGsv(nmea);
      nmea = "";
    } else if (c != '\r' && nmea.length() < 120) {
      nmea += c;
    }
  }
  if (millis() - lastReport >= 2000) {
    lastReport = millis();
    int inView = 0, tracked = 0, maxSnr = 0;
    String per;
    for (int i = 0; i < ntk; i++) {
      inView += tk[i].inView; tracked += tk[i].tracked;
      if (tk[i].maxSnr > maxSnr) maxSnr = tk[i].maxSnr;
      per += String(" ") + tk[i].id + ":" + tk[i].inView + "/" + tk[i].tracked + "/" + tk[i].maxSnr;
    }
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(BOARD_BAT_ADC);
    mv /= 8;
    Serial.printf("[DIAG] t=%lus chars=%lu sats=%lu inView=%d tracked=%d maxSnr=%d hdop=%.1f loc=%s",
                  (unsigned long)(millis() / 1000), (unsigned long)gps.charsProcessed(),
                  (unsigned long)gps.satellites.value(), inView, tracked, maxSnr,
                  gps.hdop.isValid() ? gps.hdop.hdop() : -1.0, gps.location.isValid() ? "OK" : "--");
    if (gps.location.isValid()) Serial.printf(" lat=%.6f lon=%.6f", gps.location.lat(), gps.location.lng());
    if (gps.time.isValid()) Serial.printf(" utc=%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
    Serial.printf(" batt_adc=%.2fV |%s\n", mv / 1000.0 * 2.0, per.c_str());
  }
}
