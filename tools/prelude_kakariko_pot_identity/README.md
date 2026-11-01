# Kakariko POC12 native pot identity source

This preserves the separate, unpromoted scene-asset correction alongside the
cumulative shop, startup and built-in Chamber source checkpoint. It does not
change the randomizer matching rules or install a Kakariko scene automatically.

Four sparse position overrides in `scenes/spot01/rooms/0.json` are restored to
the native potion-shop pot positions:

| Setup | Actor | Before | After |
| --- | --- | --- | --- |
| Child day, 0 | 8 | `[225,200,-380]` | `[222,200,-377]` |
| Child day, 0 | 10 | `[285,200,-353]` | `[284,200,-356]` |
| Child night, 1 | 7 | `[225,200,-380]` | `[222,200,-377]` |
| Child night, 1 | 9 | `[285,200,-353]` | `[284,200,-356]` |

The engine identifies native pots by their exact packed X/Z coordinates. The
middle pot already matches and remains unchanged. All other room text, actor
fields, geometry, collision, textures, audio, and the POC11 authored dog paths
are retained. All 592 other archive member payloads and compressed streams are
identical. The final room resource is committed under `resources/`; it is a
source overlay member, not a complete game archive.

## Reproduce and verify

Use Python 3.11+ and the standard library. Supply the unchanged POC11 archive
identified below; its complete preservation capsule remains a separate asset
deliverable. From this directory:

```sh
python3 -B recipe/repair_pots.py --parent /path/to/Kakariko_POC11_Authored_Dog_Paths.unbound.o2r --output /tmp/Kakariko_POC12.unbound.o2r
python3 -B recipe/verify_pots.py --parent /path/to/Kakariko_POC11_Authored_Dog_Paths.unbound.o2r --candidate /tmp/Kakariko_POC12.unbound.o2r --base reference/native_reference.o2r --donor reference/authored_path_reference.o2r --dog-verifier reference/verify_kakariko_paths.py --report /tmp/Kakariko_POC12-verification.json
```

- POC11 SHA-256: `4d3059750f71cfb907a1311990b88764b88f925ab5444a7a91a5d5116b64e558`.
- POC12 SHA-256: `c87aedf634783306d791e1c6d427fd96ee1416224c786e818026ea2df4ca5169`.
- Independent source-and-checks capsule SHA-256:
  `e607f4bb6ffb8938324b516c337f50187f7cfd0084a3e21983b3c77ca2011956`.

Reference archives contain only the pinned native/donor excerpts needed by the
verifier. Their original identities are recorded in `reference/provenance.json`.
The exact field delta, resource hashes, native key evidence and source checkpoint
are recorded in `source_manifest.json` and `evidence/`.

Source-only restoration reproduced the exact POC12 archive, all six day/night
pot identities and six dog-route lookups. Gameplay, a configured randomizer
seed, collection flags, visual/collision behavior and vanilla/Alt runtime parity
remain untested. Replace the prior POC11 overlay when testing POC12; retain it
for rollback. No accepted master is promoted by this publication.
