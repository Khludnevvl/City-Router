#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <iostream>
namespace transport_catalogue
{
namespace transport_router
{

struct RouteInfo
{
    double total_time;
    std::vector<graph::Edge<double>> items;
};

struct TransportRouterParams
{
    double bus_velocity;
    int bus_wait_time;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> stop_to_stops_distance;
    std::unordered_map<std::string, std::vector<std::string>> bus_to_stops;
    std::set<std::string> using_stops;
};

class TransportRouter
{
private:
public:
    using TransportGraph = graph::DirectedWeightedGraph<double>;

    explicit TransportRouter(TransportGraph graph, graph::Router<double>::RoutesInternalData internal_data,
                             std::unordered_map<std::string, size_t> stops_to_vertex_id)
        : graph_(std::move(graph)), stops_to_vertex_id_(std::move(stops_to_vertex_id))
    {
        router_.emplace(graph_, std::move(internal_data));
    }

    explicit TransportRouter(TransportRouterParams router_params)
        : bus_velocity_(router_params.bus_velocity), bus_wait_time_(router_params.bus_wait_time),
          graph_(router_params.using_stops.size()), stop_to_stops_distance_(std::move(router_params.stop_to_stops_distance)),
          bus_to_stops_(std::move(std::move(router_params.bus_to_stops))), using_stops_(std::move(router_params.using_stops))
    {
        CreateMap();

        for (const auto &[bus, stops] : bus_to_stops_)
        {
            for (size_t i = 0; i < stops.size() - 1; i++)
            {
                FinishStopRoute(i, stops, bus);
            }
        }
        router_.emplace(graph_);
    }

    std::optional<RouteInfo> BuildRoute(const std::string &stop_from, const std::string &stop_to) const;

    size_t GetStopId(std::string_view stop_name) const;

    const auto &GetRoutesInternalData() const
    {
        return router_->GetRoutesInternalData();
    }

    const TransportGraph &GetGraph() const
    {
        return graph_;
    }

    const std::unordered_map<std::string, size_t> &GetStopsToId() const
    {
        return stops_to_vertex_id_;
    }

private:
    int GetDistanceBetweenStops(const std::string &stop_from, const std::string &stop_to) const
    {
        auto iter = stop_to_stops_distance_.at(stop_from).find(stop_to);
        if (iter != stop_to_stops_distance_.at(stop_from).end())
        {
            return iter->second;
        }

        iter = stop_to_stops_distance_.at(stop_to).find(stop_from);
        if (iter != stop_to_stops_distance_.at(stop_to).end())
        {
            return iter->second;
        }
        using namespace std::string_literals;
        throw std::logic_error("Failed to get Distance between "s + stop_from + " and "s + stop_to);
        return 0;
    }

    double KilometersToMeters(double velocity) const;

    double CalculateTime(double distanse, double velocity) const;

    void FinishStopRoute(size_t stop_from_pos, const std::vector<std::string> &stops, const std::string& bus_name);

    void FinishConstructRouter();

    void CreateMap();

    double bus_velocity_;
    int bus_wait_time_;
    TransportGraph graph_;
    std::optional<graph::Router<double>> router_ = std::nullopt;
    std::unordered_map<size_t, std::string_view> vertex_ids_to_stop_;
    std::unordered_map<std::string, size_t> stops_to_vertex_id_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> stop_to_stops_distance_;
    std::unordered_map<std::string, std::vector<std::string>> bus_to_stops_;
    std::set<std::string> using_stops_;
};

} // namespace transport_router
} // namespace transport_catalogue