#include "request_handler.h"
#include "transport_catalogue.h"

namespace transport_catalogue
{
namespace request_handler
{

using namespace domain;

RequestHandler::RequestHandler(const TransportCatalogue &catalogue, renderer::MapRenderer &renderer)
    : catalogue_(catalogue), renderer_(renderer)
{
}

std::optional<RequestHandler::BusInfo>
RequestHandler::GetBusStat(const std::string_view &bus_name) const
{
    const BusPtr tmp_bus = catalogue_.GetBus(bus_name);
    if (tmp_bus == nullptr)
    {
        return std::nullopt;
    }
    int stops_count = tmp_bus->GetStops().size();
    int unique_count = tmp_bus->CalculateUniqueStops();
    int lenght_route = catalogue_.CalculateRoute(tmp_bus);
    double curvature = lenght_route / CalculateGeographicalDistance(tmp_bus);

    BusInfo result{tmp_bus->GetName(), stops_count, unique_count, lenght_route, curvature};
    return result;
}

const std::optional<std::set<std::string_view>>
RequestHandler::GetBusesNameByStop(const std::string_view &stop_name) const
{
    StopPtr stop = catalogue_.GetStop(stop_name);
    if (stop == nullptr)
    {
        return std::nullopt;
    }
    const auto &stop_to_buses = catalogue_.GetAllStopToBuses();
    auto iter = stop_to_buses.find(stop);

    if (iter == stop_to_buses.end())
    {
        const static std::set<std::string_view> empty_set;
        return empty_set;
    }
    return iter->second;
}

svg::Document &RequestHandler::RenderMap(svg::Document &doc)
{
    std::vector<domain::BusPtr> buses;
    buses.reserve(catalogue_.GetAllBusesNames().size());

    for (const auto &bus_name : catalogue_.GetAllBusesNames())
    {
        buses.push_back(catalogue_.GetBus(bus_name));
    }

    const auto &stops_to_buses = catalogue_.GetAllStopToBuses();
    std::vector<domain::StopPtr> stops;
    stops.reserve(stops_to_buses.size());

    for (const auto &stop : catalogue_.GetAllStops())
    {
        if (stops_to_buses.count(stop.second))
        {
            stops.push_back(stop.second);
        }
    }

    return renderer_.Render(doc, buses, stops);
}

void RequestHandler::CreateRouter(transport_router::TransportRouterParams router_params)
{
    router_.emplace(std::move(router_params));
}

const std::optional<transport_router::TransportRouter>& RequestHandler::GetRouter() const
{
    return router_;
}

std::optional<transport_router::TransportRouter> &RequestHandler::GetRouter()
{
    return router_;
}

std::optional<transport_router::RouteInfo> RequestHandler::GetRouteInfo(const std::string &stop_from, const std::string &stop_to)
{
    if (catalogue_.GetAllBusesNames().size() == 0)
    {
        return std::nullopt;
    }
    std::optional<transport_router::RouteInfo> info = router_->BuildRoute(stop_from, stop_to);
    return info;
}

} // namespace request_handler
} // namespace transport_catalogue
