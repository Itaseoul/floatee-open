/*
 * ffid_store.h — 수신 없는 구간의 기록을 칩 플래시(LittleFS)에 쌓는 저장소 (2026-09-27)
 *
 * 왜: 기존 RTC 메모리 버퍼는 16개뿐이고 넘치면 가장 오래된 것부터 버렸다. 전원이 끊기면
 *     통째로 사라졌다. 맹그로브 안쪽처럼 며칠 수신이 없는 자리에서는 「들어간 경로」가 먼저
 *     지워지고 걸린 자리의 같은 좌표만 남는다. 이 파일이 그 두 가지를 고친다.
 *
 * 구조
 *   /ffq_v3.bin  PingRec 을 고정 크기로 이어 붙인 파일(웨이크당 한 줄 추가).
 *   /ffq_v3.hd   보낸 앞부분의 개수(head, 4바이트). 보낸 기록을 지우는 대신 이 값만 옮긴다.
 *   남은 기록 = (파일 레코드 수) − head. head 가 COMPACT_AT 을 넘으면 한 번에 정리한다.
 *   ★v1.3(2026-09-28): PingRec 에 통신 품질·GNSS 고정 시간·태양광 전압 칸을 더해 파일 이름을
 *     v2 → v3 으로 올렸다(72 → 96바이트). 부팅 때 옛 v2 파일이 있으면 v3 으로 옮겨 적고 지운다
 *     (재플래시로 보내지 않은 기록을 잃지 않게).
 *
 * 넘칠 때(상한 g_store_cap) 솎는 규칙 — 들어간 경로와 현재 위치를 둘 다 지킨다
 *   ① 앞 KEEP_HEAD 개(처음 들어간 경로)와 뒤 KEEP_TAIL 개(최근 위치)는 건드리지 않는다.
 *   ② 가운데에서 「마지막 보고(F_LAST)」·「되살아남(F_REVIVED)」은 남긴다.
 *   ③ 가운데의 좌초 의심(F_STRANDED) 기록은 STRAND_KEEP_EVERY 개 중 1개만 남긴다
 *      (같은 자리 좌표의 반복이라 정보가 적다).
 *   ④ 그래도 넘치면 가운데 나머지를 하나 걸러 하나씩 남긴다(간격이 두 배가 된다).
 *   → 가운데가 성겨질 뿐 경로의 처음과 끝은 끝까지 남는다. 서버는 seq 빈틈으로 솎인 수를 안다.
 *
 * 전원 차단: 추가 중에 끊기면 파일 끝에 조각이 남을 수 있다. storeBegin 이 조각을 잘라낸다
 *   (그 한 건만 잃는다). 정리·솎기는 임시 파일에 쓴 뒤 이름을 바꾼다.
 * 플래시 마모: 쓰기는 웨이크당 레코드 한 줄 + 보낸 뒤 head 4바이트. LittleFS 가 마모를 고르게
 *   나눈다. 60분 간격이면 수명에 여유가 크다(벤치에서 확인 전에는 수치를 주장하지 않는다).
 *
 * 이 헤더는 스케치에서 PingRec·pingRecInit() 과 F_* 를 정의한 뒤에 include 한다.
 */
#pragma once
#include <LittleFS.h>

#ifndef STORE_MAX
// ★v1.3: 1,200 → 5,000건. 근거(2026-09-28 확인):
//   - 보드 설정 "ESP32 Dev Module" 기본 분할표(core 3.0.7 tools/partitions/default.csv)의 파일 영역은
//     spiffs 0x290000, 크기 0x160000 = 1,441,792바이트(1.375 MB)다. LittleFS 가 이 영역을 쓴다.
//   - 레코드 96바이트(v3) × 5,000 = 480 KB. 정리·솎기는 임시 파일에 다시 쓰므로 순간 최대
//     2 × (5,000 + COMPACT_AT 256) × 96 ≈ 1.01 MB 가 필요하고, 이는 영역의 약 73% 다.
//   - 전문가 검수(260928 03 문서 9절)의 「약 6,000건」은 72바이트 기준이었다. 96바이트로 6,000건이면
//     순간 최대 약 1.2 MB(83%)라 LittleFS 메타데이터 여유가 얇아 5,000건으로 잡았다.
//   - 분할표가 다른 보드에서는 storeBegin 이 실제 영역 크기로 상한을 다시 줄인다(g_store_cap).
//   60분 간격이면 약 200일, 30분 간격이면 약 100일치다.
#define STORE_MAX          5000
#endif
#define KEEP_HEAD            48   // 처음 들어간 경로(60분 간격이면 이틀치)
#define KEEP_TAIL           240   // 최근 위치(60분 간격이면 열흘치)
#define STRAND_KEEP_EVERY     6   // 가운데 좌초 기록은 6개 중 1개
#define COMPACT_AT          256   // 보낸 앞부분이 이만큼 쌓이면 파일을 정리한다

