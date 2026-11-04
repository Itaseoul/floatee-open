> **Archived.** This describes an earlier draft (tiers, wide-mouth container or 3D-printed inserts). The current build is the PET-bottle drifter v4: [https://floatee.caresea.kr/hardware](https://floatee.caresea.kr/hardware).

# Build guide (outline)

Step-by-step in the Precious Plastic Academy spirit: short steps, a photo or short clip per step. This is the outline; fill each step with a photo as you build your first unit.

## Tier 1 — Cellular Bottle

1. Gather parts. See [../hardware/BOM.md](../hardware/BOM.md). Confirm your board is the A7670 "With GPS (L76K)" variant for your region's bands.
2. Print the inserts. `../hardware/cad/` — bracket, ballast keel, recovery loop. Measure your bottle first and edit `params.scad`.
3. Flash the firmware. [../firmware/](../firmware/) with lewisxhe/TinyGSM-fork. Set APN and server URL.
4. Bench test (battery installed). Confirm modem attaches, GPS gets a fix outdoors, and one ping reaches your server (200). USB-only power will brown out; always use the 18650.
5. Assemble. Board and cell on the bracket, lead-free weight in the keel at the belly, antenna routed to the top and kept above the waterline.
6. Seal and float-test. Seal pass-throughs and the cap. Float in a tank: it should lie on its side, sit about half-submerged, and right itself antenna-up when you roll it. Adjust ballast until it does.
7. Add visibility and recovery. Bright tape at the waterline, waterproof QR/contact label, recovery loop on top.
8. Soak test 24 h. Check for leaks and condensation; confirm the battery lasts and pings keep arriving.
9. Field signal test. Walk the target stream holding the unit; confirm pings arrive in real time and your roaming SIM actually attaches.
10. Short release and recover. Release in a safe, accessible reach, track it, and recover it. Two people, life vests, a pole or net. Recover every unit.

## Tier 0 — Phone-in-a-Bottle

Reuse an old phone with GPSLogger posting to your server URL, sealed in a dry bag inside a bottle with bottom ballast and a recovery loop. This proves the whole pipeline for almost nothing, in any country. Do this first.
