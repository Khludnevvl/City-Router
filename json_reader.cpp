#include "json_reader.h"
#include "JSONlib/json_builder.h"
#include "serialization.h"

#include <deque>
#include <iostream>
#include <set>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace transport_catalogue
{
namespace json_reader
{
using namespace std::string_literals;
JsonReader::JsonReader(TransportCatalogue &catalogue, request_handler::RequestHandler &handler,
                       renderer::MapRenderer &map_renderer)
    : catalogue_(catalogue), handler_(handler), map_renderer_(map_renderer)
{
}

void JsonReader::ReadMakeBaseRequest(std::istream &input)
{
    using namespace std::literals;
    json::Document doc = json::Load(input);
    const auto &base_requests_ = doc.GetRoot().AsDict().at("base_requests"s).AsArray();
    render_settings_ = doc.GetRoot().AsDict().at("render_settings"s).AsDict();
    routing_settings_ = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
    serialization_file_name = doc.GetRoot().AsDict().at("serialization_settings").AsDict().at("file").AsString();
    for (auto &request : base_requests_)
    {
        const auto &type = request.AsDict().at("type"s);
        if (type.AsString() == "Stop"s)
        {
            stop_requests_.emplace_back(std::move(request.AsDict()));
        }
        else if (type.AsString() == "Bus"s)
        {
            bus_requests_.emplace_back(std::move(request.AsDict()));
        }
    }
}

void JsonReader::ReadStatRequest(std::istream &input)
{
    using namespace std::literals;
    json::Document doc = json::Load(input);
    stat_requests_ = doc.GetRoot().AsDict().at("stat_requests"s).AsArray();
    serialization_file_name = doc.GetRoot().AsDict().at("serialization_settings").AsDict().at("file").AsString();
}
void JsonReader::ProcessRouteRequest()
{
    std::unordered_map<std::string, std::vector<std::string>> bus_to_stops;
    std::set<std::string> using_stops;
    for (const json::Dict &request : bus_requests_)
    {
        std::vector<std::string> stops;
        for (const json::Node &stop_name : request.at("stops"s).AsArray())
        {
            using_stops.insert(stop_name.AsString());
            stops.emplace_back(stop_name.AsString());
        }
        if (!request.at("is_roundtrip"s).AsBool())
        {
            for (int i = stops.size() - 2; i >= 0; i--)
            {
                stops.push_back(stops[i]);
            }
        }
        bus_to_stops[request.at("name"s).AsString()] = std::move(stops);
    }
    std::unordered_map<std::string, std::unordered_map<std::string, int>> stop_to_stops_distance_;
    for (const json::Dict &request : stop_requests_)
    {
        const std::string &stop_name_from = request.at("name"s).AsString();
        std::unordered_map<std::string, int> stop_to_distance;
        for (const auto &[stop_name_to, distance] : request.at("road_distances"s).AsDict())
        {
            stop_to_distance[stop_name_to] = distance.AsInt();
        }
        stop_to_stops_distance_[stop_name_from] = std::move(stop_to_distance);
    }

    transport_router::TransportRouterParams router_params;
    router_params.bus_to_stops = std::move(bus_to_stops);
    router_params.stop_to_stops_distance = std::move(stop_to_stops_distance_);
    router_params.using_stops = std::move(using_stops);
    router_params.bus_velocity = routing_settings_.at("bus_velocity"s).AsDouble();
    router_params.bus_wait_time = routing_settings_.at("bus_wait_time"s).AsInt();
    handler_.CreateRouter(std::move(router_params));
}

void JsonReader::SerializeCatalogue()
{
    ProcessRouteRequest();
    std::ofstream out(serialization_file_name, std::ios::binary);
    if (!out)
    {
        throw std::logic_error("Failed to open file: " + serialization_file_name);
    }
    transport_catalogue::serialization::Serializer serializer(stop_requests_, bus_requests_, render_settings_, *handler_.GetRouter());
    serializer.SerializeToOstream(out);
}

void JsonReader::DeserializeCatalogue()
{
    std::ifstream input(serialization_file_name, std::ios::binary);
    if (!input)
    {
        throw std::logic_error("Failed to open file: " + serialization_file_name);
    }

    transport_catalogue::serialization::Deserializer deserializer(catalogue_, map_renderer_, handler_.GetRouter());
    deserializer.DeserializeFromIstream(input);
}

void JsonReader::OutputRequest(std::ostream &output)
{
    json::Array result;
    for (const auto &out_request : stat_requests_)
    {
        const auto &type = out_request.AsDict().at("type"s);
        if (type.AsString() == "Bus"s)
        {
            result.emplace_back(OutputBusRequest(out_request.AsDict()));
        }
        else if (type.AsString() == "Stop"s)
        {
            result.emplace_back(OutputStopRequest(out_request.AsDict()));
        }
        else if (type.AsString() == "Map"s)
        {
            result.emplace_back(OutputMap(out_request.AsDict()));
        }
        else if (type.AsString() == "Route"s)
        {
            result.emplace_back(OutputRouteRequest(out_request.AsDict()));
        }
    }

    json::Print(json::Document{result}, output);
}

json::Dict JsonReader::OutputMap(const Request &request) const
{
    svg::Document map;
    handler_.RenderMap(map);
    std::stringstream io_stream;
    map.Render(io_stream);

    return json::Builder{}
        .StartDict()
        .Key("map"s).Value(io_stream.str())
        .Key("request_id"s).Value(request.at("id"s).AsInt())
        .EndDict()
        .Build()
        .AsDict();
}

json::Dict JsonReader::OutputBusRequest(const Request &request)
{
    using namespace std::string_literals;

    auto bus_info = handler_.GetBusStat(request.at("name"s).AsString());

    if (bus_info == std::nullopt)
    {
        return json::Builder{}
            .StartDict()
            .Key("error_message"s).Value("not found"s)
            .Key("request_id"s).Value(request.at("id"s).AsInt())
            .EndDict()
            .Build()
            .AsDict();
    }

    return json::Builder{}
        .StartDict()
        .Key("request_id"s).Value(request.at("id"s).AsInt())
        .Key("curvature"s).Value(bus_info->curvature)
        .Key("route_length"s).Value(bus_info->route_length)
        .Key("stop_count"s).Value(bus_info->stops_on_route)
        .Key("unique_stop_count"s).Value(bus_info->unique_stops)
        .EndDict()
        .Build()
        .AsDict();
}

json::Dict JsonReader::OutputStopRequest(const Request &request)
{
    using namespace std::string_literals;
    auto stop_info = handler_.GetBusesNameByStop(request.at("name"s).AsString());
    if (stop_info == std::nullopt)
    {
        return json::Builder{}
            .StartDict()
            .Key("error_message"s).Value("not found"s)
            .Key("request_id"s).Value(request.at("id"s).AsInt())
            .EndDict()
            .Build()
            .AsDict();
    }

    json::Array buses;
    for (const auto &bus : *stop_info)
    {
        buses.emplace_back(std::string{bus});
    }
    return json::Builder{}
        .StartDict()
        .Key("buses"s).Value(buses)
        .Key("request_id"s).Value(request.at("id"s).AsInt())
        .EndDict()
        .Build()
        .AsDict();
}

json::Dict JsonReader::OutputRouteRequest(const Request &request)
{
    using namespace std::string_literals;

    auto route_info = handler_.GetRouteInfo(request.at("from"s).AsString(), request.at("to"s).AsString());

    if (route_info == std::nullopt)
    {

        return json::Builder{}
            .StartDict()
            .Key("error_message"s).Value("not found"s)
            .Key("request_id"s).Value(request.at("id"s).AsInt())
            .EndDict()
            .Build()
            .AsDict();
    }
    json::Array result_items;
    for (const auto &item : route_info->items)
    {
        json::Dict json_wait_item = json::Builder{}
                                        .StartDict()
                                        .Key("type"s).Value("Wait"s)
                                        .Key("stop_name"s).Value(std::string(item.stop_name))
                                        .Key("time"s).Value(item.wait_time)
                                        .EndDict()
                                        .Build()
                                        .AsDict();
        result_items.push_back(std::move(json_wait_item));

        json::Dict json_bus_item = json::Builder{}
                                       .StartDict()
                                       .Key("type"s).Value("Bus"s)
                                       .Key("bus"s).Value(std::string(item.bus_name))
                                       .Key("span_count"s).Value(item.span_count)
                                       .Key("time"s).Value(item.time_in_road)
                                       .EndDict()
                                       .Build()
                                       .AsDict();

        result_items.push_back(std::move(json_bus_item));
    }
    return json::Builder{}
        .StartDict()
        .Key("request_id"s).Value(request.at("id"s).AsInt())
        .Key("total_time"s).Value(route_info->total_time)
        .Key("items"s).Value(result_items)
        .EndDict()
        .Build()
        .AsDict();
}

} // namespace json_reader
} // namespace transport_catalogue