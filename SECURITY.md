# Security, Data Integrity and Content Policy

## Reporting a vulnerability
Please do not open a public issue for security problems. Email **CONTACT_EMAIL** with the details.
We aim to acknowledge within 7 days.

## What the reference ingest server does and does not do
`server/ingest_server.py` is a **reference implementation** for learning and small deployments.
- It accepts pings that match the FF-ID schema ([data/schema.md](data/schema.md)) and rejects duplicates by `seq`.
- It does **not** authenticate senders. Operator authentication (registered operator + per-operator key,
  unauthenticated data held back until verified) is **specified but not yet implemented**
  ([docs/DIY_DIVERSITY.md](docs/DIY_DIVERSITY.md) §5).
- Anyone running it on the public internet should put it behind authentication and rate limiting.

## Inappropriate, false, or illegal content
The data commons holds device positions, time, battery and quality fields, plus two short identifier
strings (`device_id`, `site_id`). There are no message or image fields, so the data cannot carry
pictures or long text; the identifier strings are the only place a mistake could put personal data.
- **False or spam tracks**: maintainers remove tracks that fail quality control, cannot be tied to a
  registered unit, or are reported as fabricated. Removal is logged.
- **Personal data submitted by mistake** (for example a phone number in a device name): removed on
  report and within 7 days of discovery.
- **Content in issues and pull requests** follows [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
Report any of the above to **CONTACT_EMAIL**.
