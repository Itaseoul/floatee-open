# floatee_pairing — 기기 등록 스케치

드리프터를 floatee.caresea.kr 계정에 등록할 때 쓰는 스케치입니다. 통신 모듈과 GPS 는 켜지 않습니다.

## 하는 일

| 항목 | 내용 |
|---|---|
| 기기 ID | `ff-` + 칩 고유번호(eFuse MAC) 12자리. 펌웨어를 다시 올려도 같습니다 |
| 등록 코드 | 첫 부팅 때 하드웨어 난수로 만든 32자. NVS `floatee` 네임스페이스에 저장됩니다 |
| 시리얼 질의 | 115200 bps 로 `FLOATEE INFO` 를 받으면 `{"device_id","secret","fw"}` 한 줄로 답합니다 |
| 등록 주소 | 부팅 때 `https://floatee.caresea.kr/start#d=<기기 ID>&k=<등록 코드>` 를 찍습니다 |

★등록 코드는 비밀번호와 같습니다. 사진이나 채팅에 올리지 않습니다. 플래시 전체 지우기(`erase_flash`)를 하면 새 코드가 만들어지므로, 그때는 웹에서 등록을 해제한 뒤 다시 등록합니다.

## 쓰는 법

1. 이 스케치를 올립니다(보드 `ESP32 Dev Module`).
2. 시리얼 모니터를 닫습니다. 웹 화면이 같은 포트를 써야 합니다.
3. floatee.caresea.kr/start 에서 「USB 로 기기 연결」을 누릅니다.
4. 등록이 끝나면 본 펌웨어(`drifter_a7670_cat1`)를 올립니다.

## 본 펌웨어에 넣을 것 (아직 반영 전)

`floatee_identity.h` 를 본 펌웨어 폴더에 복사하고 세 곳을 고칩니다.

```cpp
#include "floatee_identity.h"
floatee::Identity ident;

// setup() 맨 앞: 신원을 읽고, 부팅 직후 3초 동안 설치 화면 질의에 답한다
floatee::loadOrCreate(ident);
floatee::listenWindow(Serial, ident, FW_VERSION, 3000);

// buildPingBody(): device_id 를 상수 DEVICE_ID 대신 ident.device_id 로
// flushBuffer(): 헤더를 하나 더 보낸다
http.sendHeader("Authorization", String("Bearer ") + ident.secret);
```

서버 주소는 `floatee.caresea.kr` 의 `/api/drift/ping` 입니다. 비밀값이 틀리거나 등록 전이면 서버가 401 을 돌려주고, 펌웨어는 기록을 버퍼에 두었다가 등록 뒤 다음 전송 때 보냅니다.
