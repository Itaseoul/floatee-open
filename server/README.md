# Ingest server — 60-second quickstart

`ingest_server.py` is the reference ingest endpoint: a single file, **no dependencies** (Python 3
standard library only). It receives one small JSON ping per device wake, assigns the citation
`obs_id` + provenance, assembles the track, and serves a public map / open data — the
`[drifter] → [ingest server] → [public map / open data]` pipeline from the [README](../README.md).

This closes the reproducibility gate: cloning the public repo now gives you the server, so
`device → server → map` runs end-to-end without hunting for a file.

## Run it

```bash
python3 server/ingest_server.py 8770 0.0.0.0        # port 8770, all interfaces
# → logs "FF-ID schema v1.1"
```

Drive-run without a device (curl):

```bash
# a v1.1 ping (see data/schema.md §1)
curl -X POST http://localhost:8770/api/ping \
  -H 'Content-Type: application/json' \
  -d '{"device_id":"river-001","site_id":"nakdong-hakjang","seq":1,"lat":35.10560,"lon":128.94310,"batt":4.02,"ts_fix":"2026-07-10T09:00:00Z","sample_interval_s":900,"fix_quality":1.4,"gnss_source":"l76k"}'
# → {"ok":true, ...}   ; re-POST same seq → {"dup":true}
curl http://localhost:8770/api/live.geojson                 # assembled track
curl http://localhost:8770/api/recovery.json               # devices whose last record has non-zero flags
```

## Exposing it to a cellular device (★ the device is on LTE, not Wi-Fi)

The board reaches the internet over the carrier network, so a LAN/private IP (192.168.x.x) is
**unreachable** (carrier NAT). Expose the server publicly:

- `ngrok tcp 8770` → use the issued `host:port` as the firmware `SERVER_HOST`/`PORT`
  (`ngrok http` serves TLS and won't accept the plaintext bench client — see firmware header), OR
- router port-forward 8770 + a public IP.

## Data / license

Observations follow [data/schema.md](../data/schema.md) (FF-ID v1.1). Data is **CC BY 4.0**; run your
own server or POST to a shared community endpoint — same schema, so the data is interoperable.
Schema non-compliance = not included in the shared dataset (a gate, not enforcement).

> Reference implementation. Harden before production: TLS with a pinned CA on the device side
> (see firmware header), per-operator API-key write-auth, rate limiting, and the server-side physical
> plausibility / dedup checks described in DIY_DIVERSITY §5.2.
