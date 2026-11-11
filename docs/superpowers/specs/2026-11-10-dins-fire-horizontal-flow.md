# Din's Fire POC2 — horizontal flame flow

cor reviewed the POC1 animation and pointed out that the supplied reference's
fire is almost scrolling horizontally. Dense reinspection of the original
clip at 1.20–1.90 seconds confirms broad, winding flame ridges sweeping laterally
across an already established surrounding volume. Upward curls are secondary.

POC1's principal material motion is vertical: S coordinates restart at 0/63 on
every quad, while only T scrolls. Its separate upright sheets therefore miss
the reference. Retain POC1 at commit
`f8e1c4c1db99e79ff02139b10fb11cc5b75b0b49` as a recoverable, visually unaccepted
checkpoint. POC2 isolates the correction on
`candidate/dins-fire-remake-poc2-horizontal-20261110`.

## Bounded correction

Replace the disconnected upright shell cards with a shared rounded latitude
profile and continuous circumferential UV mapping. Animate S as the dominant
material flow, with only slow secondary vertical drift/curl. Stretch the existing
flame material laterally so large ridges and openings read as winding bands.
Keep the hollow surrounding volume and foreground arc; do not flatten the fire
into a ground disc. Breakup should remove/fade band patches rather than leave
rows of tall isolated strips.

Changing the scroll axis alone would keep POC1's repeated card boundaries;
flattening the mesh would lose the reference's depth. The continuous rounded
shell with independent S/T mapping is the selected implementation approach for
this correction, subject to another visual preview.

Preserve the complete green pre-cast/hand flame sequence, authored texture bytes,
cosmetics, menus, native timing, gameplay, medallion/weapon isolation, resource
fallbacks, native display-scope reservation fix and bounded allocations.

## Evidence needed

Exercise actual native submitted UVs for both hosts: circumferential continuity,
dominant S motion, finite/range-safe coordinates and unchanged charge mapping.
Retain existing native draw/fallback and arena-boundary checks. Export the revised
production mesh through the same independently verified native actor timeline,
and inspect a separate animated preview before calling the design accepted.

Configured in-game appearance remains untested. The existing baseline MM damage
diagnostic failure is unchanged and outside this visual correction. No source
publication or promotion is authorized by this pass.

## Observed POC2 result

The initial offline sequence shows horizontal winding channels traveling over
the rounded shell, with the green/hand-charge sequence preserved. The reference
comparison still identifies finer, darker and more uniformly distributed flame
detail in this candidate. Broad soft flame bodies and irregular bright crests
remain material-direction work; this checkpoint is not a design acceptance.

Native redraws reuse vertex texture coordinates while interpolating matrices,
so the final preview holds actual 20 Hz mesh/UV samples instead of adding a
smoother coordinate animation that the source does not supply. The appended
half-speed hold replay is explicitly a review aid.
