#include "transport_router.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace transport_catalogue
{
namespace transport_router
{

std::optional<RouteInfo> TransportRouter::BuildRoute(const std::string& stop_from, const std::string& stop_to) const
{
    auto stop_from_iter = stops_to_vertex_id_.find(stop_from);
    if (stop_from_iter == stops_to_vertex_id_.end())
    {
        return std::nullopt;
    }
    auto stop_to_iter = stops_to_vertex_id_.find(stop_to);
    if (stop_to_iter == stops_to_vertex_id_.end())
    {
        return std::nullopt;
    }

    auto info = router_->BuildRoute(stop_from_iter->second, stop_to_iter->second);
    if (info == std::nullopt)
    {
        return std::nullopt;
    }
    if (info->edges.size() == 0)
    {
        return RouteInfo{};
    }
    RouteInfo result;
    result.total_time = info->weight;
    for (size_t edge_id : info->edges)
    {
        const auto &edge = graph_.GetEdge(edge_id);
        result.items.push_back(edge);
    }
    return result;
}

double TransportRouter::KilometersToMeters(double velocity) const
{
    return velocity * 1000.0 / 60.0;
}

double TransportRouter::CalculateTime(double distanse, double velocity) const
{
    return distanse / KilometersToMeters(velocity);
}

void TransportRouter::FinishStopRoute(size_t stop_from_pos, const std::vector<std::string> &stops,const  std::string& bus_name)
{
    const std::string &current_name = stops[stop_from_pos];
    const size_t current_stop_id = stops_to_vertex_id_.at(current_name);
    double result = 0;
    for (size_t stop_to_add_pos = stop_from_pos + 1; stop_to_add_pos < stops.size(); stop_to_add_pos++)
    {
        const size_t stop_id_to_add = stops_to_vertex_id_.at(stops[stop_to_add_pos]);
        const int distance_between_stops = GetDistanceBetweenStops(stops[stop_to_add_pos - 1], stops[stop_to_add_pos]);
        result += CalculateTime(static_cast<double>(distance_between_stops), bus_velocity_);

        graph::Edge<double> route_edge;
        route_edge.wait_time = bus_wait_time_;
        route_edge.time_in_road = result;
        route_edge.stop_name = current_name;
        route_edge.span_count = stop_to_add_pos - stop_from_pos;
        route_edge.bus_name = bus_name;
        route_edge.weight = result + bus_wait_time_;
        route_edge.from = current_stop_id;
        route_edge.to = stop_id_to_add;
        graph_.AddEdge(std::move(route_edge));
    }
}

void TransportRouter::CreateMap()
{
    size_t current_stop_id = 0;
    for (const auto &stop_name : using_stops_)
    {
        stops_to_vertex_id_[stop_name] = current_stop_id;
        vertex_ids_to_stop_[current_stop_id] = stop_name;
        current_stop_id++;
    }
}

} // namespace transport_router
} // namespace transport_catalogue