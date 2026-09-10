# 2026-09-10 — TC001 이라고 다 8MB 는 아니다: 4MB 변형 보드 추가

새 Ulanzi TC001 을 USB 로 붙이고 릴리스 이미지를 굽으려다 멈췄다. `agentdeck esp32 flash`
는 `connect…` 에서 실패했지만(esptool-js `ClassicReset` 이 이 CH340 과 sync 안 됨 — esptool.py
는 첫 시도에 붙는다. `ips_10` 과 같은 계열의 구멍), 진짜 문제는 그 다음에 나왔다.

**flash id 가 4MB 였다.** JEDEC mfr `0x46` / device `0x4016`. SSOT 의 `ulanzi_tc001` 은 8MB 로
선언돼 있고, `partitions/ulanzi_tc001.csv` 는 `app1` 을 0x310000, `coredump` 를 0x7F0000 에
둔다 — 둘 다 이 파트의 끝 너머다. `esp32PreflightVerdict` 가 거부하는(declared 8 > detected 4)
바로 그 조합이고, override 는 없다.

### "flash id 를 잘못 읽은 것 아닌가" 를 배제한 근거 셋

mfr `0x46` 은 우리가 특성화해 둔 JEDEC 벤더가 아니라서 id 하나만으로는 의심스러웠다. 세 신호가
독립적으로 일치했다:

1. capacity byte `0x16` → 4MB
2. 칩에 이미 올라가 있던 **공장 부트로더 헤더** `e9 04 02 20` — `0x20` = 4MB/40MHz
3. 0x8000 의 **공장 파티션 테이블이 정확히 `0x400000` 에서 끝난다** (app0/app1 각 0x1E0000,
   spiffs 가 4MB 경계를 채움)

3번이 결정적이다. 우리 devlog 는 8MB TC001 에 4MB 헤더를 쓰면 부트로더가 오동작한다고
(GPIO15 부저 HIGH 고정 + 화면 미출력) 여러 번 기록해 뒀다. 그러니 4MB 를 선언하고도 정상
동작하는 공장 펌웨어는 4MB 파트에서만 가능하다.

### 전기적으로는 같은 보드

공장 app 을 덤프해 보니 원본 Ulanzi AWTRIX 2(`Ulanzi Pixel Clock`)였고, FastLED 설정이
`DATA_PIN = 32`, `RGB_ORDER = 66`(octal `0102` = GRB), 32×8 — `boards/board_ulanzi_tc001.h` 와
정확히 일치한다. 펌웨어는 그대로 맞고, 다른 건 flash 뿐이다.

### 그래서 wire id 를 나눴다

`ulanzi_tc001_4mb` / env `led8x32_4mb`. 같은 PCB 인데 굳이 id 를 나눈 이유는 **OTA** 다:
`agentdeck esp32-ota <target>` 은 보드가 보고한 id 로 대상을 고르므로, id 를 공유하면 8MB
빌드가 4MB 유닛의 합법적인 OTA 페이로드가 된다 — USB 대신 WiFi 로 도착하는 같은 벽돌.
4MB env 는 렌더 분기를 다 쓰려고 `BOARD_LED8X32` 도 함께 정의하므로 `agentdeckBoardName()`
의 사다리에서 **4MB 를 먼저** 검사한다.

파티션은 공장 layout 을 그대로 베꼈다(줄여 맞춘 게 아니라 이 파트를 위해 설계된 표라는 게
0x400000 에서 정확히 끝난다는 사실 자체다). dual OTA 슬롯 유지 — 0x1E0000 에 1.49MB 펌웨어가
76% 로 들어간다. `flashFreq` 는 형제의 `80m` 이 아니라 `40m`: 특성화되지 않은 벤더의 파트이고
자기 공장 부트로더가 그 속도로 돌린다.

### 곁다리로 고친 것

`serial_client.cpp` 가 wire id 용 `#ifdef` 사다리를 따로 갖고 있었다. `board_config.h` 의
`agentdeckBoardName()` 주석이 "두 번째 사다리가 바로 둘이 어긋나는 경로"라고 못박아 둔 그
사다리다. 둘은 이미 같은 테스트가 아니었다 — 복사본은 `BOARD_AMOLED` 대신 `IS_ROUND` 로
분기했고 `BOARD_JC8012P4A1C` 철자를 받지 않았다. 지금 그 철자를 쓰는 env 가 없어서 아직
오보한 적은 없지만, 생겼을 때 둘 중 하나만 고쳐질 자리였다. 한 줄 호출로 합쳤다.

### 플래시 후: 예상이 틀린 곳

