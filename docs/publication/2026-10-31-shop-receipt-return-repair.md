# Black Market receipt return repair — source candidate

The reproduced Spring purchase grants and saves the item, displays its receipt,
then returns the camera to the keeper without opening “Keep shopping?”. Earlier
Summer and Autumn log sequences also acknowledge the grants before the reported
lock. The Spring recording locates the visible failure after the receipt closes.

The MM adapter waits until the custom receipt has completely left the player
get-item action. This misses the native get-item tail's automatic conversation
continuation. EnOssan_ItemPurchased can then advance to ContinueShopping when its
camera starts, before any talk acceptance opens the continuation textbox.

The candidate restores the native offer/Player_StartTalking handoff while the
adapter still owns the delivered receipt. It waits for Player_Action_Talk before
completing the receipt. EnOssan retains authority for the continuation message,
camera, restocking, Yes/No and exit. The temporary player text ID is zero so a
deferred put-away callback cannot independently reopen a prompt.

The private phase records the receipt ticket, player/play and main-camera
identity. Native global TALK does not record a target/subcamera, so only a global
talk started by this handoff is allowed through the completion predicate.
Permanent phase ownership loss aborts the reservation. Cancellation/destruction
retires a matching native player callback and targets through the native idle
exit; a foreign player action/callback/target is preserved. Old-arena teardown
with no live play pointer drops the private snapshot without dereferencing it.

## Source evidence

- Baseline: public `628e891c683f5cd5b02a5570b1572ad62d42dd37`, tree
  `31d6341a9135b1d3180760118a45b4715282a0e2`.
- The four edited shop/test files are unchanged between the initial morning
  baseline `1514491385626d2ac06b463eede4c4dadf4b299b` and this newer baseline.
- Removing the fixture's forced post-receipt talk acceptance reproduces the
  missing continuation prompt on the original source.
- The updated regression compiles the actual C actor offer/acceptance,
  Player_StartTalking, Player_SetupTalk and blocking-state predicates. It runs
  the complete native EnOssan actor and actual cutscene/camera manager.
- Forty receipt cases cover all five MM forms, immediate and 30-frame put-away,
  both Yes/No choices at the first available prompt, and queued/unstarted native
  global talk. Fourteen further cases interrupt the handoff through context,
  scene, transition, camera identity, acceptance, player takeover or destruction.
- Native shop/camera integration, ordinary/private adapter parity and the
  BlackMarket.cpp real-header syntax check pass. Selection/return envelopes
  remain clear of 7,577 room and 144 shelf triangles for all forms and slots.

Animation services, native player action installation/exit, input, messages and
resource services retain controlled fixture boundaries. These checks establish
source behavior, not a complete Windows executable build or configured gameplay.

## Runtime acceptance still required

Build the combined candidate, purchase an unowned season, finish every receipt
page, and verify the continuation prompt appears. Check both Yes (browse and buy
again) and No (walk away), including Spring, Summer and Autumn where available.
Confirm each purchase charges/grants once and persists after reload. No configured
runtime acceptance or master promotion is recorded by this source checkpoint.
