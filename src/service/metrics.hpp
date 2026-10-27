#pragma once

#include <memory>
#include <string>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace metrics
{

class Metrics
{
public:
    explicit Metrics(const std::string& exposer_address = "0.0.0.0:9091")
        : m_registry(std::make_shared<prometheus::Registry>())
        , m_requests(&create_request_counter(*m_registry))
        , m_request_points(&create_request_points_histogram(*m_registry))
        , m_exposer(exposer_address)
    {
        m_exposer.RegisterCollectable(m_registry);
    }

    void record_request()
    {
        m_requests->Increment();
    }

    void record_request_points(size_t point_count)
    {
        m_request_points->Observe(static_cast<double>(point_count));
    }

    std::shared_ptr<prometheus::Registry> registry() const
    {
        return m_registry;
    }

private:
    static prometheus::Counter& create_request_counter(prometheus::Registry& registry)
    {
        return prometheus::BuildCounter()
            .Name("service_requests_total")
            .Help("Total number of requests handled by the service")
            .Register(registry)
            .Add({});
    }

    static prometheus::Histogram& create_request_points_histogram(
        prometheus::Registry& registry)
    {
        return prometheus::BuildHistogram()
            .Name("service_request_points")
            .Help("Number of points in each request")
            .Register(registry)
            .Add({}, prometheus::Histogram::BucketBoundaries{
                           1, 10, 50, 100, 250, 500, 1000, 2500, 5000});
    }

    std::shared_ptr<prometheus::Registry> m_registry;

    // Owned by the metric families inside m_registry.
    prometheus::Counter* m_requests;
    prometheus::Histogram* m_request_points;

    prometheus::Exposer m_exposer;
};

} // namespace metrics