esptool.py 로 erase + merged 이미지를 0x0 에 썼다(`Hash of data verified`). TC001 은 serial 로
확인이 안 되는 보드라 **NVS 로 부팅을 증명했다** — 쓰기 전에 flash 전체를 지웠으므로 0x9000 이
`0xFF` 여야 하는데, 읽어 보니 ESP-IDF NVS 페이지 헤더(`feff ffff 0000 0000`, ACTIVE)와 기록된
엔트리 비트맵이 있었다. 펌웨어가 실제로 돌지 않으면 나올 수 없는 바이트다.

그리고 **예상이 틀렸다**: 데몬이 이 보드를 serial 로 잡았다
(`ulanzi_tc001_4mb v1.2.2 OTA 1.9MB @ /dev/cu.usbserial-210`). 직접 찔러도 3/3 응답.
즉 **이 유닛의 CH340 TX 는 멀쩡하다.** "같은 PCB 니까 broken TX 도 물려받는다"는 추론으로
`SERIAL_PROBE_UNAVAILABLE` 에 넣어 뒀던 것을 되돌렸다 — 이 목록은 실패한 검증을 통과로
바꾸는 목록이라, 답할 수 있는 보드는 답하게 만들어야 한다. broken TX 는 8MB 유닛의 성질이지
설계의 성질이 아니었다.

따라서 판정은 비대칭이다: `ulanzi_tc001_4mb` 라고 답하면 이 변형이 확실하지만, **무응답은
8MB 라는 증거가 아니라 "확인 불가"** 다. 플래시 전 flash id 읽기는 그대로 유효하다.

매트릭스 렌더까지 육안 확인 — 이 보드의 문서화된 검증 기준(matrix render + WiFi 등록) 중
첫 번째가 충족됐다. 즉 첫 플래시는 세 갈래로 검증됐다: 전체 erase 후 NVS 초기화, serial
`device_info` 응답, 그리고 매트릭스 렌더.

`webFlashStatus` 는 `unverified` 가 아니라 **`blocked`** 로 뒀다. "측정 안 했다"가 아니라
"측정했고 여기서는 안 된다" 이기 때문이다 — esptool-js 는 2/2 로 SYNC 에 못 가고 esptool.py
는 같은 케이블/포트에서 첫 시도에 붙는다. `ips_10` 과 같은 계열이라 `webFlashVerified` 에
그 측정을 적어 뒀다(차단의 근거도 근거다).

### WiFi 프로비저닝

데몬의 auto-provision 이 그대로 먹었다 — `wifi-config.json` 의 저장된 자격증명으로 serial
`wifi_provision` 이 나가고 `[WiFi] Connected via provision: 192.168.2.68`. 이후
`ESP32 WiFi registered: ulanzi_tc001_4mb:192.168.2.68`. MAC `d4:e9:f4:d0:90:10`,
Speedport 가 `esp32-d09010.speedport.ip` 로 잡는다. 이로써 이 보드의 문서화된 검증 기준
두 갈래(matrix render + WiFi 등록)가 모두 충족됐다.

디버깅 중 헛짚은 것 하나: 등록이 안 보이길래 수동으로 `wifi_provision` 을 다시 쏘았는데,
로그 타임라인을 되짚어 보면 **필요 없는 일이었다**. 데몬이 16:41:19 에 이미 보냈고, 16:43
과 16:47 재시작이 provision 없이 "token armed" 만 찍은 것이 그 증거다(그 분기는
`!wifiConnected` 일 때만 탄다). 등록이 안 보였던 이유는 내가 serial 포트를 비우려고 데몬을
계속 멈췄기 때문이다 — 보드가 붙을 대상이 없었다.

한 가지 알아 둘 것: TC001 의 radio parking 규칙은 `serialConnected && !wifiConnected` 라,
**아직 한 번도 프로비저닝되지 않은** 유닛은 데몬이 포트를 연 뒤 4초면 라디오를 park 한다.
WiFi 가 붙은 뒤로는 park 하지 않으므로(주석대로 "WiFi 가 이미 죽은 뒤에만 park") 첫 프로
비저닝만 그 창을 신경 쓰면 된다.

### 아직 아닌 것

`webFlash: false` — 이 변형에서 **완료된 쓰기**를 아직 측정하지 않았다. `webFlashVerified`
는 handshake 가 아니라 쓰기를 기록하는 필드다. CLI 경로는 영향 없다(`webFlash` 는 "브라우저에
노출" 이지 "써도 된다" 가 아니다). 릴리스에 이 보드의 이미지가 아직 없으므로 첫 플래시는
로컬 빌드 + `--firmware`, esptool-js sync 문제 때문에 esptool.py 가 필요할 수 있다.
