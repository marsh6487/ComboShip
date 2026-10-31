# Solo MM native partner dependencies

The normal `GenerateSohOtr` and `Generate2ShipOtr` targets merge authored partner
resources before archive normalization and deployment. No optional mod archive or
user ROM is needed for these resources. Shared source lives in
`combo/assets/partners/shared.json`; these 403 unique resource payloads are identical to the
preserved SoH mod pack. `native.json` contains only the 202 additional OoT payloads
needed by Solo MM: Malon (`object_ma1`, `object_ma2`), Cojiro (`object_niw`), and the
Cojiro item icon. Each value is zlib level 9 compressed bytes encoded with Python's
Base85 alphabet. This is a bounded source dependency bundle, not a game archive.

`manifest.json` pins every native and shared payload by path and SHA-256, plus the
unchanged source archive and source member identities. The preserved source
candidate's MM pack is the exact union of those two sets. Its donor identity is
recorded in the manifest; no claim of runtime acceptance is made here.

To reproduce the source conversion from the preserved source candidate:

```python
from pathlib import Path
from zipfile import ZipFile
from io import BytesIO
import base64, hashlib, json, zlib

root = Path('.')
folder = root / 'tools/partners_native_dependencies'
manifest = json.loads((folder / 'manifest.json').read_text())
source = Path(manifest['source_archive'])
assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest['source_archive_sha256']
with ZipFile(source) as source_zip:
    data = source_zip.read(manifest['source_member'])
assert hashlib.sha256(data).hexdigest() == manifest['source_member_sha256']
with ZipFile(BytesIO(data)) as pack:
    assert set(pack.namelist()) == set(manifest['native']) | set(manifest['shared'])
    native = {name: pack.read(name) for name in sorted(manifest['native'])}
for name, data in native.items():
    assert hashlib.sha256(data).hexdigest() == manifest['native'][name]
encoded = {name: base64.b85encode(zlib.compress(data, 9)).decode()
           for name, data in native.items()}
(folder / 'native.json').write_text(json.dumps(encoded, indent=2) + '\n')
```

The merger rejects differing existing resource collisions rather than replacing
unrelated output. An identical existing entry is retained. All source hashes and
existing ZIP payloads are validated before mutation; output is verified and
atomically replaced. The subsequent normalizer retains compressed streams while
canonicalizing the complete output. Run `python3 tests/partners/test_packaging.py`
to verify synthetic archive merging, preservation, idempotence and rejection.

## Editing and recovering shared authored source

`combo/assets/partners/shared.json` is sorted by the full original
`module/resource/path` identity. Values use the same zlib9/Base85 encoding as
`native.json`. Module ownership is retained in each key; the decoded archive path
is everything after its first slash. Both manual and native packaging share
`load_shared()` and validate the unchanged 403 payload hashes in `manifest.json`.
No decoded resource changed during the text-only conversion. The raw source
checkpoint remains in Git history and the cumulative recovery bundle/source ZIP.

Decode the human-editable working tree from repository root when authoring:

```python
from pathlib import Path
import base64, json, zlib

source = Path('combo/assets/partners/shared.json')
encoded = json.loads(source.read_text())
working = Path('partner-authoring')
for identity, value in encoded.items():
    path = working / identity
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(zlib.decompress(base64.b85decode(value)))
```

After intentional authoring, encode each working-tree file under its unchanged
module/resource identity with `base64.b85encode(zlib.compress(data, 9)).decode()`
and write sorted JSON with two-space indentation and a trailing newline. Update
only the corresponding shared SHA-256 records in the manifest for intentional
payload changes, then run the packaging tests. A pure restore/re-encode requires
no manifest changes. Keep the authoring directory outside committed source.
