#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>
#include <map>
#include <fstream>
#include <stdexcept>
#include <string>
#include "dbscan.hpp"

#ifndef BENCHMARK_DATA_DIR
#define BENCHMARK_DATA_DIR "test/test_data/benchmark"
#endif

namespace
{

std::map<int, std::set<size_t>> get_clusters(const std::vector<dbscan::Point>& points, double eps, size_t min_pts)
{
    std::vector<int> labels = dbscan::dbscan(points, eps, min_pts);
    std::map<int, std::set<size_t>> clusters;
    for (size_t i = 0; i < labels.size(); ++i)
    {
        if (labels[i] != -1)
        {
            clusters[labels[i]].insert(i);
        }
    }
    return clusters;
}

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

    return points;
}

std::vector<int> read_labels(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("could not open " + path.string());
    }

    std::vector<int> labels;
    int label = 0;
    while (file >> label)
    {
        labels.push_back(label);
    }

    return labels;
}

std::vector<std::filesystem::path> benchmark_files()
{
    namespace fs = std::filesystem;

    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(BENCHMARK_DATA_DIR))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".xyz")
        {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

void check_benchmark_dataset(const std::filesystem::path& path)
{
    const auto points = read_xyz(path);

    auto label_path = path;
    label_path.replace_extension(".lbl");
    const auto ground_truth = read_labels(label_path);

    ASSERT_EQ(points.size(), ground_truth.size());

    constexpr double eps = 0.5;
    constexpr size_t min_pts = 10;
    const auto labels = dbscan::dbscan(points, eps, min_pts);

    std::set<int> predicted_clusters;
    std::set<int> ground_truth_clusters;
    std::map<int, std::map<int, size_t>> overlaps;

    for (size_t i = 0; i < labels.size(); ++i)
    {
        if (labels[i] != dbscan::NOISE)
        {
            predicted_clusters.insert(labels[i]);
        }
        if (ground_truth[i] != -1)
        {
            ground_truth_clusters.insert(ground_truth[i]);
        }
        if (labels[i] != dbscan::NOISE && ground_truth[i] != -1)
        {
            overlaps[labels[i]][ground_truth[i]]++;
        }
    }

    EXPECT_EQ(predicted_clusters.size(), ground_truth_clusters.size());

    // Predicted cluster IDs are arbitrary, so map each one to the ground-truth
    // label that most of its points have.
    std::map<int, int> predicted_to_truth;
    for (const auto& [predicted_id, counts] : overlaps)
    {
        int best_label = -1;
        size_t best_count = 0;
        for (const auto& [truth_id, count] : counts)
        {
            if (count > best_count)
            {
                best_label = truth_id;
                best_count = count;
            }
        }
        predicted_to_truth[predicted_id] = best_label;
    }

    std::set<int> mapped_truth_clusters;
    for (const auto& [predicted_id, truth_id] : predicted_to_truth)
    {
        mapped_truth_clusters.insert(truth_id);
    }
    EXPECT_EQ(mapped_truth_clusters.size(), ground_truth_clusters.size());

    size_t wrong_cluster = 0;
    size_t cluster_points_lost = 0;
    size_t noise_points_absorbed = 0;

    for (size_t i = 0; i < labels.size(); ++i)
    {
        if (ground_truth[i] == -1)
        {
            if (labels[i] != dbscan::NOISE)
            {
                noise_points_absorbed++;
            }
            continue;
        }

        if (labels[i] == dbscan::NOISE)
        {
            cluster_points_lost++;
        }
        else if (predicted_to_truth[labels[i]] != ground_truth[i])
        {
            wrong_cluster++;
        }
    }

    EXPECT_EQ(cluster_points_lost, 0);
    EXPECT_EQ(wrong_cluster, 0);

    // DBSCAN may absorb a few border-line noise points into a cluster.
    EXPECT_LE(noise_points_absorbed, points.size() / 100);
}

} // namespace

// All points are farther apart than eps and fewer than min_pts can be
// neighbors, so the algorithm should classify every point as noise.
TEST(DBScanTest, AllNoise)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {10.0, 0.0, 0.0},
        {0.0, 10.0, 0.0}
    };
    std::vector<int> labels = dbscan::dbscan(points, 1.0, 3);
    for (int label : labels)
    {
        EXPECT_EQ(label, -1);
    }
}

// Three mutually close points with min_pts=2 should form exactly one
// cluster containing all three points.
TEST(DBScanTest, SingleCluster)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {0.1, 0.0, 0.0},
        {0.0, 0.1, 0.0}
    };
    auto clusters = get_clusters(points, 1.0, 2);
    EXPECT_EQ(clusters.size(), 1);
    EXPECT_EQ(clusters.begin()->second.size(), 3);
}

// Two tight groups separated by a large gap should produce two independent
// clusters rather than being merged into one.
TEST(DBScanTest, TwoSeparateClusters)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {0.1, 0.0, 0.0},
        {10.0, 0.0, 0.0},
        {10.1, 0.0, 0.0}
    };
    auto clusters = get_clusters(points, 0.5, 2);
    EXPECT_EQ(clusters.size(), 2);
    for (const auto& [cluster_id, point_set] : clusters)
    {
        EXPECT_EQ(point_set.size(), 2);
    }
}

// The three close points satisfy the density requirement and should share one
// cluster; the isolated point should remain noise.
TEST(DBScanTest, CoreAndBorderPoints)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {0.1, 0.0, 0.0},
        {0.0, 0.1, 0.0},
        {10.0, 10.0, 0.0}
    };
    std::vector<int> labels = dbscan::dbscan(points, 0.3, 3);
    EXPECT_NE(labels[0], -1);
    EXPECT_NE(labels[1], -1);
    EXPECT_NE(labels[2], -1);
    EXPECT_EQ(labels[0], labels[1]);
    EXPECT_EQ(labels[0], labels[2]);
    EXPECT_EQ(labels[3], -1);
}

// The middle point is within eps of both endpoints, so density should connect
// all three points into one cluster even though the endpoints are farther apart.
TEST(DBScanTest, ChainConnectivity)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {2.0, 0.0, 0.0}
    };
    std::vector<int> labels = dbscan::dbscan(points, 1.5, 2);
    EXPECT_NE(labels[0], -1);
    EXPECT_NE(labels[1], -1);
    EXPECT_NE(labels[2], -1);
    EXPECT_EQ(labels[0], labels[1]);
    EXPECT_EQ(labels[0], labels[2]);
}

// A point far from the dense cluster should remain noise and should not be
// absorbed into the nearby cluster's label.
TEST(DBScanTest, NoiseNotAbsorbed)
{
    std::vector<dbscan::Point> points = {
        {0.0, 0.0, 0.0},
        {0.1, 0.0, 0.0},
        {0.0, 0.1, 0.0},
        {5.0, 5.0, 0.0},
        {10.0, 10.0, 0.0}
    };
    std::vector<int> labels = dbscan::dbscan(points, 0.3, 3);
    EXPECT_EQ(labels[4], -1);
    EXPECT_NE(labels[0], labels[4]);
}

// Runs each generated benchmark dataset and compares the discovered partition
// against the ground-truth labels in the matching .lbl file.
TEST(DBScanBenchmark, MatchesGroundTruthLabels)
{
    const auto files = benchmark_files();
    ASSERT_FALSE(files.empty());

    for (const auto& path : files)
    {
        SCOPED_TRACE(path.filename().string());
        check_benchmark_dataset(path);
    }
}
