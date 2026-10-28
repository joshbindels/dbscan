#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "argparser.h"
#include "httplib.h"
#include "json.hpp"

namespace
{

class Client
{
public:
    Client(double eps,
           int min_samples,
           std::string input_file,
           std::string output_dir,
           std::string host_port)
        : m_eps(eps)
        , m_min_samples(min_samples)
        , m_input_file(std::move(input_file))
        , m_output_dir(std::move(output_dir))
        , m_host_port(std::move(host_port))
    {
    }

    bool make_request()
    {
        std::string points;

        try
        {
            points = read_input_file();
        }
        catch (const std::exception& error)
        {
            std::cerr << "Error: " << error.what() << "\n";
            return false;
        }

        httplib::Client client(m_host_port);

        nlohmann::json payload = nlohmann::json::object();
        payload["eps"] = m_eps;
        payload["min_samples"] = m_min_samples;
        payload["points"] = points;

        auto res = client.Post("/dbscan", payload.dump(), "application/json");
        if (!res)
        {
            std::cerr << "Error: request to " << m_host_port
                      << " failed: " << httplib::to_string(res.error()) << "\n";
            return false;
        }

        if (res->status != 200)
        {
            std::cerr << "Error: server returned status " << res->status << "\n";
            return false;
        }

        try
        {
            const auto response = nlohmann::json::parse(res->body);

            // Here we could use a JSON validator to ensure the response format
            // is valid.
            return store_output(response);
        }
        catch (const std::exception& error)
        {
            std::cerr << "Error: invalid server response: " << error.what() << "\n";
            return false;
        }
    }

private:
    std::string read_input_file() const
    {
        // This could be a read_xyz_file method that validates the xyz format
        std::ifstream file(m_input_file, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("could not open input file " + m_input_file);
        }

        std::ostringstream contents;
        contents << file.rdbuf();

        if (file.bad())
        {
            throw std::runtime_error("could not read input file " + m_input_file);
        }

        return contents.str();
    }

    bool store_output(const nlohmann::json& response)
    {
        // The expected response is:
        // {
        //   "noise": "x y z\n...",
        //   "clusters": {
        //     "1": "x y z\n...",
        //     "2": "x y z\n..."
        //   }
        // }
        // Every point payload is a raw XYZ string.
        if (!write_xyz_file("noise.xyz", response.at("noise").get<std::string>()))
        {
            return false;
        }

        for (const auto& [key, data] : response.at("clusters").items())
        {
            if (!write_xyz_file("cluster_" + key + ".xyz", data.get<std::string>()))
            {
                return false;
            }
        }

        return true;
    }

    bool write_xyz_file(const std::string& filename, const std::string& data)
    {
        const auto path = std::filesystem::path(m_output_dir) / filename;
        std::ofstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            std::cerr << "Error: could not open output file " << path << "\n";
            return false;
        }

        file << data;

        // Ensure trailing newline
        if (!data.empty() && data.back() != '\n')
        {
            file << '\n';
        }

        if (!file)
        {
            std::cerr << "Error: could not write output file " << path << "\n";
            return false;
        }

        return true;
    }

    double m_eps;
    int m_min_samples;
    std::string m_input_file;
    std::string m_output_dir;
    std::string m_host_port;
};

} // namespace

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

    // Return early if we're showing the help message
    if (options.show_help)
    {
        return 0;
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

    Client client(options.eps,
                  options.min_samples,
                  options.input,
                  options.output_dir,
                  options.host_port);

    return client.make_request() ? 0 : 1;
}
