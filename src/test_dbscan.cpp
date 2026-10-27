#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <fstream>
#include "dbscan.hpp"

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

} // namespace

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

TEST(DBScanTest, NonIntegerCoordinates)
{
    std::vector<dbscan::Point> points = {
        {-0.5, 0.25, 1.75},
        {-0.4, 0.3, 1.8},
        {-0.45, 0.22, 1.77}
    };
    auto clusters = get_clusters(points, 0.2, 2);
    EXPECT_EQ(clusters.size(), 1);
    EXPECT_EQ(clusters.begin()->second.size(), 3);
}

TEST(DBScanTest, BenchmarkDatasetRegression)
{
    std::vector<dbscan::Point> points;
    std::ifstream file("/Users/josh/workspace/vehicle_tracker/test_data/benhcmark_hepta.dat");
    ASSERT_TRUE(file.is_open());
    size_t num_points;
    file >> num_points;
    file >> std::ws;
    double x, y, z;
    char comma;
    size_t cluster;
    while (file >> x >> comma >> y >> comma >> z >> comma >> cluster)
    {
        points.push_back({x, y, z});
    }
    file.close();

    std::vector<int> labels = dbscan::dbscan(points, 0.75, 4);

    int num_clusters = 0;
    for (int label : labels)
    {
        if (label > num_clusters)
        {
            num_clusters = label;
        }
    }

    int num_noise = 0;
    for (int label : labels)
    {
        if (label == -1)
        {
            num_noise++;
        }
    }

    EXPECT_EQ(num_clusters, 7);
    EXPECT_EQ(num_noise, 1);
}
