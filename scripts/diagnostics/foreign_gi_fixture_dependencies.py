"""Production dependencies and inactive-path boundaries for foreign GI fixtures."""
from pathlib import Path

from run_time_pedestal_tests import functions


def startup_receipt_classifier(root: Path) -> str:
    """Retain the actual receipt-family predicate and public adapter."""
    source = (root / "soh/soh/Enhancements/Gameplay/ComboStartupReceipt.cpp").read_text()
    bodies = functions(source.replace('extern "C" ', ''))
    return ('#include "combo/rando/StartupCheckReceipt.h"\n'
            'namespace {\n' + bodies["IsReceipt"] + '\n}\n'
            'extern "C" ' + bodies["ComboStartupOot_IsReceipt"] + '\n')


def startup_foreign_receipt_boundary(root: Path) -> str:
    """Ordinary-item probes must fail if dispatched through a startup snapshot.

    Positive frozen-receipt rendering is exercised by tests/startup_receipt.
    Keep the production classifier so these boundaries cannot hide an ordinary
    item accidentally acquiring the startup token bit.
    """
    return startup_receipt_classifier(root) + r'''
#include <cstdio>
#include <cstdlib>
extern "C" const char* ComboStartupOot_FrozenForeignName(const GetItemEntry*) {
    std::fputs("ordinary-item fixture entered startup receipt lookup\n", stderr);
    std::abort();
}
inline const ComboForeignDrawInfo* ComboResolveFrozenForeignDrawInfo(const char*) {
    std::fputs("ordinary-item fixture entered frozen foreign rendering\n", stderr);
    std::abort();
}
'''


def clock_descriptor(root: Path) -> str:
    """Keep the real Clock recipe without importing either engine's draw types."""
    source = (root / "combo/NeiClockGI.h").read_text()
    constants = source[:source.index("inline const char* TextureForGlass(")]
    # Both owner paths belong to the extracted combined-game Describe body.
    # Copy their declarations verbatim; its surrounding COMBO_BUILD guard is
    # supplied by the production consumer, not by this header-free fixture.
    constants = "\n".join(line for line in constants.splitlines()
                          if line.startswith("inline constexpr "))
    return ('#include "combo/menu/ComboItemDrawABI.h"\n'
            '#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"\n'
            '#include <cstring>\nnamespace NeiClockGi {\n' + constants + "\n" +
            functions(source)["Describe"] + "\n}\n")
