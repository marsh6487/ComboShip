#!/usr/bin/env python3
"""Keep standard-only package checks active when no external HD is supplied."""
import contextlib
import io
from pathlib import Path
import tempfile
import zipfile

from run_bundled_asset_tests import PREFIX, ROOT, STANDARD, verify

SHADOW_KEYS = (
    "objects/object_nei_shadow_crystal/gNeiShadowCrystalDL",
    "textures/icon_item_custom/gItemIconShadowCrystalTex",
    "textures/item_name_custom/gShadowCrystalNameTex",
)
standard = (ROOT / "soh/assets/custom" / PREFIX / STANDARD).read_bytes()

with tempfile.TemporaryDirectory(prefix="wolf-standard-package-") as temporary:
    pack = Path(temporary) / "fixture.o2r"
    for owner in ("soh_archive", "mm_archive"):
        for missing in (None, *SHADOW_KEYS):
            with zipfile.ZipFile(pack, "w") as archive:
                archive.writestr(PREFIX + STANDARD, standard)
                for key in SHADOW_KEYS:
                    if key != missing:
                        archive.writestr(key, b"package-presence fixture")
            try:
                with contextlib.redirect_stdout(io.StringIO()):
                    verify(**{owner: pack})
            except AssertionError as error:
                assert missing and "requires a separate Shadow Crystal pack" in str(error), error
            else:
                assert missing is None, f"{owner}: accepted package missing {missing} without --hd-archive"
print("PASS standard-only package checks: both owners accept complete packs and reject each missing Shadow Crystal resource")
