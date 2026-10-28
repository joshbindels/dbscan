# Vehicle tracker

A client-server application that allows a client to send xyz files to a server over HTTP and receive noise/clusters in return (using DBScan).

# Dependencies
The C++ service and client have 2 dependencies that are vendored in `src/libs`
* cpp-httplib
* nlohmann json

And 2 dependencies that are downloaded at build time
* GTest
* Prometheus-cpp

Python dependencies are listed in `src/scripts/requirements.txt`

# Build 

```bash
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

# Run

## service
The service runs on `localhost:8080` it also exposes prometheus metrics at `localhost:9091/metrics`
```bash
./build/service
```

## client

The client sends a POST request to `localhost:8080/dbscan`. It sends the content of the input file to the service along with the `eps` and `min_samples` values and writes the service output to the `output-dir`

```bash
./build/processPointcloud --input <input.xyz> --output-dir <out_dir> --eps <eps> --min_samples <min_samples> <host:port>
```
- input: A 3 column `.xyz` file containing space separated x y and z coordinates of points.
- output-dir: Output is written to this directory as `.xyz` files. One file per cluster and a separate file for noise (`noise.xyz`, `cluster_1.xyz`, `cluster_N.xyz`)
- eps: The epsilon value used for DBScan (range of the neighbourhood).
- min_samples: The minimum number of samples required to start a cluster (core points).
- host:port: The host and port of the service (`localhost:8080`)

# HTTP API

The service exposes a POST endpoint at `/dbscan` it expects an `application/json` message with a body in the format:

```json
{
    "eps": 0.5,
    "min_samples": 10,
    "points": "a b c\nd e f..."
}
```

The service responds with an `application/json` message in the format:
```json
{
  "noise": "x y z\n...",
  "clusters": {
    "1": "x y z\n...",
    "2": "x y z\n...",
    ...
  }
}
```

# Testing & Peformance
This package contains 3 types of tests:
1. Unit tests to test specific DBSCAN edge cases
2. Benchmark to check DBSCAN results on various input files
3. Performance tests for DBSCAN in solation as well as end-to-end tests of the service.

## Unit tests and benchmark
**NOTE: Must build in release mode -DCMAKE_BUILD_TYPE=Release for performance tests to pass**

### unit test and benchmark correctness
```bash
ctest --test-dir build -V --output-on-failure
```

### performance test
Checks the performance of the DBSCAN function. Running on various xyz input file sizes (1500-8500) points and measuring the latency. The target is to handle 5000 points within 100ms.

```bash
./build/test/test_dbscan_performance
```

Results
```bash
DBSCAN performance (eps=0.5, min_samples=10)

File                    Points      Latency (ms)
------------------------------------------------
p1500.xyz                 1500            11.021
p2500.xyz                 2500            21.486
p3500.xyz                 3500            28.031
p4500.xyz                 4500            35.653
p5500.xyz                 5500            48.563
p6500.xyz                 6500            70.033
p7500.xyz                 7500            91.316
p8500.xyz                 8500           119.145
```

### end-to-end performance test
Checks the end-to-end performance, the time it takes for a client to receive a response. This includes the DBSCAN latency as well as any additional overhead (network etc.). The target is handling 5000 points within 100ms. This test runs a 5000 point file 10x and ensures all of the runs were under 100ms.

```bash
python src/test/test_end_to_end_performance.py
```

Results
```bash
Run 1: 62.09 ms
Run 2: 49.81 ms
Run 3: 50.15 ms
Run 4: 52.72 ms
Run 5: 49.89 ms
Run 6: 49.67 ms
Run 7: 49.90 ms
Run 8: 49.54 ms
Run 9: 51.18 ms
Run 10: 50.38 ms
PASS
```

# Known limitations

