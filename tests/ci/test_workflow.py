"""Pin independent completion and required coverage in the actual Actions graph."""

import json
from pathlib import Path
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[2]


class WorkflowTests(unittest.TestCase):
    def test_platforms_and_every_test_group_finish_independently(self):
        workflow = yaml.safe_load((ROOT / ".github/workflows/build-artifacts.yml").read_text())
        checks = workflow["jobs"]["checks"]
        self.assertIs(checks["strategy"]["fail-fast"], False)
        self.assertNotIn("continue-on-error", checks)
        entries = checks["strategy"]["matrix"]["include"]
        kinds = [entry["check"] for entry in entries]
        groups = {task["suite"] for task in json.loads((ROOT / "scripts/ci/regressions.json").read_text())}
        self.assertCountEqual(kinds, ["windows", "linux", *groups])
        self.assertTrue(all(entry.get("label") for entry in entries))
        self.assertEqual(checks["uses"], "./.github/workflows/ci-check.yml")
        self.assertEqual(checks["with"]["check"], "${{ matrix.check }}")
        self.assertCountEqual(workflow["jobs"]["gate"]["needs"], ["preflight", "checks"])

    def test_component_dispatch_preserves_builds_and_single_regression_group(self):
        dispatch = yaml.safe_load((ROOT / ".github/workflows/ci-check.yml").read_text())["jobs"]
        self.assertEqual(dispatch["windows"]["uses"], "./.github/workflows/build.yml")
        self.assertEqual(dispatch["linux"]["uses"], "./.github/workflows/build-linux.yml")
        self.assertEqual(dispatch["regression"]["uses"], "./.github/workflows/regressions.yml")
        self.assertEqual(dispatch["regression"]["with"]["suite"], "${{ inputs.check }}")
        regression = yaml.safe_load((ROOT / ".github/workflows/regressions.yml").read_text())["jobs"]["regression"]
        self.assertNotIn("strategy", regression)


if __name__ == "__main__":
    unittest.main()
