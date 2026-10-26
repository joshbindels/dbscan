#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <stdio.h>

#include "dbscan.h"
#include "httplib.h"
#include "json.hpp"

void read_benchmark(std::vector<Point>& points)
{
    const std::string filename = "/Users/josh/workspace/vehicle_tracker/test_data/benhcmark_hepta.dat";
	std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file " << filename << std::endl;
    }

    // first line is number of points
    size_t num_points;
    file >> num_points;
    file >> std::ws;

    double x;
    double y;
    double z;
    char comma;
    size_t cluster;

    while (file >> x >> comma >> y >> comma >> z >> comma >> cluster)
    {
        Point temp_p;
        temp_p.x = x;
        temp_p.y = x;
        temp_p.z = x;
        temp_p.clusterID = UNCLASSIFIED;
        points.push_back(std::move(temp_p));
    }

    std::cout << "Number of points: " << num_points << std::endl;

    file.close();
}

void print_results(std::vector<Point>& points, int num_points)
{
    int i=0;
    std::cout << "Number of points: " << num_points << std::endl;
    std::cout << "x\ty\tz\tcluster_id" << std::endl;
    return;
    while (i < num_points)
    {
        std::cout << points[i].x << "\t"
        << points[i].y << "\t"
        << points[i].z << "\t"
        << points[i].clusterID << std::endl;
        i++;
    }
}


int main(int argc, char** argv)
{
    httplib::Server srv;

    srv.Post("/dbscan", [](const httplib::Request& req, httplib::Response& res)
    {
        nlohmann::json data = nlohmann::json::parse(req.body);
        std::cout << "Got request with: " << std::endl
        << "eps: " << data["eps"] << std::endl
        << "min_samples: " << data["min_samples"] << std::endl;

        std::vector<Point> points;
        read_benchmark(points);
        DBSCAN ds(4, (0.75 * 0.75), points);
        ds.run();
        //print_results(ds.m_points, ds.getTotalPointSize());
        nlohmann::json resp_json = nlohmann::json();
        resp_json["total_points"] = ds.getTotalPointSize();
        res.set_content(resp_json.dump(), "application/json");
    });

    srv.listen("0.0.0.0", 8080);
    return 0;
}
