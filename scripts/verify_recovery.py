"""Force-close the desktop with disposable autosaves, then recover on relaunch."""

import argparse
import json
import os
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory
import time
from zipfile import ZipFile


def check_recovery(app, fixture, evidence, fallback):
    with TemporaryDirectory(dir=evidence) as directory:
        profile = Path(directory)
        (profile / ".zahncad-recovery-test").touch()
        env = {**os.environ, "QT_FORCE_STDERR_LOGGING": "1"}
        with subprocess.Popen([app, "--recovery-smoke-write", profile, fixture], env=env) as writer:
            deadline = time.monotonic() + 45
            try:
                while not (profile / "ready").is_file():
                    if writer.poll() is not None:
                        raise RuntimeError(f"Autosave process exited with {writer.returncode}")
                    if time.monotonic() >= deadline:
                        raise TimeoutError("Timed out waiting for two autosaves")
                    time.sleep(0.1)
                if writer.poll() is not None:
                    raise RuntimeError("Autosave process exited before it could be force-closed")
            finally:
                if writer.poll() is None:
                    writer.kill()
                writer.wait(timeout=15)

        if fallback:
            (profile / "recovery.zahncad").write_bytes(b"intentionally corrupt")
        case = evidence / ("recovered-previous.zahncad" if fallback else "recovered-latest.zahncad")
        subprocess.run([app, "--recovery-smoke-read", profile, case], env=env, check=True, timeout=45)
        with ZipFile(case) as saved:
            manifest = json.loads(saved.read("manifest.json"))
            scan, = manifest["scans"]
            expected_id = "Recovery first" if fallback else "Recovery second"
            if (manifest["caseId"], scan["visible"], scan["opacity"], scan["role"]) != (
                expected_id, fallback, 1 if fallback else 0.35, "Reference"
            ) or saved.read(scan["source"]) != fixture.read_bytes():
                raise RuntimeError(f"Unexpected recovered case: {case.name}")
        print(f"Recovered and saved {case.name} after forcing the autosaving process closed")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("desktop", type=Path)
    parser.add_argument("evidence", type=Path)
    args = parser.parse_args()
    fixture = Path(__file__).resolve().parents[1] / "examples/triangle.stl"
    evidence = args.evidence.resolve()
    evidence.mkdir(parents=True, exist_ok=True)
    for fallback in (False, True):
        check_recovery(args.desktop.resolve(), fixture, evidence, fallback)


if __name__ == "__main__":
    main()
