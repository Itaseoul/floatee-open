// Floatee 기기 신원 — 기기 ID 와 등록 코드(비밀값)를 만들고 보관하며, PC 설치 화면의 질의에 답한다.
//
//  기기 ID   : "ff-" + 칩 고유번호(eFuse MAC) 12자리 hex. 펌웨어를 다시 올려도 바뀌지 않는다.
//  등록 코드 : 첫 부팅 때 하드웨어 난수 24바이트를 base64url 32자로 만들어 NVS("floatee" 네임스페이스)에 둔다.
//              펌웨어를 다시 올려도 유지되고, 플래시 전체 지우기(erase_flash)를 하면 새로 만들어진다.
//              → 그때는 웹에서 기기 등록을 해제한 뒤 다시 등록한다.
//  시리얼 질의 : 115200 bps 로 "FLOATEE INFO" 한 줄을 받으면
//              {"device_id":"ff-…","secret":"…","fw":"…"} 한 줄로 답한다. floatee.caresea.kr/start 가 이 값을 읽는다.
//  서버 전송   : 핑을 보낼 때 헤더 "Authorization: Bearer <등록 코드>" 를 붙인다(floatee /api/drift/ping).
//
// ★등록 코드는 비밀번호와 같다. 공개 저장소 · 사진 · 채팅에 올리지 않는다.
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>

namespace floatee {

struct Identity {
  char device_id[16];  // "ff-" + 12 + NUL
  char secret[33];     // 32 + NUL
};

inline void base64url(const uint8_t *in, size_t n, char *out) {
  static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
  size_t o = 0;
  for (size_t i = 0; i + 2 < n; i += 3) {
    uint32_t v = (uint32_t(in[i]) << 16) | (uint32_t(in[i + 1]) << 8) | in[i + 2];
    out[o++] = T[(v >> 18) & 63];
    out[o++] = T[(v >> 12) & 63];
    out[o++] = T[(v >> 6) & 63];
    out[o++] = T[v & 63];
  }
  out[o] = '\0';
}

inline void loadOrCreate(Identity &id) {
  uint64_t mac = ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL;
  snprintf(id.device_id, sizeof(id.device_id), "ff-%04x%08x", (unsigned)(mac >> 32), (unsigned)(mac & 0xFFFFFFFFUL));

  Preferences p;
  p.begin("floatee", false);
  String s = p.getString("secret", "");
  if (s.length() != 32) {
    uint8_t buf[24];
    esp_fill_random(buf, sizeof(buf));  // RF 가 꺼져 있어도 부트로더 엔트로피로 채워진다
    char tmp[33];
    base64url(buf, sizeof(buf), tmp);
    s = String(tmp);
    p.putString("secret", s);
  }
  p.end();
  strncpy(id.secret, s.c_str(), 32);
  id.secret[32] = '\0';
}

inline void printInfo(Stream &out, const Identity &id, const char *fw) {
  out.printf("{\"device_id\":\"%s\",\"secret\":\"%s\",\"fw\":\"%s\"}\n", id.device_id, id.secret, fw);
}

// 시리얼에서 한 줄씩 읽어 "FLOATEE INFO" 면 답한다. loop 에서 자주 부르거나, 부팅 직후 창(ms) 동안 기다린다.
inline bool pollSerial(Stream &in, const Identity &id, const char *fw) {
  static String line;
  bool answered = false;
  while (in.available()) {
    char c = (char)in.read();
    if (c == '\n' || c == '\r') {
      line.trim();
      if (line.equalsIgnoreCase("FLOATEE INFO")) {
        printInfo(in, id, fw);
        answered = true;
      }
      line = "";
    } else if (line.length() < 40) {
      line += c;
    }
  }
  return answered;
}

inline void listenWindow(Stream &in, const Identity &id, const char *fw, uint32_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) {
    pollSerial(in, id, fw);
    delay(10);
  }
}

}  // namespace floatee
