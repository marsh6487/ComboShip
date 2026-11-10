"""A diagnostic retry must preserve the original native seed failure."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "scripts/ci/run_seed_smoke.py"


class SeedSmokeTests(unittest.TestCase):
    def run_case(self, code):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            calls = directory / "debugger-called"
            debugger = directory / "gdb"
            debugger.write_text("#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$SEED_TEST_CALLS\"\nexit 0\n")
            debugger.chmod(0o755)
            result = subprocess.run(
                [sys.executable, str(RUNNER), sys.executable, "-c", code],
                env={**os.environ, "PATH": str(directory) + os.pathsep + os.environ["PATH"],
                     "SEED_TEST_CALLS": str(calls)}, capture_output=True, text=True)
            return result, calls.read_text() if calls.exists() else ""

    def test_success_does_not_run_debugger(self):
        result, calls = self.run_case("print('seed passed')")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(calls, "")
        self.assertIn("seed passed", result.stdout)

    def test_failed_generation_keeps_failure_even_when_debugger_succeeds(self):
        result, calls = self.run_case("raise SystemExit(17)")
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertIn("thread apply all bt 24", calls)
        self.assertIn("--args", calls)

    def test_native_signal_becomes_shell_failure_status(self):
        result, calls = self.run_case("import os,signal; os.kill(os.getpid(),signal.SIGTERM)")
        self.assertEqual(result.returncode, 143, result.stderr)
        self.assertTrue(calls)


if __name__ == "__main__":
    unittest.main()
