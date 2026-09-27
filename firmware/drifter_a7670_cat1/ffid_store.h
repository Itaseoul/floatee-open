/*
 * ffid_store.h — 수신 없는 구간의 기록을 칩 플래시(LittleFS)에 쌓는 저장소 (2026-09-27)
 *
 * 왜: 기존 RTC 메모리 버퍼는 16개뿐이고 넘치면 가장 오래된 것부터 버렸다. 전원이 끊기면
 *     통째로 사라졌다. 맹그로브 안쪽처럼 며칠 수신이 없는 자리에서는 「들어간 경로」가 먼저
 *     지워지고 걸린 자리의 같은 좌표만 남는다. 이 파일이 그 두 가지를 고친다.
 *
 * 구조
 *   /ffq_v2.bin  PingRec 을 고정 크기로 이어 붙인 파일(웨이크당 한 줄 추가).
 *   /ffq_v2.hd   보낸 앞부분의 개수(head, 4바이트). 보낸 기록을 지우는 대신 이 값만 옮긴다.
 *   남은 기록 = (파일 레코드 수) − head. head 가 COMPACT_AT 을 넘으면 한 번에 정리한다.
 *
 * 넘칠 때(STORE_MAX) 솎는 규칙 — 들어간 경로와 현재 위치를 둘 다 지킨다
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
 * 이 헤더는 스케치에서 PingRec 과 F_* 를 정의한 뒤에 include 한다.
 */
#pragma once
#include <LittleFS.h>

#ifndef STORE_MAX
#define STORE_MAX          1200   // 약 1,200건 × 68바이트 ≈ 82KB. 60분 간격이면 약 50일
#endif
#define KEEP_HEAD            48   // 처음 들어간 경로(60분 간격이면 이틀치)
#define KEEP_TAIL           240   // 최근 위치(60분 간격이면 열흘치)
#define STRAND_KEEP_EVERY     6   // 가운데 좌초 기록은 6개 중 1개
#define COMPACT_AT          256   // 보낸 앞부분이 이만큼 쌓이면 파일을 정리한다

static const char* STORE_FILE = "/ffq_v2.bin";
static const char* STORE_HEAD = "/ffq_v2.hd";
static const char* STORE_TMP  = "/ffq_v2.tmp";

static bool g_store_ok = false;

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
  LittleFS.remove(STORE_FILE);
  bool ok = LittleFS.rename(STORE_TMP, STORE_FILE);
  storeSetHead(0);
  return ok;
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
  if (storeCount() >= STORE_MAX) {
    uint32_t midCtr = 0;
    storeRewrite([&](uint32_t i, uint32_t n, const PingRec &r) {
      if (i < KEEP_HEAD || i + KEEP_TAIL >= n) return true;
      if (r.flags & (F_LAST | F_REVIVED)) return true;
      return (midCtr++ % 2) == 0;
    });
  }
  Serial.printf("[FF] 저장소 솎기 %lu → %lu건\n", (unsigned long)before, (unsigned long)storeCount());
}

// 부팅 때 한 번. 파일 끝 조각(추가 중 전원 차단)을 잘라낸다.
bool storeBegin() {
  g_store_ok = LittleFS.begin(true);   // 첫 부팅이면 포맷
  if (!g_store_ok) { Serial.println("[FF] LittleFS 마운트 실패 — 이번 웨이크는 저장 없이 진행"); return false; }
  File f = LittleFS.open(STORE_FILE, "r");
  if (f) {
    size_t sz = f.size();
    f.close();
    if (sz % sizeof(PingRec) != 0) {
      Serial.printf("[FF] 저장소 끝 조각 %u바이트 정리\n", (unsigned)(sz % sizeof(PingRec)));
      storeCompact();   // 온전한 레코드만 옮겨 쓰면서 조각이 빠진다
    }
  }
  return true;
}

// 기록 한 건 추가. 가득 찼으면 먼저 솎는다.
void storePush(const PingRec &r) {
  if (!g_store_ok) return;
  if (storeHead() >= COMPACT_AT) storeCompact();
  if (storeCount() >= STORE_MAX) storeThin();
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
