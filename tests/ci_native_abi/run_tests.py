#!/usr/bin/env python3
"""Exercise the production regression runner with controlled compiler executables."""

import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[2]
COMPILERS = ("cc", "c++", "gcc", "g++", "clang", "clang++")
ABI_FLAGS = ("-DGBI_FLOAT_MTX", "-DGBI_S32_VTX")
ARGUMENTS = ["-c", "input with spaces.c", "-DVALUE=two words", "literal$(untouched).o", "two\nlines"]


class NativeAbiRunnerTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="ci-native-abi-test-")
        self.addCleanup(self.temporary.cleanup)
        self.build = Path(self.temporary.name)
        self.compilers = self.build / "real compilers"
        self.compilers.mkdir()
        self.log = self.build / "compiler log.jsonl"
        program = f"#!{sys.executable}\n" + textwrap.dedent("""\
            import json
            import os
            from pathlib import Path
            import sys
            entry = {
                "compiler": Path(sys.argv[0]).name,
                "args": sys.argv[1:],
                "CPATH": os.environ.get("CPATH"),
                "CC": os.environ.get("CC"),
                "CXX": os.environ.get("CXX"),
                "PATH_HEAD": os.environ["PATH"].split(os.pathsep)[0],
            }
            with open(os.environ["FAKE_COMPILER_LOG"], "a") as output:
                output.write(json.dumps(entry) + "\\n")
            raise SystemExit(int(os.environ.get("FAKE_COMPILER_EXIT", "0")))
        """)
        for name in COMPILERS:
            compiler = self.compilers / name
            compiler.write_text(program)
            compiler.chmod(0o755)
        self.environment = dict(os.environ)
        for key in ("GITHUB_STEP_SUMMARY", "COMBO_NATIVE_TEST_ABI", "COMBO_NEI_FIXTURE_CPATH"):
            self.environment.pop(key, None)
        self.environment.update({
            "PATH": str(self.compilers) + os.pathsep + os.defpath,
            "CC": shlex.join([str(self.compilers / "gcc"), "-DCC_CALLER=two words"]),
            "CXX": shlex.join([str(self.compilers / "clang++"), "-DCXX_CALLER=still here", "-fno-common"]),
            "CPATH": "/caller/include one:/caller/include two",
            "FAKE_COMPILER_LOG": str(self.log),
            "FAKE_COMPILER_ARGUMENTS": json.dumps(ARGUMENTS),
        })

    def run_runner(self, *, task=None, environment=None, payload=None, list_only=False):
        self.log.unlink(missing_ok=True)
        payload_file = self.build / "compiler probe.py"
        payload_file.write_text(payload or textwrap.dedent("""\
            import json
            import os
            import shlex
            import subprocess
            args = json.loads(os.environ["FAKE_COMPILER_ARGUMENTS"])
            for compiler in ("cc", "c++", "gcc", "g++", "clang", "clang++"):
                subprocess.run([compiler, *args], check=True)
            for key in ("CC", "CXX"):
                subprocess.run([*shlex.split(os.environ[key]), *args], check=True)
        """))
        selected = {
            "suite": "probe", "origin": "gate", "name": "controlled compiler probe",
            "run": shlex.join([sys.executable, "-B", str(payload_file)]),
            **(task or {}),
        }
        manifest = self.build / "manifest.json"
        manifest.write_text(json.dumps([selected]))
        env = {**self.environment, **(environment or {})}
        command = [sys.executable, "-B", str(ROOT / "scripts/ci/run_regressions.py"),
                   "--suite", "probe", "--manifest", str(manifest)]
        if list_only:
            command.append("--list")
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True, timeout=30)
        entries = [json.loads(line) for line in self.log.read_text().splitlines()] if self.log.exists() else []
        return result, entries

    def assert_cleaned(self, entries):
        self.assertTrue(entries)
        directory = Path(entries[0]["PATH_HEAD"])
        self.assertNotEqual(directory, self.compilers)
        self.assertFalse(directory.exists(), "temporary compiler adapters must be removed")
        for key in ("CC", "CXX"):
            self.assertEqual(Path(entries[0][key]).parent, directory)
            self.assertFalse(Path(entries[0][key]).exists())

    def test_all_compiler_names_and_configured_flags_keep_exact_arguments_and_cpath(self):
        result, entries = self.run_runner()
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([entry["compiler"] for entry in entries], [*COMPILERS, "gcc", "clang++"])
        for entry in entries:
            for flag in ABI_FLAGS:
                self.assertEqual(entry["args"].count(flag), 1, entry)
            self.assertEqual(entry["args"][-len(ARGUMENTS):], ARGUMENTS)
            self.assertEqual(entry["CPATH"], self.environment["CPATH"])
        self.assertIn("-DCC_CALLER=two words", entries[-2]["args"])
        self.assertIn("-DCXX_CALLER=still here", entries[-1]["args"])
        self.assertIn("-fno-common", entries[-1]["args"])
        self.assert_cleaned(entries)

    def test_task_explicit_abi_optout_preserves_deliberate_legacy_flags(self):
        result, entries = self.run_runner(task={"native_abi": "explicit"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(entries), 8)
        for entry in entries:
            self.assertFalse(any(flag in entry["args"] for flag in ABI_FLAGS), entry)
        self.assert_cleaned(entries)

    def test_plain_configured_compiler_paths_with_spaces_are_supported(self):
        result, entries = self.run_runner(environment={
            "CC": str(self.compilers / "gcc"), "CXX": str(self.compilers / "g++"),
        })
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual([entry["compiler"] for entry in entries[-2:]], ["gcc", "g++"])
        for entry in entries:
            for flag in ABI_FLAGS:
                self.assertEqual(entry["args"].count(flag), 1)
        self.assert_cleaned(entries)

    def test_environment_explicit_abi_optout(self):
        result, entries = self.run_runner(environment={"COMBO_NATIVE_TEST_ABI": "explicit"})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        for entry in entries:
            self.assertFalse(any(flag in entry["args"] for flag in ABI_FLAGS), entry)
        self.assert_cleaned(entries)

    def test_nested_script_can_select_both_abis_and_use_cc_cxx_as_executable_paths(self):
        payload = textwrap.dedent("""\
            import os
            import subprocess
            subprocess.run([os.environ["CC"], "combined source.c"], check=True)
            os.environ["COMBO_NATIVE_TEST_ABI"] = "explicit"
            subprocess.run([os.environ["CC"], "legacy source.c"], check=True)
            subprocess.run([os.environ["CXX"], "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "wide source.cpp"], check=True)
        """)
        result, entries = self.run_runner(payload=payload)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(len(entries), 3)
        for flag in ABI_FLAGS:
            self.assertEqual(entries[0]["args"].count(flag), 1)
            self.assertNotIn(flag, entries[1]["args"])
            self.assertEqual(entries[2]["args"].count(flag), 1)
        self.assert_cleaned(entries)

    def test_nei_header_path_policy_is_preserved(self):
        expected = "/nei/header one:/nei/header two"
        result, entries = self.run_runner(task={"origin": "nei"},
                                          environment={"COMBO_NEI_FIXTURE_CPATH": expected})
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertTrue(all(entry["CPATH"] == expected for entry in entries))
        self.assert_cleaned(entries)

    def test_failed_task_preserves_exit_code_and_cleans_up_adapters(self):
        result, entries = self.run_runner(task={"run": shlex.join(["cc", *ARGUMENTS])},
                                          environment={"FAKE_COMPILER_EXIT": "7"})
        self.assertEqual(result.returncode, 7, result.stdout + result.stderr)
        self.assertIn("Regression failed", result.stdout)
        self.assertEqual(len(entries), 1)
        for flag in ABI_FLAGS:
            self.assertIn(flag, entries[0]["args"])
        self.assert_cleaned(entries)

    def test_list_mode_does_not_require_a_compiler(self):
        result, entries = self.run_runner(environment={"PATH": "/missing/compiler/path"}, list_only=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(result.stdout.strip(), "controlled compiler probe")
        self.assertEqual(entries, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
