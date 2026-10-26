#pragma once

/*
 * A simple arg parser for the processPointcloud.cpp cli argument format
 */

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

namespace argparse
{

struct Options
{
    std::string input;
    std::string output_dir;
    double eps = 0.0;
    int min_samples = 0;
    std::string host_port;
    bool show_help = false;
};

inline void print_usage(const char* program)
{
    std::cerr
        << "Usage: " << program
        << " --input <input.xyz> --output-dir <out_dir>"
        << " --eps <eps> --min_samples <min_samples> <host:port>\n";
}

inline bool parse_double(const std::string& value, double& result)
{
    try
    {
        std::size_t parsed = 0;
        result = std::stod(value, &parsed);
        return parsed == value.size() && std::isfinite(result);
    }
    catch (const std::exception&)
    {
        return false;
    }
}

inline bool parse_int(const std::string& value, int& result)
{
    try
    {
        std::size_t parsed = 0;
        result = std::stoi(value, &parsed);
        return parsed == value.size();
    }
    catch (const std::exception&)
    {
        return false;
    }
}

inline bool is_host_port(const std::string& value)
{
    const auto colon = value.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == value.size())
    {
        return false;
    }

    int port = 0;
    return parse_int(value.substr(colon + 1), port) && port > 0 && port <= 65535;
}

inline Options parse_arguments(int argc, char** argv)
{
    Options options;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "--help" || arg == "-h")
        {
            options.show_help = true;
            return options;
        }
        else if (arg == "--input")
        {
            if (++i >= argc)
            {
                throw std::runtime_error("--input requires a value");
            }
            options.input = argv[i];
        }
        else if (arg == "--output-dir")
        {
            if (++i >= argc)
            {
                throw std::runtime_error("--output-dir requires a value");
            }
            options.output_dir = argv[i];
        }
        else if (arg == "--eps")
        {
            if (++i >= argc || !parse_double(argv[i], options.eps))
            {
                throw std::runtime_error("--eps requires a positive number");
            }
            if (options.eps <= 0.0)
            {
                throw std::runtime_error("--eps must be greater than zero");
            }
        }
        else if (arg == "--min_samples")
        {
            if (++i >= argc || !parse_int(argv[i], options.min_samples))
            {
                throw std::runtime_error("--min_samples requires a positive integer");
            }
            if (options.min_samples <= 0)
            {
                throw std::runtime_error("--min_samples must be greater than zero");
            }
        }
        else if (!arg.empty() && arg.front() == '-')
        {
            throw std::runtime_error("unknown option " + arg);
        }
        else
        {
            if (!options.host_port.empty())
            {
                throw std::runtime_error("only one host:port argument is allowed");
            }
            options.host_port = arg;
        }
    }

    if (options.input.empty())
    {
        throw std::runtime_error("missing required argument: --input");
    }
    if (options.output_dir.empty())
    {
        throw std::runtime_error("missing required argument: --output-dir");
    }
    if (options.eps == 0.0)
    {
        throw std::runtime_error("missing required argument: --eps");
    }
    if (options.min_samples == 0)
    {
        throw std::runtime_error("missing required argument: --min_samples");
    }
    if (options.host_port.empty())
    {
        throw std::runtime_error("missing required argument: host:port");
    }
    if (!is_host_port(options.host_port))
    {
        throw std::runtime_error("host:port must be in the form host:port");
    }

    return options;
}

} // namespace argparse
