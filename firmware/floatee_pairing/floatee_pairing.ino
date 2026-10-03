// Floatee 기기 등록 스케치 (LILYGO T-A7670G R2)
// 기기 ID 와 등록 코드를 만들어 두고, floatee.caresea.kr/start 의 「USB 로 기기 연결」 질의에 답한다.
// 통신 모듈 · GPS 는 켜지 않는다. 등록을 마친 뒤 본 펌웨어(drifter_a7670_cat1)를 올린다.
//
// 쓰는 법
//   1) 이 스케치를 올린다(보드: ESP32 Dev Module).
//   2) 시리얼 모니터를 닫는다(웹 화면이 같은 포트를 써야 한다).
//   3) floatee.caresea.kr/start 에서 「USB 로 기기 연결」을 누른다.
//   브라우저 USB 연결이 안 되면: 시리얼 모니터(115200)에 나오는 등록 주소를 복사해 브라우저에 붙인다.
#include "floatee_identity.h"

#define FW_VERSION "pairing-1.0"

floatee::Identity ident;
uint32_t lastHint = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  floatee::loadOrCreate(ident);
  Serial.println();
  Serial.println("[FLOATEE] 기기 등록 모드");
  Serial.printf("[FLOATEE] 기기 ID  %s\n", ident.device_id);
  // 등록 주소는 # 뒤에 값을 담아 서버 로그에 남지 않게 한다
  Serial.printf("[FLOATEE] 등록 주소 https://floatee.caresea.kr/start#d=%s&k=%s\n", ident.device_id, ident.secret);
  Serial.println("[FLOATEE] 등록 코드는 비밀번호와 같습니다. 사진이나 채팅에 올리지 마세요.");
}

void loop() {
  floatee::pollSerial(Serial, ident, FW_VERSION);
  if (millis() - lastHint > 15000) {
    lastHint = millis();
    Serial.println("[FLOATEE] 대기 중 · 웹에서 「USB 로 기기 연결」을 누르세요");
  }
  delay(10);
}
