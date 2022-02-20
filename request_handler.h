#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <deque>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace transport_catalogue
{
namespace request_handler
{
class RequestHandler
{

    struct BusInfo
    {
        std::string name;
        int stops_on_route = 0;
        int unique_stops = 0;
        int route_length = 0;
        double curvature = 0;
    };

public:
    RequestHandler(const TransportCatalogue &catalogue, renderer::MapRenderer &renderer);

    std::optional<BusInfo> GetBusStat(const std::string_view &bus_name) const;

    const std::optional<std::set<std::string_view>> GetBusesNameByStop(const std::string_view &stop_name) const;

    svg::Document &RenderMap(svg::Document &doc);

    void CreateRouter(transport_router::TransportRouterParams router_params);

    std::optional<transport_router::RouteInfo> GetRouteInfo(const std::string &stop_from, const std::string &stop_to);

    const std::optional<transport_router::TransportRouter> &GetRouter() const;

    std::optional<transport_router::TransportRouter> &GetRouter();

private:
    const TransportCatalogue &catalogue_;
    std::optional<transport_router::TransportRouter> router_ = std::nullopt;
    renderer::MapRenderer &renderer_;
};
} // namespace request_handler
} // namespace transport_catalogue