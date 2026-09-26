# Firmware

ESP32 + A7670 (LTE Cat.1 bis) + L76K GNSS. **FF-ID schema v1.1** ([data/schema.md](../data/schema.md)).

Wake on a timer → **GPS fix first** (so an observation is kept even with no coverage) → append to an RTC store-and-forward buffer with a monotonic `seq` → modem on → network → POST buffered records oldest-first → deep sleep. The device sends points; the server assembles the track and never trusts arrival order (it orders by `ts_fix`/`seq`).

Reference firmware: [`drifter_a7670_cat1/drifter_a7670_cat1.ino`](drifter_a7670_cat1/drifter_a7670_cat1.ino) (MIT).

## v1.1 payload (what each ping carries)

```json
{"device_id":"ff-kr-bs-u0001","site_id":"nakdong-hakjang","seq":1487,
 "lat":35.105600,"lon":128.943100,"batt":4.02,
 "sample_interval_s":1800,"fix_quality":1.4,"gnss_source":"l76k",
 "ts_fix":"2026-07-10T09:00:00Z","ts":"2026-07-10T09:00:00Z"}
```

- `seq` — monotonic per-device counter. Survives deep sleep (RTC memory) **and** power loss (NVS). The server dedups and detects gaps by `seq`.
- `ts_fix` — the GPS UTC time of the fix (the observation time). Legacy `ts` is sent with the same value for older ingest servers (schema 1.0→1.1 mapping).
- `sample_interval_s` — the sleep time chosen after this record (voltage-adaptive since 2026-09-26; required by the schema for mixed-cadence weighting).
- `flags` — recovery bits (2026-09-26): `0x01` recover-request · `0x02` out of recovery zone · `0x04` stranded (within 50 m for 6 h) · `0x08` last report before shutdown (first time below 3.55 V) · `0x10` revived (was low, recovered above 3.75 V). `0` = normal.
- `fix_quality` — HDOP from the L76K (omitted when invalid); `gnss_source:"l76k"`.
- **Store-and-forward**: up to 16 records buffer in RTC memory across sleep cycles; a cellular dead zone does not lose observations. Power loss drops the buffer (documented tradeoff) but never reuses a `seq`.

## Test paths and recovery mode (2026-09-26)

- **Wi-Fi test mode** (`USE_WIFI 1`, added 2026-09-12): the ESP32 sends over Wi-Fi (phone hotspot or office AP, 2.4 GHz only) so you can prove GPS and server round-trip before a SIM arrives. Wi-Fi drops within tens of metres of the bank — **set `USE_WIFI 0` (LTE) before release.**
- **Voltage-adaptive interval** (`ADAPTIVE_INTERVAL`): ≥3.90 V 60 min · ≥3.70 V 120 min · ≥3.50 V 360 min · below 720 min. Out of the recovery zone with ≥3.90 V: 30 min.
- **Recovery zone**: set `ZONE_LAT`, `ZONE_LON`, `ZONE_RADIUS_M` per release site (radius 0 disables).
- **Low-battery memory in NVS**: the protection circuit may cut power and wipe RTC memory, so the "was low" mark survives in NVS to produce the `revived` flag.
- All thresholds are initial values. **Compiles as of 2026-09-26 in all three modes (Wi-Fi, LTE bench HTTP, LTE HTTPS); not yet bench-tested on a board.**

## Toolchain

- Arduino IDE, ESP32 Arduino core 3.0.x, board "ESP32 Dev Module"
- Command line (verified 2026-09-26, core 3.0.7, arduino-cli 1.3.1): `arduino-cli compile -b esp32:esp32:esp32 firmware/drifter_a7670_cat1`. Flash use: Wi-Fi mode 949,785 bytes (72%), LTE bench 335,389 (25%), LTE HTTPS 338,797 (25%).
- Libraries: **lewisxhe/TinyGSM-fork** (required; stock TinyGSM lacks the A7670 macro — uninstall stock), TinyGPSPlus, ArduinoHttpClient. `Preferences` ships with the core.

## Configure

- `DEVICE_ID` — pair it with the unit metadata directory `unit_id` (schema §3).
- `APN` for your SIM (Soracom `soracom.io` user/pass `sora`; 1NCE `iot.1nce.net`; KT MVNO `lte.ktfwing.com`; SKT MVNO `lte.sktelecom.com`). Global IoT SIMs roam on KT/SKT only — no LG U+.
- `SERVER_HOST` / `SERVER_PATH` — bench with the bundled `ingest_server.py` (`BENCH_HTTP 1`), then promote to HTTPS (`BENCH_HTTP 0`).
- `SLEEP_MINUTES` — see [hardware/ELECTRONICS_POWER.md](../hardware/ELECTRONICS_POWER.md) for the honest endurance table (all figures estimates pending bench measurement).

## Gotchas

