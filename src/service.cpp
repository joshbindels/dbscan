#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <stdio.h>

#include "dbscan.hpp"
#include "httplib.h"
#include "json.hpp"
#include "metrics.hpp"

void read_points_from_request(const nlohmann::json& data, std::vector<dbscan::Point>& points)
{
    if (!data.is_object() || !data.contains("points") ||
        !data.at("points").is_string())
    {
        std::cerr << "Error: request does not contain a points string\n";
        return;
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

int main(int argc, char** argv)
{
    metrics::Metrics metrics;

    httplib::Server srv;

    srv.Post("/dbscan", [&metrics](const httplib::Request& req, httplib::Response& res) -> void
    {
        metrics.record_request();

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
            return;
        }

        std::vector<dbscan::Point> points;
        std::cout << "points: " << points.size() << '\n';
        read_points_from_request(data, points);
        metrics.record_request_points(points.size());

        std::vector<int> labels = dbscan::dbscan(points, eps, min_pts);

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

        nlohmann::json resp_json = nlohmann::json();
        resp_json["noise"] = points_to_xyz(noise_points);
        nlohmann::json clusters_json = nlohmann::json::object();
        for (const auto& [cluster_id, cluster_points] : clusters)
        {
            clusters_json[std::to_string(cluster_id)] = points_to_xyz(cluster_points);
        }
        resp_json["clusters"] = clusters_json;
        res.set_content(resp_json.dump(), "application/json");
    });

    srv.listen("0.0.0.0", 8080);
    return 0;
}
