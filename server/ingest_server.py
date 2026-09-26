"""
SEA:CUT — ita.city 수신 파이프라인 (3단계, stdlib 전용)
=======================================================
맨 처음 다이어그램의 구현:

  드리프터(셀룰러 LTE Cat.1, A7670) ──POST /api/ping──> 이 서버 ──> 지도 궤적 레이어
                                            └──> 예측(OpenDrift) 궤적과 오버레이

엔드포인트
---------
  POST /api/ping             드리프터가 좌표 1점 전송 — FF-ID 스키마 v1.1(data/schema.md)
                             필수: device_id, lat, lon
                             v1.1: seq, ts_fix, sample_interval_s, fix_quality, gnss_source,
                                   batt, site_id, motion_state, flags (v1.0 "ts"는 ts_fix로 매핑)
                             ★(device_id, seq) 중복 재전송은 멱등 처리(재적재 안 함 — 펌웨어
                               store-and-forward 재시도 대비)
  GET  /api/live.geojson     실측 드리프터 궤적(단말별 LineString, ts_fix→seq 순 정렬)
  GET  /api/predicted.geojson 예측 결과: 최종위치 점군 + 앙상블 평균 경로
  GET  /api/recovery.json    회수 목록: 마지막 레코드의 flags 가 0 이 아닌 단말
  GET  /                     Leaflet 지도(실측 + 예측 오버레이, 자동 새로고침)

데이터
------
  data/pings.ndjson          실측 ping 누적(한 줄 = 한 관측)
  nakdong_track.csv          예측 궤적(OpenDrift 또는 lite 데모 산출)

실행:  python ingest_server.py                    # http://127.0.0.1:8770 (로컬 개발)
       python ingest_server.py 8770 0.0.0.0       # ★실보드 벤치(LAN 노출) — 펌웨어
                                                  #   SERVER_HOST = 이 PC의 LAN IP
의존성 없음(표준 라이브러리만) — 실제 배포 시 Next.js /api 라우트로 이식.
"""
import csv
import json
import os
import sys
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

BASE = os.path.dirname(os.path.abspath(__file__))
PINGS = os.path.join(BASE, "data", "pings.ndjson")
GAME_DRIFTS = os.path.join(BASE, "data", "game_drifts.ndjson")  # 게임 시뮬 궤적(실드리퍼와 분리)
PRED_CSV = os.path.join(BASE, "nakdong_track.csv")
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else int(os.environ.get("SEACUT_PORT", 8770))
BIND = sys.argv[2] if len(sys.argv) > 2 else os.environ.get("SEACUT_BIND", "127.0.0.1")
os.makedirs(os.path.dirname(PINGS), exist_ok=True)