- GPS is the separate L76K chip on its own UART (NMEA via TinyGPSPlus), not the modem's AT GNSS.
- Test with the 18650 installed. USB-only power browns out on the ~2 A transmit peak and looks like a modem fault.
- The v1.1 flow order (GPS → modem) differs from the original bench-validated v1.0 order (modem → GPS). Primitives are unchanged, but **re-run the bench sequence before deploying a fleet** (honesty: no improvement claims before measurement).
- Start with plain HTTP, promote to HTTPS after the pipeline works end to end.

## Security & supply chain

Two hard requirements before any **real** deployment. Both are honest gaps in the reference build today — documented here rather than silently assumed fixed.

**1. TLS is encrypted but not yet authenticated.** In production mode (`BENCH_HTTP 0`) the firmware POSTs over HTTPS/443 with `TinyGsmClientSecure`. But on the A76xx the secure client **does not verify the server certificate unless you configure a CA certificate and enable server authentication** — so out of the box it is open to a man-in-the-middle (a hostile cell/roaming network could impersonate the server and read/alter observations). To close it:

- Obtain the server's CA/root certificate (PEM) — e.g. the Let's Encrypt / ISRG Root for `openc.caresea.kr`.
- Before the first `client.connect(...)`, set it and require verification, then **verify hostname** matches: `client.setCACert(PROD_CA_PEM);` plus the fork's SSL auth-mode / SNI configuration (`AT+CSSLCFG` "authmode" = verify server, "sni"/"servername" = the host). The reference `.ino` marks the exact call site with a `TLS-AUTH` comment and a commented hook.
- **Verify on hardware.** Per this repo's ethic (no improvement claims before measurement), do not treat TLS auth as working until you have watched a build *reject* a wrong/self-signed cert on the bench. A cert that never expires-checks is not authentication.

**2. Pin the modem library to a commit.** The build requires **lewisxhe/TinyGSM-fork** (stock `vshymanskyy/TinyGSM` lacks the `A7670` macro). "Use the fork" is a supply-chain risk if it means "whatever HEAD is today." For a fleet build, **install a pinned commit**, not a moving branch:

```
git clone https://github.com/lewisxhe/TinyGSM ~/Arduino/libraries/TinyGSM-fork
cd ~/Arduino/libraries/TinyGSM-fork && git checkout <COMMIT_SHA_YOU_VERIFIED>
```

Record the SHA you bench-verified next to your build (Arduino Library Manager cannot pin, so a production build must use a manually checked-out commit). Re-verify after any bump. Same discipline for TinyGPSPlus and ArduinoHttpClient versions.

Bench mode (`BENCH_HTTP 1`) is **plaintext HTTP over an ngrok tcp tunnel** — correct for a bench loop, never for real observations. Do not collect real data in bench mode.

## Bench re-verification checklist (v1.1, after the adversarial-review fixes)

1. **GPS power gate**: with `BENCH_HTTP 1`, confirm NMEA bytes arrive **before** the modem PWRKEY runs (serial shows a fix, or on failure `NMEA chars=` > 0).
   - ★**Schematic-confirmed** (docs/구멍_해결_2026-07-11.md, hole A2): the L76K VCC sits on the **V3V rail gated by GPIO12 (BOARD_POWERON) OR USB VBUS** — it does **not** depend on modem power-on. `getFix()` already raises `BOARD_POWERON` HIGH first, so GPS-first is expected to work. The old "move `modemPowerOn()` earlier" fallback is therefore **not** needed if `BOARD_POWERON` is HIGH.
   - ★**Verify on battery only** (USB disconnected): with USB, VBUS keeps V3V alive regardless of GPIO12, masking the gate. If `chars=0` on battery, probe V3V (TF-card VDD pad / PX header pin 16): it should read 0 V at GPIO12 LOW and 3.3 V at HIGH.
2. **Full cycle**: fix → buffer → POST 200 → deep sleep → wake, `seq` +1 each wake. Then kill coverage (unscrew LTE antenna) for 2–3 cycles and confirm the buffer flushes in one batch on recovery.
3. **HTTPS build**: `BENCH_HTTP 0` compiles under `TINY_GSM_MODEM_A76XXSSL` and POSTs over 443 (the modem class changes — re-verify attach; check the SSL client's CA-validation default).
4. **Sleep current**: measure before/after the GPIO-hold fix and update [hardware/ELECTRONICS_POWER.md](../hardware/ELECTRONICS_POWER.md) estimates with measured values.

> ★**First attach is the hardest step — instrument it or lose days.** A cold unit (no cached network) takes a while to register, and a silent failure looks identical whether the cause is a loose antenna, the wrong band, a roaming rejection, or a SIM that was never activated. Before you trust attach: enable `#define TINY_GSM_DEBUG Serial` (AT echo), and log `getSignalQuality()` (CSQ), `getRegistrationStatus()`, `getOperator()`, `getModemInfo()` around the attach; give the **first** attach **120–180 s** (a 60 s `waitForNetwork` is too short for a cold roam); and on failure retry 1–2× within the same wake before deep-sleeping. Global IoT SIMs attach on **KT/SKT only** — confirm the operator you actually landed on, not the one you expected.
