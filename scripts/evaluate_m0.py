"""Run the M0 synthetic baseline with a desktop build and a working display."""

import argparse
import csv
import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory


def run(*command):
    result = subprocess.run(command, capture_output=True, text=True, errors="replace",
                            timeout=180, env={**os.environ, "QT_FORCE_STDERR_LOGGING": "1"})
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)
    result.check_returncode()
    return result.stdout.strip()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def evaluate(build, evidence):
    root = Path(__file__).resolve().parents[1]
    suffix = ".exe" if os.name == "nt" else ""
    cli = build / f"zahncad-cli{suffix}"
    desktop = build / f"zahncad{suffix}"
    benchmark = build / f"viewer_benchmark{suffix}"
    for binary in (cli, desktop, benchmark):
        require(binary.is_file(), f"Missing build target: {binary}")

    fixtures = root / "examples"
    triangle = fixtures / "triangle.stl"
    colored = fixtures / "colored-scan.ply"
    brush = fixtures / "brush-test.ply"
    evidence.mkdir(parents=True)
    with TemporaryDirectory(dir=evidence) as temporary:
        scratch = Path(temporary)
        scaled = scratch / "triangle-scaled.stl"
        run(cli, "convert", triangle, scaled, "2.54")
        require(scaled.stat().st_size == 134, "Unexpected binary STL size")

        for fixture, extent in ((triangle, 10), (scaled, 25.4)):
            report = run(cli, "inspect", fixture)
            fields = dict(line.split(": ", 1) for line in report.splitlines())
            expected = {"Triangles": "1", "Degenerate": "0", "Boundary edges": "3",
                        "Non-manifold edges": "0", "Inconsistent edges": "0",
                        "Bounds (mm)": f"0,0,0 to {extent},{extent},0"}
            require(all(fields.get(key) == value for key, value in expected.items()),
                    f"Unexpected geometry report for {fixture.name}:\n{report}")
            print(f"{fixture.name}: {extent} x {extent} x 0 mm, one face, three boundary edges")

        stress = scratch / "curved-stress.stl"
        run(sys.executable, root / "scripts/generate_stress_stl.py", stress,
            "--columns", "2500", "--rows", "1000")
        for fixture, count in ((triangle, 1), (colored, 2), (brush, 16), (stress, 5_000_000)):
            report = run(benchmark, fixture)
            fields = dict(field.split("=", 1) for field in report.split())
            navigation = int(fields["navigation_triangles"])
            expected_lod = 0 < navigation < count if fixture == stress else navigation == count
            require(int(fields["triangles"]) == count and expected_lod,
                    f"Unexpected display counts for {fixture.name}: {report}")
            print(f"{fixture.name}: {report}")

        for fixture, mode in ((triangle, "--smoke-test"), (colored, "--stage2-smoke-test"),
                              (brush, "--stage2-smoke-test"), (stress, "--smoke-test")):
            screenshot = scratch / f"{fixture.stem}.png"
            run(desktop, mode, fixture, screenshot)
            artifacts = [screenshot]
            if mode == "--stage2-smoke-test":
                artifacts += [Path(f"{screenshot}.brush.png"), Path(f"{screenshot}.zahncad")]
            for artifact in artifacts:
                require(artifact.is_file() and artifact.stat().st_size > 0,
                        f"Missing evidence: {artifact.name}")
                artifact.replace(evidence / artifact.name)
            print(f"{fixture.name}: desktop smoke passed")

        screenshot = scratch / "reference.png"
        run(desktop, "--m0-reference-smoke-test", triangle, screenshot)
        report = Path(f"{screenshot}.csv")
        with report.open(encoding="utf-8", newline="") as stream:
            readings = list(csv.DictReader(stream))
        require([row["phase"] for row in readings] ==
                ["initial_session", "after_reopen", "after_reopen"], "Wrong reference phases")
        require([row["within_tolerance"] for row in readings] == ["yes", "yes", "no"],
                "Wrong reference tolerance results")
        measured = [float(row["measured_mm"]) for row in readings]
        for row, distance in zip(readings, measured):
            require(abs(float(row["error_mm"]) - (distance - 5)) < 1e-9 and
                    abs(float(row["repeat_spread_mm"]) - (max(measured) - min(measured))) < 1e-9 and
                    row["reviewer_notes"] == "'=Synthetic, \"control\"",
                    "Wrong reference CSV values or escaping")
        for artifact in (screenshot, report, Path(f"{screenshot}.zahncad")):
            require(artifact.is_file() and artifact.stat().st_size > 0,
                    f"Missing evidence: {artifact.name}")
            artifact.replace(evidence / artifact.name)
        print("Reference measurement: CSV and reopened case passed")

    print(f"M0 synthetic baseline passed. Evidence: {evidence}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path, help="Desktop build directory with tests enabled")
    parser.add_argument("evidence", type=Path, help="Directory for screenshots and edited cases")
    args = parser.parse_args()
    try:
        evaluate(args.build.resolve(), args.evidence.resolve())
    except (OSError, KeyError, ValueError, subprocess.SubprocessError) as error:
        sys.exit(f"M0 synthetic baseline failed: {error}")


if __name__ == "__main__":
    main()
