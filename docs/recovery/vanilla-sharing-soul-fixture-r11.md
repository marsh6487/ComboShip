# Vanilla sharing fixture native soul service closure

The unchanged registered systems suffix failed run_sharing_tests.py because the extracted FleetShared receipt/pull functions now call the owning host's shared-soul service, while the fixture omitted that contract. The preceding exact journal/receipt row passed.

This test-only closure includes each host's real SoulShuffleShared.h and real native soul item enum, extracts the actual public ShareSoulReceipt/ApplySharedSoulState function bodies, and instantiates the complete production SharedSoulRuntime. Native ownership flags and native/peer grants remain explicit controlled services; this fixture does not replace the existing full native saved-mask/flag integration tests. The resolver now distinguishes ordinary shared state from the actual shared-soul export name instead of returning one ordinary callback for every requested symbol.

All existing Vanilla/NEI assertions remain. Added controls verify ordinary receipt fallthrough, handled soul receipt stopping the unqualified legacy route, no recursive source grant, receive-depth and solo/Vanilla gates, peer slot/category constraints, ownership reconciliation/idempotence, guarded apply, and unchanged native/NEI save boundaries. Both host variants retain strict Wall/Wextra/Werror and ASan/UBSan.

Direct sanitizer runner passed both hosts. The exact unchanged registered `Combo Vanilla NEI-only native sharing, latent ownership and exact source identity` row passed through scripts/ci/run_regressions.py in25.05 seconds. No production source, required manifest registration or warning policy changed. Opening fixture and codec closures belong to souls; the remaining six systems rows are being run separately, with no complete systems-group PASS claim.
