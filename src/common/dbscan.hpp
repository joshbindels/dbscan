#include <vector>
#include <cmath>
#include <algorithm>
#include <iterator>

// based on https://en.wikipedia.org/wiki/DBSCAN#Original_query-based_algorithm

namespace dbscan
{

struct Point
{
	double x;
	double y;
	double z;
};

constexpr int UNDEFINED = 0;
constexpr int NOISE = -1;

namespace
{

std::vector<int> range_query(const Point& p, const std::vector<Point>& points, double eps)
{
    std::vector<int> neighbors;

    auto calculate_distance = [](const Point& a, const Point& b) -> double
    {
        return std::hypot(b.x - a.x, b.y - a.y, b.z - a.z);
    };

    for (size_t i = 0; i < points.size(); ++i)
    {
        if (calculate_distance(p, points[i]) <= eps)
        {
            neighbors.push_back(i);
        }
    }
    return neighbors;
}

std::vector<int> set_union(const std::vector<int>& a, const std::vector<int>& b)
{
    std::vector<int> res;
    std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(res));
    return res;
}

} // namespace

std::vector<int> dbscan(const std::vector<Point>& points, double eps, size_t min_pts)
{
    std::vector<int> labels(points.size(), UNDEFINED);

    size_t cluster_id = 0;

    for (size_t p = 0; p < points.size(); ++p)
    {
        if (labels[p] != UNDEFINED) { continue; }

        std::vector<int> neighbors = range_query(points[p], points, eps);
        if (neighbors.size() < min_pts)
        {
            labels[p] = NOISE;
            continue;
        }

        cluster_id += 1;
        labels[p] = cluster_id;
        while (!neighbors.empty())
        {
            int q = neighbors.back();
            neighbors.pop_back();
            if (labels[q] == NOISE) { labels[q] = cluster_id; }
            if (labels[q] != UNDEFINED) { continue; }
            labels[q] = cluster_id;
            std::vector<int> non = range_query(points[q], points, eps);
            if (non.size() >= min_pts)
            {
                neighbors = set_union(neighbors, non);
            }
        }
    }
    return labels;
}

} // namespace dbscan
