/*
 * sim_check — 유심 하나를 꽂아 「이 보드가 이 망에 붙는가」만 확인하는 시험 스케치
 * 대상 보드: LILYGO T-A7670G R2 (ESP32-WROVER-E). 핀과 전원 순서는 drifter_a7670_cat1 과 같다.
 *
 * 하는 일(자동, 시리얼 모니터 115200 에 결과를 찍는다)
 *   1. 모뎀 전원을 켜고 AT 응답을 기다린다
 *   2. 유심 인식(AT+CPIN?) · 유심 번호(AT+CICCID) · 모뎀 IMEI(AT+CGSN)
 *   3. 망 등록(AT+CEREG?)을 최대 3분 기다린다. 1 = 집 망, 5 = 로밍. 둘 다 성공
 *   4. APN 을 넣고 데이터 연결(AT+CGACT) → 받은 IP(AT+CGPADDR)
 *   5. 평문 HTTP GET 한 번(http://example.com). 200 이 나오면 데이터가 실제로 오간 것
 * 끝나면 시리얼 모니터에서 AT 명령을 직접 쳐서 모뎀에 보낼 수 있다(패스스루).
 *
 * 꽂기 전: 보드 전원을 끄고 · LTE 안테나를 먼저 연결하고 · 유심 PIN 잠금과
 *          통신사 유심보호서비스(다른 기기 사용 차단)를 끈다.
 * 서버가 필요 없다. 우리 서버 없이 망과 데이터만 본다.
 */

// ── 꽂은 유심의 망에 맞게 하나만 남긴다 ──
const char* APN = "internet.lguplus.co.kr";   // LG U+ (헬로모바일 등 U+ 망 알뜰폰 포함)
// const char* APN = "lte.sktelecom.com";     // SKT 망
// const char* APN = "lte.ktfwing.com";       // KT 망

#define MODEM_TX       26
#define MODEM_RX       27
#define BOARD_PWRKEY    4
#define BOARD_POWERON  12
#define MODEM_RST       5
#define SerialAT  Serial1

String at(const String& cmd, uint32_t waitMs = 2000, const char* until = "OK") {
  while (SerialAT.available()) SerialAT.read();
  SerialAT.println(cmd);
  String r; uint32_t t0 = millis();
  while (millis() - t0 < waitMs) {
    while (SerialAT.available()) r += (char)SerialAT.read();
    if (r.indexOf(until) >= 0 || r.indexOf("ERROR") >= 0) break;
  }
  r.trim();
  Serial.printf(">> %s\n%s\n", cmd.c_str(), r.c_str());
  return r;
}

void modemPowerOn() {
  pinMode(BOARD_POWERON, OUTPUT); digitalWrite(BOARD_POWERON, HIGH);
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);  delay(100);
  digitalWrite(MODEM_RST, HIGH); delay(2600);
  digitalWrite(MODEM_RST, LOW);
  pinMode(BOARD_PWRKEY, OUTPUT);
  digitalWrite(BOARD_PWRKEY, LOW);  delay(100);
  digitalWrite(BOARD_PWRKEY, HIGH); delay(1000);
  digitalWrite(BOARD_PWRKEY, LOW);
}

void verdict(const char* step, bool ok) {
  Serial.printf("\n[결과] %-18s %s\n\n", step, ok ? "통과" : "실패");
}

void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("\n=== sim_check 시작 ===");
  modemPowerOn();
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  bool ok = false;
  for (int i = 0; i < 30 && !ok; i++) { ok = at("AT", 1000).indexOf("OK") >= 0; }
  verdict("모뎀 응답", ok); if (!ok) return;

  at("ATE0"); at("ATI"); at("AT+CGSN");
  ok = at("AT+CPIN?", 5000, "READY").indexOf("READY") >= 0;
  verdict("유심 인식", ok);
  if (!ok) { Serial.println("  SIM PIN 이 걸렸거나 유심이 제대로 안 들어갔다"); return; }
  at("AT+CICCID");

  at("AT+CGDCONT=1,\"IP\",\"" + String(APN) + "\"");
  ok = false;
  for (int i = 0; i < 36 && !ok; i++) {        // 5초 × 36 = 3분
    String r = at("AT+CEREG?", 2000);
    ok = r.indexOf(",1") >= 0 || r.indexOf(",5") >= 0;
    if (!ok) { at("AT+CSQ"); delay(5000); }
  }
  at("AT+COPS?");
  verdict("망 등록", ok);
  if (!ok) { Serial.println("  CEREG 가 0,3 이면 망이 거부한 것(IMEI·유심 문제), 0,2 에 머물면 신호 문제"); return; }

  at("AT+CGATT?");
  at("AT+CGACT=1,1", 15000);
  ok = at("AT+CGPADDR=1").indexOf(".") >= 0;
  verdict("데이터 연결(IP)", ok); if (!ok) return;

  at("AT+HTTPINIT");
  at("AT+HTTPPARA=\"URL\",\"http://example.com\"");
  String r = at("AT+HTTPACTION=0", 30000, "+HTTPACTION:");
  ok = r.indexOf(",200,") >= 0;
  at("AT+HTTPTERM");
  verdict("HTTP 200", ok);

  Serial.println("=== 끝. 이제 AT 명령을 직접 칠 수 있다 ===");
}

void loop() {
  while (Serial.available())   SerialAT.write(Serial.read());
  while (SerialAT.available()) Serial.write(SerialAT.read());
}
