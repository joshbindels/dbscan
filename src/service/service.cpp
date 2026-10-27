#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <chrono>
#include <stdio.h>

#include "dbscan.hpp"
#include "httplib.h"
#include "json.hpp"
#include "metrics.hpp"

bool read_points_from_request(const nlohmann::json& data, std::vector<dbscan::Point>& points)
{
    if (!data.is_object() || !data.contains("points") ||
        !data.at("points").is_string())
    {
        std::cerr << "Error: request does not contain a points string\n";
        return false;
    }

    std::istringstream stream(data.at("points").get<std::string>());

    double x;
    double y;
    double z;

    while (stream >> x >> y >> z)
    {
        dbscan::Point point;
        point.x = x;
        point.y = y;
        point.z = z;
        points.push_back(std::move(point));
    }

    return true;
}

std::string points_to_xyz(const std::vector<dbscan::Point>& points)
{
    std::ostringstream output;

    for (const auto& point : points)
    {
        output << point.x << ' ' << point.y << ' ' << point.z << '\n';
    }

    return output.str();
}

class ScopedRequestMetrics
{
public:
    ScopedRequestMetrics(metrics::Metrics& metrics, const httplib::Response& response)
        : m_metrics(metrics)
        , m_response(response)
        , m_start(std::chrono::steady_clock::now())
    {
    }

    ~ScopedRequestMetrics()
    {
        const double elapsed_ms =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - m_start)
                .count();

        m_metrics.record_request(m_response.status);
        m_metrics.record_request_duration_ms(elapsed_ms);
    }

private:
    metrics::Metrics& m_metrics;
    const httplib::Response& m_response;
    std::chrono::steady_clock::time_point m_start;
};

void handle_dbscan_request(const httplib::Request& req,
                           httplib::Response& res,
                           metrics::Metrics& metrics)
{
    res.status = 500;
    ScopedRequestMetrics request_metrics(metrics, res);

    nlohmann::json data;
    double eps;
    size_t min_pts;
    try
    {
        data = nlohmann::json::parse(req.body);
        eps = data["eps"];
        min_pts = data["min_samples"];
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: request format: " << error.what() << "\n";
        res.status = 400;
        res.set_content("invalid request", "text/plain");
        return;
    }

    std::vector<dbscan::Point> points;
    if (!read_points_from_request(data, points))
    {
        res.status = 400;
        res.set_content("invalid points", "text/plain");
        return;
    }
    metrics.record_request_points(points.size());

    const auto dbscan_start = std::chrono::steady_clock::now();
    std::vector<int> labels = dbscan::dbscan(points, eps, min_pts);
    metrics.record_dbscan_duration_ms(
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - dbscan_start)
            .count());

    std::map<int, std::vector<dbscan::Point>> clusters;
    std::vector<dbscan::Point> noise_points;

    for (size_t i = 0; i < points.size(); ++i)
    {
        if (labels[i] == dbscan::NOISE)
        {
            noise_points.push_back(points[i]);
        }
        else if (labels[i] != dbscan::UNDEFINED)
        {
            clusters[labels[i]].push_back(points[i]);
        }
    }

    metrics.record_dbscan_clusters(clusters.size());
    metrics.record_dbscan_noise_ratio(
        points.empty() ? 0.0
                       : static_cast<double>(noise_points.size()) /
                             static_cast<double>(points.size()));

    nlohmann::json resp_json = nlohmann::json();
    resp_json["noise"] = points_to_xyz(noise_points);
    nlohmann::json clusters_json = nlohmann::json::object();
    for (const auto& [cluster_id, cluster_points] : clusters)
    {
        clusters_json[std::to_string(cluster_id)] = points_to_xyz(cluster_points);
    }
    resp_json["clusters"] = clusters_json;
    res.status = 200;
    res.set_content(resp_json.dump(), "application/json");
}

int main(int argc, char** argv)
{
    metrics::Metrics metrics;

    httplib::Server srv;

    srv.Post("/dbscan", [&metrics](const httplib::Request& req, httplib::Response& res) -> void
    {
        handle_dbscan_request(req, res, metrics);
    });

    srv.listen("0.0.0.0", 8080);
    return 0;
}
