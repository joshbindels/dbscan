#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "dbscan.hpp"

#ifndef BENCHMARK_DATA_DIR
#define BENCHMARK_DATA_DIR "test/test_data/benchmark"
#endif

namespace
{

constexpr double kEps = 0.5;
constexpr size_t kMinSamples = 10;

struct BenchmarkResult
{
    std::string filename;
    size_t point_count;
    double latency_ms;
};

std::vector<dbscan::Point> read_xyz(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("could not open " + path.string());
    }

    std::vector<dbscan::Point> points;

    dbscan::Point point;
    while (file >> point.x >> point.y >> point.z)
    {
        points.push_back(point);
    }

    if (!file.eof())
    {
        throw std::runtime_error("could not parse " + path.string());
    }

    return points;
}

} // namespace

int main()
{
    namespace fs = std::filesystem;

    const fs::path data_dir(BENCHMARK_DATA_DIR);

    if (!fs::is_directory(data_dir))
    {
        std::cerr << "Error: benchmark directory not found: "
                  << data_dir << "\n";
        return 1;
    }

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(data_dir))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".xyz")
        {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());

    if (files.empty())
    {
        std::cerr << "Error: no .xyz benchmark files found in "
                  << data_dir << "\n";
        return 1;
    }

    std::vector<BenchmarkResult> results;

    for (const auto& file : files)
    {
        const auto points = read_xyz(file);

        const auto start = std::chrono::steady_clock::now();
        const auto labels = dbscan::dbscan(points, kEps, kMinSamples);
        const auto end = std::chrono::steady_clock::now();

        (void)labels;

        const double latency_ms =
            std::chrono::duration<double, std::milli>(end - start).count();

        results.push_back({file.filename().string(), points.size(), latency_ms});
    }

    std::sort(results.begin(), results.end(),
              [](const BenchmarkResult& a, const BenchmarkResult& b)
              {
                  if (a.point_count != b.point_count)
                  {
                      return a.point_count < b.point_count;
                  }
                  return a.filename < b.filename;
              });

    std::cout << "DBSCAN performance (eps=" << kEps
              << ", min_samples=" << kMinSamples << ")\n\n";

    std::cout << std::left << std::setw(20) << "File"
              << std::right << std::setw(10) << "Points"
              << std::setw(18) << "Latency (ms)" << '\n';
    std::cout << std::string(48, '-') << '\n';

    for (const auto& result : results)
    {
        std::cout << std::left << std::setw(20) << result.filename
                  << std::right << std::setw(10) << result.point_count
                  << std::setw(18) << std::fixed << std::setprecision(3)
                  << result.latency_ms << '\n';
    }

    return 0;
}
