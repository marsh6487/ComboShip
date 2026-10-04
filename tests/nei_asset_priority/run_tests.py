"""Archive provenance, Alt selection and stock companion regression checks."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="nei-priority-") as directory:
    binary = str(Path(directory) / "priority")
    subprocess.run(["c++", "-std=c++20", str(ROOT / "tests/nei_asset_priority/priority_test.cpp"), "-o", binary], check=True)
    subprocess.run([binary], check=True)
print("PASS: base/Alt mod provenance, exact stock/companion paths, live selection changes")