def _read_pings():
    if not os.path.exists(PINGS):
        return []
    out = []
    with open(PINGS, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    out.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return out


def live_geojson():
    """단말별로 시간순 LineString + 마지막 위치 Point."""
    by_dev = {}
    for p in _read_pings():
        by_dev.setdefault(p["device_id"], []).append(p)
    feats = []
    for dev, pts in by_dev.items():
        # v1.1: 도착 순서를 신뢰하지 않는다 — ts_fix(관측시각) 우선, seq로 타이브레이크
        pts.sort(key=lambda r: ((r.get("ts_fix") or r.get("ts") or ""), r.get("seq") or 0))
        coords = [[float(p["lon"]), float(p["lat"])] for p in pts]
        if len(coords) >= 2:
            feats.append({"type": "Feature", "properties": {"device_id": dev, "kind": "live_track"},
                          "geometry": {"type": "LineString", "coordinates": coords}})
        if coords:
            last = pts[-1]
            feats.append({"type": "Feature",
                          "properties": {"device_id": dev, "kind": "live_now", "ts": last.get("ts"),
                                         "seq": last.get("seq"), "batt": last.get("batt"),
                                         "ts_source": last.get("ts_source"),
                                         "fix_quality": last.get("fix_quality")},
                          "geometry": {"type": "Point", "coordinates": coords[-1]}})
    return {"type": "FeatureCollection", "features": feats}


# 펌웨어 F_* 비트와 같은 값(firmware/drifter_a7670_cat1 의 F_RECOVER 등).
FLAG_NAMES = [(0x01, "recover"), (0x02, "out_of_zone"), (0x04, "stranded"),
              (0x08, "last_report"), (0x10, "revived")]


def recovery_report():
    """회수 목록. 단말마다 마지막 레코드(ts_fix→seq 순)를 보고, 그 flags 가 0 이 아니면
    목록에 올린다. flags 를 보내지 않는 구버전 단말은 평상으로 읽는다(목록에 안 올린다)."""
    by_dev = {}
    for p in _read_pings():
        by_dev.setdefault(p["device_id"], []).append(p)
    now = datetime.now(timezone.utc)
    items = []
    for dev, pts in by_dev.items():
        pts.sort(key=lambda r: ((r.get("ts_fix") or r.get("ts") or ""), r.get("seq") or 0))
        last = pts[-1]
        fl = int(last.get("flags") or 0)
        if not fl:
            continue
        seen = _parse_ts(last.get("server_recv_ts") or last.get("ts"))
        items.append({"device_id": dev, "site_id": last.get("site_id"),
                      "flags": fl, "reasons": [n for b, n in FLAG_NAMES if fl & b],
                      "lat": last["lat"], "lon": last["lon"],
                      "ts_fix": last.get("ts_fix"), "seq": last.get("seq"),
                      "batt": last.get("batt"),
                      "hours_since_recv": round((now - seen).total_seconds() / 3600, 1) if seen else None})
    # 회수 요청·마지막 보고를 앞에, 그다음 오래 조용한 순
    items.sort(key=lambda r: (-int(bool(r["flags"] & 0x09)), -(r["hours_since_recv"] or 0)))
    return {"generated_at": now.isoformat(), "count": len(items), "devices": items}


def _haversine_m(lat1, lon1, lat2, lon2):
    import math
    R = 6371000.0
    p1, p2 = math.radians(lat1), math.radians(lat2)
    dp, dl = math.radians(lat2 - lat1), math.radians(lon2 - lon1)
    a = math.sin(dp/2)**2 + math.cos(p1)*math.cos(p2)*math.sin(dl/2)**2
    return 2*R*math.asin(min(1.0, a**0.5))


def _parse_ts(s):
    from datetime import datetime
    if not s:
        return None
    try:
        return datetime.fromisoformat(str(s).replace("Z", "+00:00"))
    except ValueError:
        return None


def qc_report():
    """FF-ID v1.1 §4 참조 구현 — ①2단계 4σ 속도 QC(Hansen&Poulain식: 전진·후진 유한차분
    양쪽 모두 4σ 초과인 점만 bad) ②좌초 탐지(마지막 3점이 20m 안·정체면 stranded).
    벤치·소규모용 순수 파이썬. 운영 이식 시 이 로직을 그대로 옮긴다."""
    by_dev = {}
    for p in _read_pings():
        by_dev.setdefault(p["device_id"], []).append(p)
    out = {}
    for dev, pts in by_dev.items():
        pts.sort(key=lambda r: ((r.get("ts_fix") or r.get("ts") or ""), r.get("seq") or 0))
        times = [_parse_ts(p.get("ts_fix") or p.get("ts")) for p in pts]
        # 구간 속도(m/s). 시각 결측/역행 구간은 None
        spd = []
        for i in range(len(pts) - 1):
            t0, t1 = times[i], times[i + 1]
            if not t0 or not t1 or (t1 - t0).total_seconds() <= 0:
                spd.append(None); continue
            d = _haversine_m(float(pts[i]["lat"]), float(pts[i]["lon"]),
                             float(pts[i+1]["lat"]), float(pts[i+1]["lon"]))
            spd.append(d / (t1 - t0).total_seconds())
        vals = [v for v in spd if v is not None]
        flagged = []
        if len(vals) >= 6:  # 표본이 적으면 통계가 무의미 — 플래그 유보(정직)
            # ★소표본에서 평균·표준편차는 이상치에 오염되어 자기 자신을 못 잡는다(검증됨).
            #   GDP 4σ의 취지를 강건 통계로 구현: 중앙값 + 4×(1.4826×MAD). MAD=0이면 하한 적용.
            sv = sorted(vals)
            med = sv[len(sv)//2]
            mad = sorted(abs(v - med) for v in vals)[len(vals)//2]
            sigma = max(1.4826 * mad, 0.1 * med, 0.05)   # 하한: 중앙값의 10% 또는 0.05 m/s
            thr = med + 4 * sigma
            for i in range(1, len(pts) - 1):
                fwd, bwd = spd[i], spd[i - 1]   # 점 i의 나가는/들어오는 속도
                if fwd is not None and bwd is not None and fwd > thr and bwd > thr:
                    flagged.append(pts[i].get("seq") if pts[i].get("seq") is not None else i)
        # 좌초: 마지막 3점이 서로 20m 이내
        motion = "unknown"
        if len(pts) >= 3:
            tail = pts[-3:]
            dmax = max(_haversine_m(float(a["lat"]), float(a["lon"]), float(b["lat"]), float(b["lon"]))
                       for a in tail for b in tail)
            motion = "stranded" if dmax < 20.0 else "afloat"
        out[dev] = {"n": len(pts), "flagged_4sigma": flagged, "motion_state": motion,
                    "speed_mean_ms": round(sum(vals)/len(vals), 3) if vals else None}
    return {"ok": True, "qc": out,
            "note": "4sigma: fwd+bwd 모두 초과 시 bad(표본<6이면 유보) · stranded: 최근3점<20m"}


def predicted_geojson():
    """예측 CSV → 최종위치 점군 + 시간별 앙상블 평균 경로."""
    if not os.path.exists(PRED_CSV):
        return {"type": "FeatureCollection", "features": [],
                "properties": {"note": "nakdong_track.csv 없음 — 먼저 표류 데모 실행"}}
    last = {}          # pid -> (time, lon, lat)
    by_time = {}       # time -> [lon_sum, lat_sum, n]
    with open(PRED_CSV, encoding="utf-8") as f:
        for row in csv.DictReader(f):
            pid = row["particle_id"]; t = row["time"]
            lon = float(row["lon"]); lat = float(row["lat"])
            last[pid] = (t, lon, lat)
            a = by_time.setdefault(t, [0.0, 0.0, 0])
            a[0] += lon; a[1] += lat; a[2] += 1
    endpoints = [{"type": "Feature", "properties": {"kind": "pred_endpoint", "particle_id": pid},
                  "geometry": {"type": "Point", "coordinates": [round(lon, 6), round(lat, 6)]}}
                 for pid, (t, lon, lat) in last.items()]
    mean = [[round(s[0] / s[2], 6), round(s[1] / s[2], 6)] for t, s in sorted(by_time.items())]
    feats = endpoints + [{"type": "Feature", "properties": {"kind": "pred_mean_path"},
                          "geometry": {"type": "LineString", "coordinates": mean}}]
    return {"type": "FeatureCollection", "features": feats,
            "properties": {"particles": len(last), "steps": len(mean)}}


def _read_ndjson(path):
    if not os.path.exists(path):
        return []
    out = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    out.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return out


def game_stats():
    """게임 시뮬 궤적 누적 집계 — '모두가 함께 만든 표류 학습 데이터' 카운터용.
    실드리퍼 라벨(pings/predicted)과 완전 분리된 별도 풀이다."""
    tracks = _read_ndjson(GAME_DRIFTS)
    collected = sum(int(t.get("game_collected", 0) or 0) for t in tracks)
    captured = sum(1 for t in tracks if t.get("recovered_at_boom"))
    trapped = sum(1 for t in tracks if t.get("stranded"))
    distance = sum(float(t.get("distance_m", 0) or 0) for t in tracks)
    points = sum(len(t.get("points", []) or []) for t in tracks)
    return {"ok": True, "source": "trash-odyssey-game",
            "tracks": len(tracks), "collected_total": collected,
            "captured": captured, "trapped": trapped,
            "distance_total_m": round(distance),
            "points_total": points,
            "note": "게임 시뮬 궤적 누적(실드리퍼 실측 라벨과 분리된 학습·참여 풀)"}


def game_geojson():
    """게임 궤적을 지도 레이어로(시각화용). 실측이 아니라 시뮬임을 properties에 명시."""
    feats = []
    for t in _read_ndjson(GAME_DRIFTS):
        pts = t.get("points") or []
        coords = [[float(p["lon"]), float(p["lat"])] for p in pts if "lon" in p and "lat" in p]
        if len(coords) >= 2:
            feats.append({"type": "Feature",
                          "properties": {"track_id": t.get("track_id"), "kind": "game_sim_track",
                                         "is_estimate": True, "source": "trash-odyssey-game",
                                         "recovered_at_boom": t.get("recovered_at_boom"),
                                         "stranded": t.get("stranded")},
                          "geometry": {"type": "LineString", "coordinates": coords}})
    return {"type": "FeatureCollection", "features": feats,
            "properties": {"note": "게임 시뮬 궤적(비실측) — 실드리퍼 레이어와 구분"}}


class H(BaseHTTPRequestHandler):
    def _cors(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")

    def _send(self, code, body, ctype="application/json"):
        data = body.encode("utf-8") if isinstance(body, str) else body
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self._cors()
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def log_message(self, *a):
        pass

    def do_OPTIONS(self):
        # CORS 프리플라이트(브라우저 game → cross-origin POST) 허용.
        self.send_response(204)
        self._cors()
        self.send_header("Content-Length", "0")
        self.end_headers()

    def do_POST(self):
        path = self.path.split("?")[0].rstrip("/")
        n = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(n) or b"{}"

        if path == "/api/game-drift":
            # 게임(쓰레기의 여정) 시뮬 궤적 적재 — DriftTrack 호환, is_estimate=true 전제.
            try:
                body = json.loads(raw)
            except (json.JSONDecodeError, UnicodeDecodeError) as e:
                return self._send(400, json.dumps({"error": f"bad json: {e}"}))
            arr = body if isinstance(body, list) else [body]
            stored = 0
            with open(GAME_DRIFTS, "a", encoding="utf-8") as f:
                for rec in arr:
                    if not isinstance(rec, dict) or not rec.get("track_id") or not rec.get("points"):
                        continue
                    rec.setdefault("source", "trash-odyssey-game")
                    rec["is_estimate"] = True  # 게임은 항상 시뮬 표본
                    rec["ingested_ts"] = datetime.now(timezone.utc).isoformat()
                    f.write(json.dumps(rec, ensure_ascii=False) + "\n")
                    stored += 1
            return self._send(200, json.dumps({"ok": True, "stored": stored, "stats": game_stats()}))

        if path == "/api/share":
            # 공유 이벤트 기록(데모) — 이메일+공유횟수. PII 최소(이메일만, 동의 전제).
            try:
                body = json.loads(raw)
            except (json.JSONDecodeError, UnicodeDecodeError) as e:
                return self._send(400, json.dumps({"error": f"bad json: {e}"}))
            rec = {"email": str(body.get("email", ""))[:120],
                   "shares": int(body.get("shares", 0) or 0),
                   "river": str(body.get("river", ""))[:60],
                   "source": "trash-odyssey-game",
                   "ts": datetime.now(timezone.utc).isoformat()}
            if not rec["email"]:
                return self._send(400, json.dumps({"error": "email required"}))
            with open(os.path.join(BASE, "data", "shares.ndjson"), "a", encoding="utf-8") as f:
                f.write(json.dumps(rec, ensure_ascii=False) + "\n")
            return self._send(200, json.dumps({"ok": True, "stored": {"email": rec["email"], "shares": rec["shares"]}}))

        if path == "/api/ping":
            try:
                body = json.loads(raw)
                now_iso = datetime.now(timezone.utc).isoformat()
                ts_fix = body.get("ts_fix") or body.get("ts") or None  # v1.0 "ts" → ts_fix 매핑
                rec = {"device_id": str(body["device_id"]),
                       "lat": float(body["lat"]), "lon": float(body["lon"]),
                       "ts": ts_fix or now_iso,                        # 정렬 폴백(구버전 호환 키)
                       "server_recv_ts": now_iso,
                       "ts_source": "device_fix" if ts_fix else "server_recv"}
                if ts_fix:
                    rec["ts_fix"] = ts_fix
                for k in ("site_id", "seq", "batt", "sample_interval_s",
                          "fix_quality", "gnss_source", "motion_state"):
                    if body.get(k) is not None:
                        rec[k] = body[k]
                # 회수 플래그(schema §1 flags). 0 이면 평상. 범위 밖 값은 거부한다.
                if body.get("flags") is not None:
                    fl = int(body["flags"])
                    if not 0 <= fl <= 0xFF:
                        raise ValueError(f"flags out of range: {fl}")
                    rec["flags"] = fl
                # ★서버 부여 신원 + 고정 컨텍스트(schema §1) — 그래야 하류 표준 export(CF/STA)가
                #   유효해진다(@iot.id·provenance 누락으로 K1 게이트 자기탈락하던 문제 해소).
                #   ★서버가 실제로 아는 것만 부여한다 — builder·qc는 날조하지 않는다.
                rec.setdefault("schema_version", "1.1")
                rec.setdefault("medium", "freshwater")   # 설계 고정값(담수). 기수 배포 시 body가 덮음
                rec.setdefault("link", "cellular")
                rec["obs_id"] = str(body["obs_id"]) if body.get("obs_id") else (
                    f"{rec['device_id']}#{rec['seq']}" if rec.get("seq") is not None
                    else f"{rec['device_id']}#{now_iso}")
                # provenance: 인제스트가 아는 사실만(수신 시각·시각 출처). 빌드측 provenance
                # (builder·bom_tier)는 body가 주면 병합, 없으면 서버가 지어내지 않는다.
                rec["provenance"] = {"ingest": "seacut-ingest", "recv_ts": now_iso,
                                     "ts_source": rec["ts_source"]}
                if isinstance(body.get("provenance"), dict):
                    rec["provenance"].update(body["provenance"])
            except (KeyError, ValueError, json.JSONDecodeError) as e:
                return self._send(400, json.dumps({"error": f"bad ping: {e}"}))
            # 멱등성: 펌웨어 store-and-forward 재전송(2xx 유실 후 재시도) 시 (device_id, seq)
            # 중복을 재적재하지 않는다. 파일 전체 스캔 = 벤치 규모 전제(운영은 DB로 이식).
            if "seq" in rec:
                for p in _read_pings():
                    if p.get("device_id") == rec["device_id"] and p.get("seq") == rec["seq"]:
                        return self._send(200, json.dumps({"ok": True, "dup": True, "stored": p}))
            with open(PINGS, "a", encoding="utf-8") as f:
                f.write(json.dumps(rec, ensure_ascii=False) + "\n")
            return self._send(200, json.dumps({"ok": True, "stored": rec}))

        self._send(404, json.dumps({"error": "not found"}))

    def do_GET(self):
        p = self.path.split("?")[0].rstrip("/")
        if p in ("", "/"):
            return self._send(200, MAP_HTML, "text/html; charset=utf-8")
        if p == "/api/live.geojson":
            return self._send(200, json.dumps(live_geojson()))
        if p == "/api/predicted.geojson":
            return self._send(200, json.dumps(predicted_geojson()))
        if p == "/api/qc.json":
            return self._send(200, json.dumps(qc_report()))
        if p == "/api/recovery.json":
            return self._send(200, json.dumps(recovery_report()))
        if p == "/api/game-drift/stats":
            return self._send(200, json.dumps(game_stats()))
        if p == "/api/game-drift.geojson":
            return self._send(200, json.dumps(game_geojson()))
        self._send(404, json.dumps({"error": "not found"}))


MAP_HTML = """<!doctype html><html><head><meta charset="utf-8">
<title>SEA:CUT — 낙동강 하구 드리프터</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
<style>html,body,#map{height:100%;margin:0}#hud{position:absolute;z-index:1000;top:10px;left:10px;
background:#fff;padding:8px 12px;border-radius:8px;font:13px sans-serif;box-shadow:0 1px 6px rgba(0,0,0,.2)}</style>
</head><body><div id="map"></div>
<div id="hud">SEA:CUT 낙동강 하구<br><span id="s">불러오는 중…</span></div>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script>
const map=L.map('map').setView([35.09,128.95],12);
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png',{attribution:'© OpenStreetMap'}).addTo(map);
let predL,liveL;
async function refresh(){
  const [pred,live]=await Promise.all([
    fetch('/api/predicted.geojson').then(r=>r.json()),
    fetch('/api/live.geojson').then(r=>r.json())]);
  if(predL)map.removeLayer(predL); if(liveL)map.removeLayer(liveL);
  predL=L.geoJSON(pred,{pointToLayer:(f,ll)=>L.circleMarker(ll,{radius:3,color:'#2a78d6',weight:0,fillOpacity:.5}),
    style:f=>f.properties.kind==='pred_mean_path'?{color:'#d85a30',weight:3}:{}}).addTo(map);
  liveL=L.geoJSON(live,{pointToLayer:(f,ll)=>L.circleMarker(ll,{radius:7,color:'#fff',weight:2,fillColor:'#0f6e56',fillOpacity:1}),
    style:f=>f.properties.kind==='live_track'?{color:'#0f6e56',weight:4}:{}}).addTo(map);
  document.getElementById('s').textContent=
    `예측 입자 ${pred.properties?.particles||0} · 실측 ping ${live.features.filter(f=>f.geometry.type==='Point').length}대`;
}
refresh(); setInterval(refresh,4000);
</script></body></html>"""


if __name__ == "__main__":
    print(f"SEA:CUT ingest server → http://{BIND}:{PORT}  (FF-ID schema v1.1)")
    print(f"  POST /api/ping  ·  GET /api/live.geojson  ·  GET /api/predicted.geojson")
    print(f"  POST /api/game-drift  ·  GET /api/game-drift/stats  ·  GET /api/game-drift.geojson")
    if BIND == "127.0.0.1":
        print("  ★실보드 벤치는 LAN 바인딩: python ingest_server.py 8770 0.0.0.0")
        print("    (펌웨어 SERVER_HOST = 이 PC의 LAN IP, 방화벽에서 포트 허용)")
    ThreadingHTTPServer((BIND, PORT), H).serve_forever()
