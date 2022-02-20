#pragma once

#include "JSONlib/json.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <stdexcept>
#include <string>
#include <transport_catalogue.pb.h>
#include <unordered_map>
#include <vector>

namespace transport_catalogue
{
namespace serialization
{

class Serializer
{
public:
    explicit Serializer(const std::vector<json::Dict> &stops_request, const std::vector<json::Dict> &bus_requests,
                        const json::Dict &renderer_settings, const transport_router::TransportRouter &router);

    void SerializeToOstream(std::ostream &output);

private:
    transport_catalogue_serialize::TransportCatalogue SerializeTransportCatalogue();

    transport_catalogue_serialize::TransportDatabase SerializeTransportDatabase();

    transport_catalogue_serialize::EdgeList SerializeEdgeList() const ;

    transport_catalogue_serialize::IncidenceLists SerializeIncidenceLists() const;

    transport_catalogue_serialize::Graph SerializeGraph() const;

    transport_catalogue_serialize::RoutesInternalData SerializeRoutesInternalData() const;

    transport_catalogue_serialize::VertexInfoList SerializeVertexList() const;

    transport_catalogue_serialize::Router SerializeRouter() const;

    transport_catalogue_serialize::StopList SerializeStops();

    transport_catalogue_serialize::BusList SerializeBuses();

    transport_catalogue_serialize::DistanceList SerializeDistances();

    transport_catalogue_serialize::ColorPalette SerializeColorPalette();

    transport_catalogue_serialize::Color SerializeColor(const json::Node &node) const;

    transport_catalogue_serialize::RenderSettings SerializeRenderSettings();

    std::vector<json::Dict> stops_request_;
    std::vector<json::Dict> bus_requests_;
    json::Dict render_settings_;
    const transport_router::TransportRouter &router_;
    std::unordered_map<std::string, size_t> bus_name_to_id_;
    std::unordered_map<std::string, size_t> stop_name_to_id_;
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> stops_name_to_stop_to_distance_;
};

class Deserializer
{
public:
    explicit Deserializer(TransportCatalogue &catalogue_, renderer::MapRenderer &renderer_,
                          std::optional<transport_router::TransportRouter> &router);

    bool DeserializeFromIstream(std::istream &input);

private:
    void DeserializeTransportDatabase(transport_catalogue_serialize::TransportDatabase database);

    void DeserializeCatalogue(transport_catalogue_serialize::TransportCatalogue catalogue);

    void DeserializeStops(transport_catalogue_serialize::StopList stop_list);

    void DeserializeDistances(transport_catalogue_serialize::DistanceList distance_list);

    void DeserializeBuses(transport_catalogue_serialize::BusList bus_list);

    svg::Color DeserializeColor(transport_catalogue_serialize::Color color);

    std::vector<graph::Edge<double>> DeserializeEdgeList(transport_catalogue_serialize::EdgeList deserialized_edge_list) const;

    std::vector<std::vector<size_t>> DeserializeIncidenceLists(transport_catalogue_serialize::IncidenceLists deserialized_incidence_lists) const;

    graph::DirectedWeightedGraph<double> DeserializeGraph(transport_catalogue_serialize::Graph graph_settings) const;

    //std::vector<std::vector<std::optional<graph::Router<double>::RouteInternalData>>>
    graph::Router<double>::RoutesInternalData DeserializeRoutesInternalData(transport_catalogue_serialize::RoutesInternalData
                                                                                deserialized_routes_internal_data) const;

    void DeserializeTransportRouter(transport_catalogue_serialize::Router router_settings,
                                    std::optional<transport_router::TransportRouter> &router);

    void DeserializeMapRenderer(transport_catalogue_serialize::RenderSettings deserialized_settings,
                                renderer::MapRenderer &renderer);

    TransportCatalogue &catalogue_;
    renderer::MapRenderer &renderer_;
    std::optional<transport_router::TransportRouter> &router_;
    std::unordered_map<size_t, std::string> id_to_stop_name_;
    std::unordered_map<size_t, std::string> id_to_bus_name_;
};
} // namespace serialization
} //namespace transport_catalogue