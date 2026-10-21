#!/usr/bin/env python3
"""Fail the stable gate unless every expected job has the expected result."""

import json
import os
import sys


def main():
    try:
        needs = json.loads(os.environ["NEEDS_JSON"])
        code = needs["preflight"]["outputs"]["code"]
        if code not in ("true", "false"):
            raise ValueError("Missing or invalid preflight code output")
        expected = {"preflight": "success"}
        expected.update({name: "success" if code == "true" else "skipped"
                         for name in ("regressions", "windows", "linux")})
        failed = False
        rows = []
        for name, required in expected.items():
            actual = needs[name]["result"]
            rows.append(f"| {name} | {actual} | {required} |")
            if actual != required:
                print(f"::error::{name}: expected {required}, got {actual or 'missing result'}")
                failed = True
        report = "## Final build gate\n\n| Job | Result | Required |\n|---|---|---|\n" + "\n".join(rows) + "\n"
        print(report)
        if os.environ.get("GITHUB_STEP_SUMMARY"):
            with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as summary:
                summary.write(report)
        return 1 if failed else 0
    except (KeyError, ValueError, TypeError) as error:
        print(f"::error::Cannot verify required CI jobs: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
