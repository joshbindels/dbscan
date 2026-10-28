#!/usr/bin/env python3

import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
CLIENT = REPO_ROOT / "build" / "processPointcloud"
SOURCE = REPO_ROOT / "src" / "test" / "test_data" / "benchmark" / "p5500.xyz"

POINT_COUNT = 5000
REQUEST_COUNT = 10
HOST_PORT = "127.0.0.1:8080"
EPS = "0.5"
MIN_SAMPLES = "10"
MAX_LATENCY_MS = 100.0


def main() -> int:
    with tempfile.TemporaryDirectory(prefix="vehicle_tracker_perf_") as temp_dir:
        temp_path = Path(temp_dir)
        input_file = temp_path / "p5000.xyz"
        output_dir = temp_path / "output"

        with SOURCE.open() as source, input_file.open("w") as destination:
            for _ in range(POINT_COUNT):
                line = source.readline()
                if not line:
                    print(f"Error: {SOURCE} does not contain enough points", file=sys.stderr)
                    return 1
                destination.write(line)

        all_passed = True

        for run in range(1, REQUEST_COUNT + 1):
            start = time.perf_counter()
            result = subprocess.run(
                [
                    str(CLIENT),
                    "--input", str(input_file),
                    "--output-dir", str(output_dir),
                    "--eps", EPS,
                    "--min_samples", MIN_SAMPLES,
                    HOST_PORT,
                ],
                capture_output=True,
                text=True,
            )
            elapsed_ms = (time.perf_counter() - start) * 1000.0

            if result.returncode != 0:
                print(result.stderr, file=sys.stderr)
                return 1

            print(f"Run {run}: {elapsed_ms:.2f} ms")

            if elapsed_ms > MAX_LATENCY_MS:
                all_passed = False

        if not all_passed:
            print(f"FAIL: one or more requests exceeded {MAX_LATENCY_MS:.0f} ms",
                  file=sys.stderr)
            return 1

        print("PASS")
        return 0


if __name__ == "__main__":
    sys.exit(main())