static const char* STORE_FILE = "/ffq_v3.bin";
static const char* STORE_HEAD = "/ffq_v3.hd";
static const char* STORE_TMP  = "/ffq_v3.tmp";
// 옛 형식(v1.2). 부팅 때 옮겨 적는다.
static const char* STORE_V2_FILE = "/ffq_v2.bin";
static const char* STORE_V2_HEAD = "/ffq_v2.hd";
static const char* STORE_V2_TMP  = "/ffq_v2.tmp";

static bool     g_store_ok  = false;
static uint32_t g_store_cap = STORE_MAX;   // 실제 파일 영역 크기로 줄인 상한(storeBegin 에서 정한다)

// v1.2 레코드 배치(72바이트). 옮겨 적기에만 쓴다. ★이 배치를 바꾸지 않는다.
struct PingRecV2 {
  double   lat, lon;
  float    batt;
  float    hdop;
  uint32_t seq;
  char     ts_fix[24];
  uint8_t  flags;
  uint16_t interval_m;
  float    wake_mah_prev;
  float    wake_s_prev;
  float    i_peak_ma_prev;
};

static uint32_t storeFileRecs() {
  File f = LittleFS.open(STORE_FILE, "r");
  if (!f) return 0;
  uint32_t n = f.size() / sizeof(PingRec);
  f.close();
  return n;
}

static uint32_t storeHead() {
  File f = LittleFS.open(STORE_HEAD, "r");
  if (!f) return 0;
  uint32_t h = 0;
  if (f.read((uint8_t*)&h, sizeof(h)) != sizeof(h)) h = 0;
  f.close();
  uint32_t n = storeFileRecs();
  return h > n ? n : h;
}

static void storeSetHead(uint32_t h) {
  File f = LittleFS.open(STORE_HEAD, "w");
  if (!f) return;
  f.write((const uint8_t*)&h, sizeof(h));
  f.close();
}

// 남은(아직 보내지 않은) 기록 수
uint32_t storeCount() {
  if (!g_store_ok) return 0;
  return storeFileRecs() - storeHead();
}

// 남은 기록 중 i 번째(0 = 가장 오래된 것)를 읽는다
bool storeRead(uint32_t i, PingRec &r) {
  if (!g_store_ok) return false;
  File f = LittleFS.open(STORE_FILE, "r");
  if (!f) return false;
  uint32_t pos = (storeHead() + i) * sizeof(PingRec);
  bool ok = f.seek(pos) && f.read((uint8_t*)&r, sizeof(r)) == sizeof(r);
  f.close();
  return ok;
}

// 원본을 한 줄씩 읽어 keep(i, rec) 이 참인 것만 임시 파일에 옮긴 뒤 바꿔 끼운다.
// i 는 남은 기록 기준 번호(head 제외). 끝나면 head = 0.
template <typename KeepFn>
static bool storeRewrite(KeepFn keep) {
  File in = LittleFS.open(STORE_FILE, "r");
  if (!in) return false;
  File out = LittleFS.open(STORE_TMP, "w");
  if (!out) { in.close(); return false; }
  uint32_t head = storeHead();
  uint32_t n = in.size() / sizeof(PingRec);
  PingRec r;
  in.seek(head * sizeof(PingRec));
  for (uint32_t k = head; k < n; k++) {
    if (in.read((uint8_t*)&r, sizeof(r)) != sizeof(r)) break;
    if (keep(k - head, n - head, r)) out.write((const uint8_t*)&r, sizeof(r));
  }
  in.close(); out.close();
  // ★순서가 중요하다. head 를 먼저 0 으로 두면, 여기서 전원이 끊겨도 원본이 남아 있고
  //   보낸 것을 다시 보낼 뿐이다(서버가 seq 로 중복을 거른다). 원본 삭제와 이름 바꾸기
  //   사이에 끊기면 storeBegin 이 임시 파일을 원본으로 되살린다.
  storeSetHead(0);
  LittleFS.remove(STORE_FILE);
  return LittleFS.rename(STORE_TMP, STORE_FILE);
}

