#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "client_argparser.h"
#include "httplib.h"
#include "json.hpp"

int main(int argc, char** argv)
{
    // Parse command line arguments
    argparse::Options options;

    try
    {
        options = argparse::parse_arguments(argc, argv);
    }
    catch (const std::runtime_error& error)
    {
        std::cerr << "Error: " << error.what() << "\n";
        argparse::print_usage(argv[0]);
        return 1;
    }

    if (options.show_help)
    {
        argparse::print_usage(argv[0]);
        return 0;
    }

    // Check if the input file is real
    std::ifstream input_file(options.input);
    if (!input_file.is_open())
    {
        std::cerr << "Error: could not open input file " << options.input << "\n";
        return 1;
    }

    // Try to create the output directory (No error if directory already exists)
    std::error_code ec;
    std::filesystem::create_directories(options.output_dir, ec);
    if (ec)
    {
        std::cerr << "Error: could not create output directory "
                  << options.output_dir << ": " << ec.message() << "\n";
        return 1;
    }

    // Send HTTP request to server
    httplib::Client client(options.host_port);

    nlohmann::json payload = nlohmann::json::object();
    payload["eps"] = options.eps;
    payload["min_samples"] = options.min_samples;

    auto res = client.Post("/dbscan", payload.dump(), "application/json");
    if (!res)
    {
        std::cerr << "Error: request to " << options.host_port
                  << " failed: " << httplib::to_string(res.error()) << "\n";
        return 1;
    }

    std::cout << res->status << std::endl;
    std::cout << res->body << std::endl;

    return res->status == 200 ? 0 : 1;
}
