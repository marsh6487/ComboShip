#!/usr/bin/env python3
"""Preserve recorded appearance proof and verify the native usable-fire timing."""
import gzip
import json
import math
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "combo/tests/fixtures/dins_fire_native_visual_golden.json.gz"


def add(a, b):
    return [x + y for x, y in zip(a, b)]


def sub(a, b):
    return [x - y for x, y in zip(a, b)]


def scale(a, b):
    return [x * b for x in a]


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def smooth(value):
    x = max(0, min(1, value))
    return x*x*(3-2*x)


def expected(frame, view):
    """Rotate recorded billboards/widths and evaluate recorded shared normals.

    Native billboards face the real camera for every view. The preview canonical
    view used XY-facing sprites; the two reviewed views already corrected them.
    Ribbon geometry, UV motion, colors and opacity are from the accepted record.
    """
    az, el = [(0, 11), (18, 16), (-55, 29)][view]
    az, el = math.radians(az), math.radians(el)
    right = [math.cos(az), 0, -math.sin(az)]
    up = [-math.sin(az)*math.sin(el), math.cos(el), -math.cos(az)*math.sin(el)]
    forward = cross(right, up)
    result = json.loads(json.dumps(frame["mesh"]))
    for index, q in enumerate(result):
        points = q["p"]
        if q["spark"] in (1, 3, 4) and max(p[2] for p in points)-min(p[2] for p in points) < .001:
            center = [sum(p[n] for p in points)/4 for n in range(3)]
            q["p"] = [add(center, add(scale(right, p[0]-center[0]), scale(up, p[1]-center[1]))) for p in points]
        if q["spark"] == 2:
            q["alpha"] = [int(a*smooth((abs(dot(n, forward))-.035)/.30))
                          for a, n in zip(q["alphaFree"], q["normals"])]
            row = (0, 1, 2, 0, 1, 2)[q["band"]]
            low, high = [(0, 343/1024), (343/1024, 668/1024), (668/1024, 1)][row]
            q["t"] = (q["t"]/64-low)/(high-low)*64
            q["tSpan"] = q["tSpan"]/(high-low)
            fire_frame = frame["frame"]-21
            q["burnFrame"] = fire_frame if fire_frame >= 41 else 0
            if q["burnFrame"]:
                q["s"] = ((index-4) % 96 // 3) / 32 * 64
    trails = [q for q in result if q["spark"] == 5]
    for start in range(0, len(trails), 24):
        group = trails[start:start+24]
        centers = [scale(add(q["p"][0], q["p"][3]), .5) for q in group]
        centers.append(scale(add(group[-1]["p"][1], group[-1]["p"][2]), .5))
        widths = [math.sqrt(dot(sub(q["p"][3], q["p"][0]), sub(q["p"][3], q["p"][0])))*.5 for q in group]
        delta = sub(group[-1]["p"][2], group[-1]["p"][1])
        widths.append(math.sqrt(dot(delta, delta))*.5)
        edges = []
        for k, point in enumerate(centers):
            side = cross(forward, sub(centers[min(24, k+1)], centers[max(0, k-1)]))
            side = scale(side, widths[k]/max(.0001, math.sqrt(dot(side, side))))
            edges.append((sub(point, side), add(point, side)))
        for k, q in enumerate(group):
            q["p"] = [edges[k][0], edges[k+1][0], edges[k+1][1], edges[k][1]]
    return result


def main():
    fixture = json.loads(gzip.decompress(FIXTURE.read_bytes()))
    frames = fixture["frames"]
    with tempfile.TemporaryDirectory(prefix="dins-native-geometry-") as temp:
        temp = Path(temp)
        cases = ["static const NativeVisualCase cases[] = {"]
        for f in frames:
            tick = f["frame"]-21 if f["frame"] >= 21 else -1
            cases.append("{%d, %.8ff, {%s}}," % (tick, f["time"], ",".join("%.8ff" % x for x in f["hand"])))
        cases.append("};")
        (temp / "native_visual_cases.inc").write_text("\n".join(cases))
        binary = temp / "geometry"
        subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu2x", "-O1", "-I"+str(ROOT),
                        "-I"+str(temp), str(ROOT / "combo/tests/dins_fire_native_geometry_test.c"), "-lm", "-o", str(binary)], check=True)
        run = subprocess.run([str(binary)], capture_output=True, text=True)
        if run.returncode:
            raise AssertionError(run.stderr.strip())
        output = [json.loads(line) for line in run.stdout.splitlines()]
        assert len(output) == len(frames)*3
        max_position_error = max_uv_error = max_alpha_error = 0
        compared = 0
        for sample in output:
            reference = expected(frames[sample["sample"]], sample["view"])
            label = f'frame={frames[sample["sample"]]["frame"]} view={sample["view"]}'
            assert len(sample["mesh"]) == len(reference), (label, len(sample["mesh"]), len(reference))
            for index, (actual, golden) in enumerate(zip(sample["mesh"], reference)):
                where = f"{label} quad={index}"
                for a, b in zip(actual["p"], golden["p"]):
                    error = max(abs(x-y) for x, y in zip(a, b))
                    assert error < .001, (where, "position", error, a, b)
                    max_position_error = max(max_position_error, error)
                for key in ("s", "t", "sSpan", "tSpan"):
                    error = abs(actual[key]-golden[key])
                    assert error < .0001, (where, key, actual[key], golden[key])
                    max_uv_error = max(max_uv_error, error)
                for key in ("spark", "primary", "secondary"):
                    assert actual[key] == golden[key], (where, key, actual[key], golden[key])
                error = max(abs(a-b) for a, b in zip(actual["alpha"], golden["alpha"]))
                assert error <= 1, (where, "alpha", actual["alpha"], golden["alpha"])
                max_alpha_error = max(max_alpha_error, error)
                if golden["spark"] == 2:
                    assert actual["band"] == golden["band"] and actual["burnFrame"] == golden["burnFrame"], where
                    assert abs(actual["burn"]-golden["burn"]) < .00001, (where, "burn")
                compared += 1
        print(f"PASS preserved accepted 40/12 recipe: {len(frames)} samples x 3 camera bases, {compared} quads; "
              f"max position error={max_position_error:.6g}, UV error={max_uv_error:.6g}, alpha error={max_alpha_error:.6g}")
        print("Reference SHA256 " + fixture["sourceTimelineSha256"])
        linger = temp / "linger"
        subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu2x", "-O1", "-I"+str(ROOT),
                        str(ROOT / "combo/tests/dins_fire_native_linger_test.c"), "-lm", "-o", str(linger)], check=True)
        subprocess.run([str(linger)], check=True)


if __name__ == "__main__":
    main()