// 보낸 앞부분을 파일에서 실제로 걷어낸다
static void storeCompact() {
  storeRewrite([](uint32_t, uint32_t, const PingRec&) { return true; });
}

// 넘칠 때 가운데를 솎는다(파일 머리의 규칙 ①~④)
static void storeThin() {
  uint32_t before = storeCount();
  // 1차: 가운데 좌초 기록을 STRAND_KEEP_EVERY 개 중 1개로
  uint32_t strandCtr = 0;
  storeRewrite([&](uint32_t i, uint32_t n, const PingRec &r) {
    if (i < KEEP_HEAD || i + KEEP_TAIL >= n) return true;
    if (r.flags & (F_LAST | F_REVIVED)) return true;
    if (r.flags & F_STRANDED) return (strandCtr++ % STRAND_KEEP_EVERY) == 0;
    return true;
  });
  // 2차: 아직 넘치면 가운데 나머지를 하나 걸러 하나씩
  if (storeCount() >= g_store_cap) {
    uint32_t midCtr = 0;
    storeRewrite([&](uint32_t i, uint32_t n, const PingRec &r) {
      if (i < KEEP_HEAD || i + KEEP_TAIL >= n) return true;
      if (r.flags & (F_LAST | F_REVIVED)) return true;
      return (midCtr++ % 2) == 0;
    });
  }
  Serial.printf("[FF] 저장소 솎기 %lu → %lu건\n", (unsigned long)before, (unsigned long)storeCount());
}

// 옛 v2 파일의 보내지 않은 기록을 v3 으로 옮겨 적는다. v1.3 에서 생긴 칸은 「없음」 값(pingRecInit).
static void storeMigrateV2() {
  if (!LittleFS.exists(STORE_V2_FILE)) {
    if (LittleFS.exists(STORE_V2_HEAD)) LittleFS.remove(STORE_V2_HEAD);
    if (LittleFS.exists(STORE_V2_TMP))  LittleFS.remove(STORE_V2_TMP);
    return;
  }
  uint32_t head = 0;
  File h = LittleFS.open(STORE_V2_HEAD, "r");
  if (h) { if (h.read((uint8_t*)&head, sizeof(head)) != sizeof(head)) head = 0; h.close(); }
  uint32_t moved = 0;
  File in = LittleFS.open(STORE_V2_FILE, "r");
  if (in) {
    uint32_t n = in.size() / sizeof(PingRecV2);
    File out = LittleFS.open(STORE_FILE, "a");
    if (out && head < n) {
      in.seek(head * sizeof(PingRecV2));
      PingRecV2 o;
      for (uint32_t k = head; k < n; k++) {
        if (in.read((uint8_t*)&o, sizeof(o)) != sizeof(o)) break;
        PingRec r; pingRecInit(r);
        r.lat = o.lat; r.lon = o.lon; r.batt = o.batt; r.hdop = o.hdop; r.seq = o.seq;
        memcpy(r.ts_fix, o.ts_fix, sizeof(r.ts_fix)); r.ts_fix[sizeof(r.ts_fix) - 1] = 0;
        r.flags = o.flags; r.interval_m = o.interval_m;
        r.wake_mah_prev = o.wake_mah_prev; r.wake_s_prev = o.wake_s_prev; r.i_peak_ma_prev = o.i_peak_ma_prev;
        out.write((const uint8_t*)&r, sizeof(r));
        moved++;
      }
    }
    if (out) out.close();
    in.close();
  }
  // 옮겨 적은 뒤에 지운다. 옮기는 도중 끊기면 다음 부팅에 v2 가 남아 한 번 더 옮긴다
  // (앞서 옮긴 것과 겹칠 수 있으나 서버가 (device_id, seq) 로 중복을 거른다).
  LittleFS.remove(STORE_V2_FILE);
  if (LittleFS.exists(STORE_V2_HEAD)) LittleFS.remove(STORE_V2_HEAD);
  if (LittleFS.exists(STORE_V2_TMP))  LittleFS.remove(STORE_V2_TMP);
  Serial.printf("[FF] 저장소 v2 → v3 옮겨 적기 %lu건\n", (unsigned long)moved);
}

