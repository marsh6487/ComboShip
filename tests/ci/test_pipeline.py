"""Exercise CI command failures and final verdicts without building the games."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "scripts/ci/run_regressions.py"
VERDICT = ROOT / "scripts/ci/check_required_jobs.py"


class CommandRunnerTests(unittest.TestCase):
    def run_suite(self, commands, suite="items", extra_args=()):
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            manifest = directory / "commands.json"
            manifest.write_text(json.dumps(commands))
            summary = directory / "summary.md"
            env = dict(os.environ, GITHUB_STEP_SUMMARY=str(summary), CI_TEST_VALUE="inherited",
                       CPATH="original", COMBO_NEI_FIXTURE_CPATH="pinned-fixtures")
            result = subprocess.run(
                [sys.executable, str(RUNNER), "--manifest", str(manifest), "--suite", suite, *extra_args],
                env=env, text=True, capture_output=True,
            )
            return result, summary.read_text() if summary.exists() else ""

    def test_runs_selected_group_with_inherited_and_explicit_environment(self):
        result, summary = self.run_suite([
            {"suite": "items", "name": "environment", "run": 'test "$CI_TEST_VALUE" = inherited; test "$LOCAL_VALUE" = preserved', "env": {"LOCAL_VALUE": "preserved"}},
            {"suite": "world", "name": "unselected failure", "run": "exit 7"},
        ])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("environment", summary)
        self.assertNotIn("unselected failure", summary)
        self.assertIn("success", summary)

    def test_stops_on_first_failed_command_and_records_failure(self):
        result, summary = self.run_suite([
            {"suite": "items", "name": "failure", "run": "exit 17"},
            {"suite": "items", "name": "must not run", "run": "echo SHOULD_NOT_RUN"},
        ])
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertNotIn("SHOULD_NOT_RUN", result.stdout)
        self.assertIn("failure", summary)
        self.assertIn("17", summary)

    def test_pipeline_and_multiline_errors_cannot_be_hidden(self):
        for command in ("false | cat", "false\necho SHOULD_NOT_RUN"):
            with self.subTest(command=command):
                result, _ = self.run_suite([{"suite": "items", "name": "strict shell", "run": command}])
                self.assertEqual(result.returncode, 1, result.stderr)
                self.assertNotIn("SHOULD_NOT_RUN", result.stdout)

    def test_unknown_group_is_an_error(self):
        result, _ = self.run_suite([{"suite": "world", "name": "other", "run": "true"}])
        self.assertEqual(result.returncode, 2)
        self.assertIn("Unknown or empty", result.stderr)

    def test_pinned_headers_keep_their_original_nei_only_scope(self):
        result, _ = self.run_suite([
            {"suite": "items", "origin": "gate", "name": "original scope", "run": 'test "$CPATH" = original'},
            {"suite": "items", "origin": "nei", "name": "NEI scope", "run": 'test "$CPATH" = pinned-fixtures'},
        ])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_legacy_entrypoint_selects_all_nei_groups_in_original_order(self):
        result, summary = self.run_suite([
            {"suite": "world", "origin": "nei", "name": "first", "run": "true"},
            {"suite": "items", "origin": "gate", "name": "outer gate", "run": "exit 17"},
            {"suite": "equipment", "origin": "nei", "name": "second", "run": "true"},
        ], suite="all", extra_args=("--source", "nei"))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertLess(summary.index("first"), summary.index("second"))
        self.assertNotIn("outer gate", summary)


class FinalVerdictTests(unittest.TestCase):
    @staticmethod
    def jobs(code="true"):
        return {
            "preflight": {"result": "success", "outputs": {"code": code}},
            **{name: {"result": "success" if code == "true" else "skipped"}
               for name in ("regressions", "windows", "linux")},
        }

    def verdict(self, jobs):
        return subprocess.run(
            [sys.executable, str(VERDICT)],
            env=dict(os.environ, NEEDS_JSON=json.dumps(jobs)), text=True, capture_output=True,
        )

    def test_all_required_results_must_succeed(self):
        self.assertEqual(self.verdict(self.jobs()).returncode, 0)
        for name in ("preflight", "regressions", "windows", "linux"):
            for status in ("failure", "cancelled", "skipped", "", "pending"):
                with self.subTest(name=name, status=status):
                    jobs = self.jobs()
                    jobs[name]["result"] = status
                    self.assertNotEqual(self.verdict(jobs).returncode, 0)

    def test_docs_only_allows_intentional_skips(self):
        self.assertEqual(self.verdict(self.jobs("false")).returncode, 0)
        jobs = self.jobs("false")
        jobs["linux"]["result"] = "failure"
        self.assertNotEqual(self.verdict(jobs).returncode, 0)

    def test_missing_routing_output_or_job_is_a_failure(self):
        for missing in ("preflight", "regressions", "windows", "linux"):
            jobs = self.jobs()
            del jobs[missing]
            self.assertNotEqual(self.verdict(jobs).returncode, 0)
        for code in ("", "unknown", None, True):
            self.assertNotEqual(self.verdict(self.jobs(code)).returncode, 0)


if __name__ == "__main__":
    unittest.main()
