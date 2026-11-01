# Cumulative shop, startup, Chamber and native-pot checkpoint

The cumulative source checkpoint is `37b6be75d4f1682688ec325ae00dd3184c27be4e`,
tree `f55b402803767ec7dbb6794cec912da8dbbc312f`, descended from the tested-build
source `628e891c683f5cd5b02a5570b1572ad62d42dd37`. The existing companion/enemy
integrations, morning repairs and default-off indexed-rendering control remain
inherited. Publication additions preserve the pot asset recipe/reference data
and the original source history; they do not modify the verified engine files.

## Included repairs

- MM Black Market item receipts restore the native conversation handoff so the
  keeper can offer both continuation choices after the receipt closes.
- The generic OoT resource loader preserves valid Blob resources, with type
  validation at the scene and optional Navi-hint consumers. The demonstrated
  `elf_message_field` fault predates and precedes companion/enemy execution.
- Normal port assets include native Unbound Temple/Sages controls and a scoped
  Chamber resource layer. Both asset modes retain canonical Temple story/spawn
  data, and optional visual addons can still override the built-in resources.
- The separate Kakariko POC12 asset correction restores native position keys for
  the two outer potion-shop pots in child day/night. Its exact room member,
  bounded repair, verifier, provenance and native reference excerpts are tracked
  in `tools/prelude_kakariko_pot_identity/`. Engine randomizer keys are unchanged.

The detailed source, donor, native-boundary and pending gameplay records remain
in the shop-receipt, startup-elf-message and Temple/Sages publication documents.

## Fresh verification

All 15 focused check runners passed from an isolated checkout: five startup/
scene/room/entrance/path runners; native MM shop, adapter parity and production
syntax; Chamber controls, packaged assets, native material checks, actual native
archive-layer checks and formatting; Unbound startup; original asset preservation.
The optional visual-donor case in the native material runner was skipped because
its separate input was not supplied. Cached real dependency headers were used
for the native compiler checks. A fresh checkout's inherited sparse exclusions
were disabled before testing; no production change was needed.

Chamber preparation produced exact 4,104-file trees for both ports. All original
4,096 payloads retain their bytes and native artwork. A separate clone with
independent object storage restored the source capsule over the pinned baseline
and reproduced tree `f55b402803767ec7dbb6794cec912da8dbbc312f` exactly. The separate
pot capsule independently reproduced POC12's complete archive SHA-256.

Full Windows/Linux builds of this final source and configured gameplay remain
pending. Earlier full-build results do not establish this candidate's build or
gameplay acceptance. No protected/default branch or accepted master is merged,
and no game release is published.

## Original history and rollback

`docs/recovery/oct31-shop-startup-chamber-37b6be75.bundle` retains all eight
original repair/integration commits above the already published parent. It is
a bounded recovery bundle and requires that parent and its ancestors.

```sh
git fetch origin 628e891c683f5cd5b02a5570b1572ad62d42dd37
git bundle verify docs/recovery/oct31-shop-startup-chamber-37b6be75.bundle
git fetch docs/recovery/oct31-shop-startup-chamber-37b6be75.bundle refs/heads/fix/initialization-shop-receipt-20261031-poc1:refs/heads/recovery/oct31-shop-startup-chamber
git rev-parse recovery/oct31-shop-startup-chamber^{tree}
```

The last command must print `f55b402803767ec7dbb6794cec912da8dbbc312f`.
Use separate rollback branches or worktrees; retain the cumulative baseline and
the POC11 archive. Runtime acceptance still needs the reported MM-to-OoT Lost
Woods handoff, seasonal shop purchases with both continuation choices, and
child/adult Temple/Chamber traversal and portal returns without an external
Chamber archive in both asset modes.