// 부팅 때 한 번. 파일 끝 조각(추가 중 전원 차단)을 잘라내고, 상한을 영역 크기에 맞추고, 옛 파일을 옮긴다.
bool storeBegin() {
  g_store_ok = LittleFS.begin(true);   // 첫 부팅이면 포맷
  if (!g_store_ok) { Serial.println("[FF] LittleFS 마운트 실패 — 이번 웨이크는 저장 없이 진행"); return false; }
  // 상한: 정리 때 원본과 임시 파일이 함께 있으므로 (영역 − 여유 64 KB) ÷ (2 × 레코드) − COMPACT_AT.
  {
    size_t total = LittleFS.totalBytes();
    uint32_t fit = total > 65536 ? (uint32_t)((total - 65536) / (2 * sizeof(PingRec))) : 0;
    fit = fit > COMPACT_AT ? fit - COMPACT_AT : 0;
    g_store_cap = fit < (uint32_t)STORE_MAX ? fit : (uint32_t)STORE_MAX;
    if (g_store_cap < KEEP_HEAD + KEEP_TAIL + 16) g_store_cap = KEEP_HEAD + KEEP_TAIL + 16;
    Serial.printf("[FF] 저장소 영역 %u바이트, 레코드 %u바이트, 상한 %lu건\n",
                  (unsigned)total, (unsigned)sizeof(PingRec), (unsigned long)g_store_cap);
  }
  // 정리 도중 전원이 끊긴 흔적 복구
  bool hasMain = LittleFS.exists(STORE_FILE), hasTmp = LittleFS.exists(STORE_TMP);
  if (!hasMain && hasTmp) {            // 원본 삭제 뒤, 이름 바꾸기 전에 끊김 → 임시 파일이 완성본
    LittleFS.rename(STORE_TMP, STORE_FILE);
    storeSetHead(0);
    Serial.println("[FF] 저장소 복구: 임시 파일을 원본으로");
  } else if (hasTmp) {                 // 임시 파일을 쓰던 중에 끊김 → 원본이 온전하니 임시 파일을 버린다
    LittleFS.remove(STORE_TMP);
  }
  File f = LittleFS.open(STORE_FILE, "r");
  if (f) {
    size_t sz = f.size();
    f.close();
    if (sz % sizeof(PingRec) != 0) {
      Serial.printf("[FF] 저장소 끝 조각 %u바이트 정리\n", (unsigned)(sz % sizeof(PingRec)));
      storeCompact();   // 온전한 레코드만 옮겨 쓰면서 조각이 빠진다
    }
  }
  storeMigrateV2();
  return true;
}

// 기록 한 건 추가. 가득 찼으면 먼저 솎는다.
void storePush(const PingRec &r) {
  if (!g_store_ok) return;
  if (storeHead() >= COMPACT_AT) storeCompact();
  if (storeCount() >= g_store_cap) storeThin();
  File f = LittleFS.open(STORE_FILE, "a");
  if (!f) { Serial.println("[FF] 저장소 쓰기 실패"); return; }
  f.write((const uint8_t*)&r, sizeof(r));
  f.close();
}

// 앞에서부터 k 건을 보냈다고 표시한다(파일은 그대로, head 만 옮긴다)
void storeDropFront(uint32_t k) {
  if (!g_store_ok || k == 0) return;
  uint32_t h = storeHead() + k;
  uint32_t n = storeFileRecs();
  storeSetHead(h > n ? n : h);
}
