# October 29 cumulative source publication

The implemented cumulative source is published on
`checkpoint/oct29-cumulative-indexed-triangles-20261029` in
`marsh6487/ComboShip`. Its 44,907 tracked files have exactly the source tree of
the authorized local checkpoint. The related
`candidate/oct29-bugfix-integration-20261029` branch adds this recovery record
and updated publication status without changing production code or source assets.

## Exact identities

GitHub checkpoints were reconstructed on the complete published morning history.
Commit IDs differ; each mapped source tree is identical. The original local
history is also preserved by the raw-object archive described below.

| Milestone | Original local commit | Published commit | Source tree |
| --- | --- | --- | --- |
| Preservation parent | `b43e86eaad1b3507367cc84b5d405f7b9f4aca1e` | `f6d4772fa7856ea95fd6335b4e0d99f335a198f6` | `eb7296bcdd76547d6201fdda56cd2d4262f37d44` |
| Indexed renderer | `dfc259f727d5f7aab927327da7e7c756359ca4fa` | `122886753a77b66d3d06b3dba77daf9892b7d388` | `646550de46f4851067761428d68cf11f36ef0d7a` |
| Active toon-normal test | `45a7cdbc853395dee39142cffdf90ab860666340` | `e06ee44c0e0e6dd10c216d943b7074f83df04e27` | `859bece940c7eaff4492c5c2cd279663fa4dc74b` |
| Complete cumulative source | `f2ca8f249994a69caf5d13d47e4d020ee954888f` | `32680828cdd96fec1173d9cd95069857db12f149` | `e0fe8178202834acdcca45db02acd48d38e39b09` |

This retains Unbound/collision/cel shading, RenderFlight, actor/NPC and native
loader repairs, missing-music handling, Major masks and stray-fairy chests in
both games, equipment ownership, Market distance, and authored-Chamber-only SFX.
The source, CI entries, tracked source assets, donor references and earlier
restoration records remain in the cumulative tree and its retained history.

## Verification and limits

Ten focused commands passed from the independent full-source recovery checkout:
indexed stream equivalence under ASan/UBSan, real DX11/OpenGL API-boundary
fixtures, cel backend preservation, six Market distance controls, Chamber scope
and emitter preservation, both native MM actor-loader ABIs, OoT chest routing,
MM chest routing, missing-audio handling, and equipment provenance in both games.
The JSON companion records exact commands, settings, exit codes and output hashes.

The initial equipment sanitizer invocation omitted the documented
`ASAN_OPTIONS=detect_leaks=0` setting and failed because LeakSanitizer cannot
inspect this host's process tasks. The corrected invocation passed with ASan
and UBSan enabled. Existing MM header redefinition/const warnings remain; this
publication does not claim a warning-free compile.

Independent source restoration has no alternate/shared object store and passes
`git fsck --full`; its tree equals the published tree above. All 260 prior
regression group names and commands remain; the manifest now contains 262 groups.
This publication pass reran the ten focused commands, not all 262 groups.

Full Windows/Linux application builds, actual GPU execution, configured visual
and audible acceptance, and gameplay FPS improvement remain unverified. Native
fixtures do not substitute for those checks.

Separate archive work is not promoted or asserted complete here: Hyrule Field
recipe doors/Malon timeline/grotto warppad, Market windows/textures, standalone
NPC/Young Din optimization, dungeon fog/lighting, and flame POCs. This publication
contains the tracked executable source and source assets; standalone archive
candidates remain separate.

## Restore the published source

Use a new directory:

```sh
git clone --single-branch --branch checkpoint/oct29-cumulative-indexed-triangles-20261029 https://github.com/marsh6487/ComboShip.git ComboShip-Oct29-source
cd ComboShip-Oct29-source
git rev-parse HEAD
git rev-parse 'HEAD^{tree}'
git fsck --full
```

Expected checkpoint: `32680828cdd96fec1173d9cd95069857db12f149`.
Expected tree: `e0fe8178202834acdcca45db02acd48d38e39b09`.
The candidate branch contains these instructions and the raw-history archive;
fetch it without moving the source checkout:

```sh
git fetch origin candidate/oct29-bugfix-integration-20261029
git show FETCH_HEAD:docs/recovery/oct29-original-history-20261029.json > ../oct29-original-history-20261029.json
```

## Recover the original local commit IDs

The JSON archive contains 31 SHA-verified original commit/tree/blob objects not
already reachable through the retained published source-tree milestones.
Combined with the source clone above, it restores the exact seven-commit local
lineage from the complete morning parent `22b6569a` through `f2ca8f24`.
No large source bundle or transient workspace is required.

From that clone, run the following once. It creates a new recovery ref and
refuses to replace an existing one; it does not change the current checkout.

```sh
python3 - <<'PY'
import base64, hashlib, json, subprocess, zlib
from pathlib import Path

archive = json.loads(Path('../oct29-original-history-20261029.json').read_text())
for obj in archive['objects']:
    raw = zlib.decompress(base64.b64decode(obj['zlib_base64'], validate=True))
    prefix = (obj['type'] + ' ' + str(len(raw)) + '\0').encode()
    if hashlib.sha1(prefix + raw).hexdigest() != obj['sha']:
        raise SystemExit('Original Git object hash mismatch')
    actual = subprocess.check_output(
        ['git', 'hash-object', '-w', '--literally', '-t', obj['type'], '--stdin'],
        input=raw).decode().strip()
    if actual != obj['sha']:
        raise SystemExit('Restored Git object identity mismatch')
subprocess.run(['git', 'update-ref',
    'refs/heads/recovery/original-f2ca8f24-20261029', archive['head'], '0' * 40],
    check=True)
subprocess.run(['git', 'fsck', '--connectivity-only'], check=True)
print('Original checkpoint restored:', archive['head'], archive['tree'])
PY
```

Every archived object was reconstructed and hash-checked in a fresh empty object
database. Required-object coverage was checked against the retained public
milestone trees. The empty-database test verifies object identity; full source
restoration is the separate check described above.

## Rollback and publication policy

Disable indexing while retaining cumulative fixes using the game console:

```text
set gEnhancements.IndexedTriangles 0
```

Use value `1` to enable it again. Keep the normal complete staged engine/port
set when eventually building; the renderer interface changed.

The original checkpoint branch stays pinned. Additional publication metadata is
a descendant on the candidate branch. The candidate's documentation commit uses
`[skip ci]` because the existing PR workflow automatically uploads packaged game
builds. This authorization covers source publication only: no packaged build or
release is published, no accepted master is promoted, and no protected branch is
merged. Full build/regression CI therefore remains pending under this scope.
