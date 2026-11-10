"""Production data-only dependencies for extracted foreign GI descriptors."""
from pathlib import Path

from run_time_pedestal_tests import functions


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
