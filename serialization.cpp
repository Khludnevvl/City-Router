#include "serialization.h"
#include "JSONlib/json.h"
#include "SvgLib/svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <transport_catalogue.pb.h>
#include <unordered_map>
#include <vector>

namespace transport_catalogue
{
namespace serialization
{

using namespace std::string_literals;

Serializer::Serializer(const std::vector<json::Dict> &stops_request, const std::vector<json::Dict> &bus_requests,
                       const json::Dict &render_settings, const transport_router::TransportRouter &router)
    : stops_request_(stops_request), bus_requests_(bus_requests), render_settings_(render_settings), router_(router)
{
}

transport_catalogue_serialize::EdgeList Serializer::SerializeEdgeList() const
{
    const auto &graph = router_.GetGraph();
    const std::vector<graph::Edge<double>> &edges = graph.GetEdges();
    transport_catalogue_serialize::EdgeList serializing_edge_list;
    size_t current_id = 0;
    for (auto &edge : edges)
    {
        transport_catalogue_serialize::Edge serializing_edge;
        serializing_edge.set_bus_id(bus_name_to_id_.at(edge.bus_name));
        serializing_edge.set_stop_id(stop_name_to_id_.at(edge.stop_name));
        serializing_edge.set_span_count(edge.span_count);
        serializing_edge.set_wait_time(edge.wait_time);
        serializing_edge.set_time_in_road(edge.time_in_road);
        serializing_edge.set_from_id(edge.from);
        serializing_edge.set_to_id(edge.to);
        serializing_edge.set_weight(edge.weight);
        serializing_edge_list.add_edge();
        *serializing_edge_list.mutable_edge(current_id) = serializing_edge;
        current_id++;
    }
    return serializing_edge_list;
}

transport_catalogue_serialize::IncidenceLists Serializer::SerializeIncidenceLists() const
{
    transport_catalogue_serialize::IncidenceLists serializing_incidence_lists_;
    const auto &graph = router_.GetGraph();
    for (const auto &incidence_list : graph.GetIncedenceLists())
    {
        transport_catalogue_serialize::IncidenceList serializing_incidence_list;
        for (size_t edge_id : incidence_list)
        {
            serializing_incidence_list.add_edge_id(edge_id);
        }
        size_t current_lists_pos = serializing_incidence_lists_.incidence_list_size();
        serializing_incidence_lists_.add_incidence_list();
        *serializing_incidence_lists_.mutable_incidence_list(current_lists_pos) = serializing_incidence_list;
    }
    return std::move(serializing_incidence_lists_);
}

transport_catalogue_serialize::Graph Serializer::SerializeGraph() const
{
    transport_catalogue_serialize::Graph serializing_graph;

    transport_catalogue_serialize::EdgeList serializing_edge_list = SerializeEdgeList();

    transport_catalogue_serialize::IncidenceLists serializing_incidence_lists_ = SerializeIncidenceLists();

    *serializing_graph.mutable_edge_list() = serializing_edge_list;
    *serializing_graph.mutable_incidence_lists() = serializing_incidence_lists_;

    return std::move(serializing_graph);
}

transport_catalogue_serialize::RoutesInternalData Serializer::SerializeRoutesInternalData() const
{
    transport_catalogue_serialize::RoutesInternalData serializing_routes_internal_data;
    const auto &routes_internal_data_ = router_.GetRoutesInternalData(); //std::vector<std::vector<std::optional<RouteInternalData>>>;

    for (const auto &route_internal_list_ : routes_internal_data_)
    {
        transport_catalogue_serialize::RouteInternalDataList serializing_route_internal_data_list;
        for (auto route_internal_data : route_internal_list_) // std::optional<RouteInternalData>
        {
            transport_catalogue_serialize::RouteInternalData current_route_data;
            if (route_internal_data != std::nullopt)
            {
                current_route_data.set_is_exist(true);
                current_route_data.set_weight(route_internal_data->weight);
                if (route_internal_data->prev_edge != std::nullopt)
                {
                    transport_catalogue_serialize::EdgeId current_id;
                    current_id.set_id(*route_internal_data->prev_edge);
                    *current_route_data.mutable_prev_edge() = current_id;
                }
            }
            else
            {
                current_route_data.set_is_exist(false);
            }
            const size_t current_list_pos = serializing_route_internal_data_list.route_internal_data_size();
            serializing_route_internal_data_list.add_route_internal_data();
            *serializing_route_internal_data_list.mutable_route_internal_data(current_list_pos) = current_route_data;
        }
        const size_t current_routes_pos = serializing_routes_internal_data.route_internal_data_list_size();
        serializing_routes_internal_data.add_route_internal_data_list();
        *serializing_routes_internal_data.mutable_route_internal_data_list(current_routes_pos) = serializing_route_internal_data_list;
    }
    return std::move(serializing_routes_internal_data);
}

transport_catalogue_serialize::VertexInfoList Serializer::SerializeVertexList() const
{
    const std::unordered_map<std::string, size_t> &stop_name_to_vertex_id_ = router_.GetStopsToId();

    transport_catalogue_serialize::VertexInfoList serializing_vertex_list;
    for (const auto &[stop_name, vertex_id] : stop_name_to_vertex_id_)
    {
        transport_catalogue_serialize::VertexInfo curr_vertex_info;
        curr_vertex_info.set_stop_id(stop_name_to_id_.at(stop_name));
        curr_vertex_info.set_vertex_id(vertex_id);
        const size_t current_pos = serializing_vertex_list.vertex_info_size();

        serializing_vertex_list.add_vertex_info();
        *serializing_vertex_list.mutable_vertex_info(current_pos) = curr_vertex_info;
    }
    return std::move(serializing_vertex_list);
}

transport_catalogue_serialize::Router Serializer::SerializeRouter() const
{
    transport_catalogue_serialize::Graph serializing_graph = SerializeGraph();
    transport_catalogue_serialize::RoutesInternalData serializing_routes_internal_data = SerializeRoutesInternalData();
    transport_catalogue_serialize::VertexInfoList serializing_vertex_list = SerializeVertexList();

    transport_catalogue_serialize::Router serializing_router;

    *serializing_router.mutable_graph() = serializing_graph;
    *serializing_router.mutable_routes_internal_data() = serializing_routes_internal_data;
    *serializing_router.mutable_vertex_info_list() = serializing_vertex_list;
    return std::move(serializing_router);
}

transport_catalogue_serialize::StopList Serializer::SerializeStops() 
{
    transport_catalogue_serialize::StopList result_list;
    for (size_t i = 0; i < stops_request_.size(); i++)
    {
        transport_catalogue_serialize::Coordinate current_coord;
        transport_catalogue_serialize::Stop current_stop;
        const std::string &stop_name = stops_request_[i].at("name"s).AsString();
        current_coord.set_lat(stops_request_[i].at("latitude"s).AsDouble());
        current_coord.set_lng(stops_request_[i].at("longitude"s).AsDouble());
        current_stop.set_name(stop_name);
        *current_stop.mutable_coordinate() = current_coord;
        current_stop.set_id(i);

        result_list.add_stop();
        *result_list.mutable_stop(i) = current_stop;
        if (stops_request_[i].count("road_distances"s))
        {
            std::unordered_map<std::string, size_t> name_to_dist;
            const json::Dict &road_distances = stops_request_[i].at("road_distances"s).AsDict();
            for (const auto &[name, distance] : road_distances)
            {
                name_to_dist[name] = distance.AsInt();
            }
            stops_name_to_stop_to_distance_[stop_name] = name_to_dist;
        }
        stop_name_to_id_[stop_name] = i;
    }
    return result_list;
}

transport_catalogue_serialize::DistanceList Serializer::SerializeDistances()
{

    transport_catalogue_serialize::DistanceList result_list;
    for (const auto &[stop_name_from, stop_to_dist] : stops_name_to_stop_to_distance_)
    {

        for (const auto &[stop_name_to, distance] : stop_to_dist)
        {
            transport_catalogue_serialize::Distance current_distance;
            current_distance.set_stop_from_id(stop_name_to_id_.at(stop_name_from));
            current_distance.set_stop_to_id(stop_name_to_id_.at(stop_name_to));
            current_distance.set_value(distance);
            const size_t current_pos = result_list.distance_size();

            result_list.add_distance();
            *result_list.mutable_distance(current_pos) = current_distance;
        }
    }
    return result_list;
}

transport_catalogue_serialize::BusList Serializer::SerializeBuses()
{
    transport_catalogue_serialize::BusList result_list;
    size_t current_id = 0;
    for (const json::Dict &request : bus_requests_)
    {

        transport_catalogue_serialize::Bus current_bus;
        const std::string bus_name = request.at("name"s).AsString();
        current_bus.set_name(bus_name);
        current_bus.set_id(current_id);
        current_bus.set_is_roundtrip(request.at("is_roundtrip"s).AsBool());

        for (const auto &stop : request.at("stops"s).AsArray())
        {
            current_bus.add_stop_id(stop_name_to_id_.at(stop.AsString()));
        }
        const size_t current_pos = result_list.bus_size();

        result_list.add_bus();
        *result_list.mutable_bus(current_pos) = current_bus;

        bus_name_to_id_[std::move(bus_name)] = current_id;
        current_id++;
    }

    return result_list;
}

transport_catalogue_serialize::Color Serializer::SerializeColor(const json::Node &node) const
{
    if (node.IsArray())
    {
        auto color = node.AsArray();
        if (color.size() == 3)
        {
            transport_catalogue_serialize::Rgb serialize_rgb;
            serialize_rgb.set_red(color[0].AsInt());
            serialize_rgb.set_green(color[1].AsInt());
            serialize_rgb.set_blue(color[2].AsInt());
            transport_catalogue_serialize::Color serialize_color;
            *serialize_color.mutable_rgb() = serialize_rgb;
            return std::move(serialize_color);
        }
        else if (color.size() == 4)
        {
            transport_catalogue_serialize::Rgba serialize_rgba;
            serialize_rgba.set_red(color[0].AsInt());
            serialize_rgba.set_green(color[1].AsInt());
            serialize_rgba.set_blue(color[2].AsInt());
            serialize_rgba.set_opacity(color[3].AsDouble());
            transport_catalogue_serialize::Color serialize_color;
            *serialize_color.mutable_rgba() = serialize_rgba;
            return std::move(serialize_color);
        }
    }
    else if (node.IsString())
    {
        transport_catalogue_serialize::Color serialize_color;
        serialize_color.set_str(node.AsString());
        return std::move(serialize_color);
    }
    throw std::runtime_error("Failed to get color from JSON"s);
}

transport_catalogue_serialize::ColorPalette Serializer::SerializeColorPalette()
{
    const auto &color_array = render_settings_.at("color_palette"s).AsArray();
    transport_catalogue_serialize::ColorPalette color_palette;
    size_t current_id = 0;
    for (const json::Node &color : color_array)
    {
        color_palette.add_color();
        *color_palette.mutable_color(current_id) = SerializeColor(color);
        current_id++;
    }
    return std::move(color_palette);
}

transport_catalogue_serialize::RenderSettings Serializer::SerializeRenderSettings()
{
    transport_catalogue_serialize::RenderSettings result_settings;

    result_settings.set_width(render_settings_.at("width"s).AsDouble());
    result_settings.set_height(render_settings_.at("height"s).AsDouble());
    result_settings.set_padding(render_settings_.at("padding"s).AsDouble());
    result_settings.set_line_width(render_settings_.at("line_width"s).AsDouble());
    result_settings.set_stop_radius(render_settings_.at("stop_radius"s).AsDouble());
    result_settings.set_bus_label_font_size(render_settings_.at("bus_label_font_size"s).AsInt());

    const auto bus_label_offset_arr = render_settings_.at("bus_label_offset"s).AsArray();
    result_settings.set_bus_label_offset_dx(bus_label_offset_arr[0].AsDouble());
    result_settings.set_bus_label_offset_dy(bus_label_offset_arr[1].AsDouble());
    result_settings.set_stop_label_font_size(render_settings_.at("stop_label_font_size"s).AsInt());

    const auto stop_label_offset_arr = render_settings_.at("stop_label_offset"s).AsArray();
    result_settings.set_stop_label_offset_dx(stop_label_offset_arr[0].AsDouble());
    result_settings.set_stop_label_offset_dy(stop_label_offset_arr[1].AsDouble());
    result_settings.set_underlayer_width(render_settings_.at("underlayer_width"s).AsDouble());

    *result_settings.mutable_underlayer_color() = SerializeColor(render_settings_.at("underlayer_color"s));

    *result_settings.mutable_color_palette() = SerializeColorPalette();

    return result_settings;
}

transport_catalogue_serialize::TransportCatalogue Serializer::SerializeTransportCatalogue()
{
    transport_catalogue_serialize::StopList stop_list = SerializeStops();
    transport_catalogue_serialize::DistanceList distance_list = SerializeDistances();
    transport_catalogue_serialize::BusList bus_list = SerializeBuses();

    transport_catalogue_serialize::TransportCatalogue serialize_catalogue;

    *serialize_catalogue.mutable_stops() = stop_list;
    *serialize_catalogue.mutable_distances() = distance_list;
    *serialize_catalogue.mutable_buses() = bus_list;

    return std::move(serialize_catalogue);
}

transport_catalogue_serialize::TransportDatabase Serializer::SerializeTransportDatabase()
{    
    transport_catalogue_serialize::TransportCatalogue serialize_catalogue = SerializeTransportCatalogue();
    transport_catalogue_serialize::RenderSettings render_settings = SerializeRenderSettings();
    transport_catalogue_serialize::Router serializing_router = SerializeRouter();
    transport_catalogue_serialize::TransportDatabase serialize_database;

    *serialize_database.mutable_catalogue() = serialize_catalogue;
    *serialize_database.mutable_render_settings() = render_settings;
    *serialize_database.mutable_router() = serializing_router;

    return std::move(serialize_database);
}

void Serializer::SerializeToOstream(std::ostream &output)
{
    transport_catalogue_serialize::TransportDatabase serialize_database = SerializeTransportDatabase();

    serialize_database.SerializeToOstream(&output);
}

//////////////////////////////////////////////////////////////////////////////////////////////

Deserializer::Deserializer(TransportCatalogue &catalogue, renderer::MapRenderer &renderer,
                           std::optional<transport_router::TransportRouter> &router)
    : catalogue_(catalogue), renderer_(renderer), router_(router)
{
}

void Deserializer::DeserializeStops(transport_catalogue_serialize::StopList stop_list)
{
    for (size_t i = 0; i < stop_list.stop_size(); i++)
    {
        catalogue_.AddStop(stop_list.stop(i).name(), {stop_list.stop(i).coordinate().lat(), stop_list.stop(i).coordinate().lng()});
        id_to_stop_name_[stop_list.stop(i).id()] = stop_list.stop(i).name();
    }
}

void Deserializer::DeserializeDistances(transport_catalogue_serialize::DistanceList distance_list)
{
    size_t distance_size = distance_list.distance_size();
    for (size_t i = 0; i < distance_size; i++)
    {
        const std::string stop_from_name = id_to_stop_name_.at(distance_list.distance(i).stop_from_id());
        const std::string stop_to_name = id_to_stop_name_.at(distance_list.distance(i).stop_to_id());
        const size_t distance = distance_list.distance(i).value();
        catalogue_.AddStopsDistance(stop_from_name, stop_to_name, distance);
    }
}

void Deserializer::DeserializeBuses(transport_catalogue_serialize::BusList bus_list)
{
    for (size_t i = 0; i < bus_list.bus_size(); i++)
    {
        bool is_roadtrip = bus_list.bus(i).is_roundtrip();
        const std::string bus_name = bus_list.bus(i).name();
        std::vector<std::string> stop_names;
        size_t stops_size = bus_list.bus(i).stop_id_size();
        for (size_t j = 0; j < stops_size; j++)
        {
            const size_t stop_id = bus_list.bus(i).stop_id(j);
            const std::string stop_name = id_to_stop_name_.at(stop_id);
            stop_names.push_back(std::move(stop_name));
        }
        id_to_bus_name_[bus_list.bus(i).id()] = bus_list.bus(i).name();
        catalogue_.AddBus(std::move(bus_name), std::move(stop_names), is_roadtrip);
    }
}

std::vector<graph::Edge<double>> Deserializer::DeserializeEdgeList(transport_catalogue_serialize::EdgeList deserialized_edge_list) const
{
    std::vector<graph::Edge<double>> edge_list;
    const size_t edge_list_size = deserialized_edge_list.edge_size();
    for (size_t i = 0; i < edge_list_size; i++)
    {
        graph::Edge<double> current_edge;
        current_edge.bus_name = id_to_bus_name_.at(deserialized_edge_list.edge(i).bus_id());
        current_edge.stop_name = id_to_stop_name_.at(deserialized_edge_list.edge(i).stop_id());
        current_edge.span_count = deserialized_edge_list.edge(i).span_count();
        current_edge.wait_time = deserialized_edge_list.edge(i).wait_time();
        current_edge.time_in_road = deserialized_edge_list.edge(i).time_in_road();
        current_edge.from = deserialized_edge_list.edge(i).from_id();
        current_edge.to = deserialized_edge_list.edge(i).to_id();
        current_edge.weight = deserialized_edge_list.edge(i).weight();
        edge_list.push_back(std::move(current_edge));
    }
    return std::move(edge_list);
}

std::vector<std::vector<size_t>> Deserializer::DeserializeIncidenceLists(transport_catalogue_serialize::IncidenceLists deserialized_incidence_lists) const
{
    std::vector<std::vector<size_t>> incidence_lists;
    incidence_lists.reserve(deserialized_incidence_lists.incidence_list_size());

    for (size_t i = 0; i < deserialized_incidence_lists.incidence_list_size(); i++)
    {
        std::vector<size_t> incidence_list;
        incidence_list.reserve(deserialized_incidence_lists.incidence_list(i).edge_id_size());

        for (size_t j = 0; j < deserialized_incidence_lists.incidence_list(i).edge_id_size(); j++)
        {
            const size_t current_edge_id = deserialized_incidence_lists.incidence_list(i).edge_id(j);
            incidence_list.push_back(current_edge_id);
        }
        incidence_lists.push_back(std::move(incidence_list));
    }
    return std::move(incidence_lists);
}

graph::DirectedWeightedGraph<double> Deserializer::DeserializeGraph(transport_catalogue_serialize::Graph graph_settings) const
{
    std::vector<graph::Edge<double>> edge_list = DeserializeEdgeList(graph_settings.edge_list());

    std::vector<std::vector<size_t>> incidence_lists = DeserializeIncidenceLists(graph_settings.incidence_lists());

    graph::DirectedWeightedGraph<double> result_graph;

    result_graph.SetIncidenceLists(std::move(incidence_lists));
    result_graph.SetEdges(std::move(edge_list));

    return std::move(result_graph);
}

//std::vector<std::vector<std::optional<graph::Router<double>::RouteInternalData>>>
graph::Router<double>::RoutesInternalData Deserializer::DeserializeRoutesInternalData(transport_catalogue_serialize::RoutesInternalData
                                                                                          deserialized_routes_internal_data) const
{
    graph::Router<double>::RoutesInternalData routes_internal_data;
    for (size_t i = 0; i < deserialized_routes_internal_data.route_internal_data_list_size(); i++)
    {
        std::vector<std::optional<graph::Router<double>::RouteInternalData>> route_internal_data_list;
        transport_catalogue_serialize::RouteInternalDataList deser_data_list = deserialized_routes_internal_data.route_internal_data_list(i);
        for (int j = 0; j < deser_data_list.route_internal_data_size(); j++)
        {
            transport_catalogue_serialize::RouteInternalData deser_current_data = deser_data_list.route_internal_data(j);
            std::optional<graph::Router<double>::RouteInternalData> curr_data;
            if (deser_current_data.is_exist())
            {
                std::optional<size_t> prev_edge;
                if (deser_current_data.has_prev_edge())
                {
                    prev_edge = deser_current_data.prev_edge().id();
                }
                else
                {
                    prev_edge = std::nullopt;
                }
                double weight = deser_current_data.weight();
                curr_data = {weight, prev_edge};
            }
            else
            {
                curr_data == std::nullopt;
            }
            route_internal_data_list.push_back(curr_data);
        }
        routes_internal_data.push_back(std::move(route_internal_data_list));
    }
    return std::move(routes_internal_data);
}

void Deserializer::DeserializeTransportRouter(transport_catalogue_serialize::Router router_settings,
                                              std::optional<transport_router::TransportRouter> &router)
{
    graph::DirectedWeightedGraph<double> result_graph = DeserializeGraph(router_settings.graph());

    graph::Router<double>::RoutesInternalData routes_internal_data = DeserializeRoutesInternalData(router_settings.routes_internal_data());

    transport_catalogue_serialize::VertexInfoList deserialized_vertex_list = router_settings.vertex_info_list();
    std::unordered_map<std::string, size_t> stop_name_to_vertex_id;
    for (size_t i = 0; i < deserialized_vertex_list.vertex_info_size(); i++)
    {
        transport_catalogue_serialize::VertexInfo current_info = deserialized_vertex_list.vertex_info(i);
        stop_name_to_vertex_id[id_to_stop_name_.at(current_info.stop_id())] = current_info.vertex_id();
    }

    router.emplace(std::move(result_graph), std::move(routes_internal_data), std::move(stop_name_to_vertex_id));
}

svg::Color Deserializer::DeserializeColor(transport_catalogue_serialize::Color color)
{
    if (color.has_rgb())
    {
        transport_catalogue_serialize::Rgb deserialized_rgb = color.rgb();
        return svg::Color(svg::Rgb(deserialized_rgb.red(), deserialized_rgb.green(), deserialized_rgb.blue()));
    }
    else if (color.has_rgba())
    {
        transport_catalogue_serialize::Rgba deserialized_rgba = color.rgba();
        return svg::Color(svg::Rgba(deserialized_rgba.red(), deserialized_rgba.green(),
                                    deserialized_rgba.blue(), deserialized_rgba.opacity()));
    }
    else if (color.str() != ""s)
    {
        return svg::Color(color.str());
    }
    throw std::runtime_error("Failed to get deserialized color");
}

void Deserializer::DeserializeMapRenderer(transport_catalogue_serialize::RenderSettings deserialized_settings,
                                          renderer::MapRenderer &renderer)
{
    renderer::RendererSettings renderer_settings;
    renderer_settings.width = deserialized_settings.width();
    renderer_settings.height = deserialized_settings.height();
    renderer_settings.padding = deserialized_settings.padding();
    renderer_settings.line_width = deserialized_settings.line_width();
    renderer_settings.stop_radius = deserialized_settings.stop_radius();
    renderer_settings.bus_label_font_size = deserialized_settings.bus_label_font_size();
    renderer_settings.bus_label_offset = {deserialized_settings.bus_label_offset_dx(), deserialized_settings.bus_label_offset_dy()};
    renderer_settings.stop_label_font_size = deserialized_settings.stop_label_font_size();
    renderer_settings.stop_label_offset = {deserialized_settings.stop_label_offset_dx(), deserialized_settings.stop_label_offset_dy()};
    renderer_settings.underlayer_width = deserialized_settings.underlayer_width();
    
    renderer_settings.underlayer_color = DeserializeColor(deserialized_settings.underlayer_color());
    transport_catalogue_serialize::ColorPalette deserialized_color_palette = deserialized_settings.color_palette();
    for (size_t i = 0; i < deserialized_color_palette.color_size(); i++)
    {
        renderer_settings.AddColor(DeserializeColor(deserialized_color_palette.color(i)));
    }
    renderer.SetSettings(std::move(renderer_settings));
}

void Deserializer::DeserializeCatalogue(transport_catalogue_serialize::TransportCatalogue catalogue)
{
    DeserializeStops(catalogue.stops());
    DeserializeDistances(catalogue.distances());
    DeserializeBuses(catalogue.buses());
}

void Deserializer::DeserializeTransportDatabase(transport_catalogue_serialize::TransportDatabase database)
{
    DeserializeCatalogue(database.catalogue());
    DeserializeMapRenderer(database.render_settings(), renderer_);
    DeserializeTransportRouter(database.router(), router_);
}

bool Deserializer::DeserializeFromIstream(std::istream &input)
{
    transport_catalogue_serialize::TransportDatabase deserialized_database;

    bool is_database_correct = deserialized_database.ParseFromIstream(&input);
    if (!is_database_correct)
    {
        return false;
    }

    DeserializeTransportDatabase(std::move(deserialized_database));

    return true;
}
} // namespace serialization
} // namespace transport_catalogue