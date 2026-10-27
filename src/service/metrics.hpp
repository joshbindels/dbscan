#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/family.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace metrics
{

class Metrics
{
public:
    explicit Metrics(const std::string& exposer_address = "0.0.0.0:9091")
        : m_registry(std::make_shared<prometheus::Registry>())
        , m_requests(&create_request_counter_family(*m_registry))
        , m_request_duration(&create_histogram(
              *m_registry,
              "http_request_duration_milliseconds",
              "HTTP request duration in milliseconds",
              {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000}))
        , m_request_points(&create_histogram(
              *m_registry,
              "service_request_points",
              "Number of points in each request",
              {1, 10, 50, 100, 250, 500, 1000, 2500, 5000}))
        , m_dbscan_duration(&create_histogram(
              *m_registry,
              "dbscan_duration_milliseconds",
              "DBSCAN clustering duration in milliseconds",
              {1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000}))
        , m_dbscan_clusters(&create_histogram(
              *m_registry,
              "dbscan_clusters",
              "Number of clusters found per request",
              {0, 1, 2, 5, 10, 20, 50, 100, 250, 500, 1000}))
        , m_dbscan_noise_ratio(&create_histogram(
              *m_registry,
              "dbscan_noise_ratio",
              "Fraction of points classified as noise per request",
              {0.0, 0.05, 0.1, 0.25, 0.5, 0.75, 0.9, 0.95, 1.0}))
        , m_exposer(exposer_address)
    {
        m_exposer.RegisterCollectable(m_registry);
    }

    void record_request(int status_code)
    {
        m_requests->Add({{"status", std::to_string(status_code)}}).Increment();
    }

    void record_request_duration_ms(double milliseconds)
    {
        m_request_duration->Observe(milliseconds);
    }

    void record_request_points(size_t point_count)
    {
        m_request_points->Observe(static_cast<double>(point_count));
    }

    void record_dbscan_duration_ms(double milliseconds)
    {
        m_dbscan_duration->Observe(milliseconds);
    }

    void record_dbscan_clusters(size_t cluster_count)
    {
        m_dbscan_clusters->Observe(static_cast<double>(cluster_count));
    }

    void record_dbscan_noise_ratio(double ratio)
    {
        m_dbscan_noise_ratio->Observe(ratio);
    }

    std::shared_ptr<prometheus::Registry> registry() const
    {
        return m_registry;
    }

private:
    static prometheus::Family<prometheus::Counter>& create_request_counter_family(
        prometheus::Registry& registry)
    {
        return prometheus::BuildCounter()
            .Name("http_requests_total")
            .Help("Total number of HTTP requests by status code")
            .Register(registry);
    }

    static prometheus::Histogram& create_histogram(
        prometheus::Registry& registry,
        const std::string& name,
        const std::string& help,
        prometheus::Histogram::BucketBoundaries buckets)
    {
        return prometheus::BuildHistogram()
            .Name(name)
            .Help(help)
            .Register(registry)
            .Add({}, std::move(buckets));
    }

    std::shared_ptr<prometheus::Registry> m_registry;

    // Owned by the metric families inside m_registry.
    prometheus::Family<prometheus::Counter>* m_requests;
    prometheus::Histogram* m_request_duration;
    prometheus::Histogram* m_request_points;
    prometheus::Histogram* m_dbscan_duration;
    prometheus::Histogram* m_dbscan_clusters;
    prometheus::Histogram* m_dbscan_noise_ratio;

    prometheus::Exposer m_exposer;
};

} // namespace metrics
