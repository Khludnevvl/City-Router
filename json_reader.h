#pragma once

#include "JSONlib/json.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <deque>
#include <string>
#include <unordered_map>

namespace transport_catalogue
{
namespace json_reader
{
using Request = std::map<std::string, json::Node>;

class JsonReader
{
public:
    JsonReader(TransportCatalogue &catalogue,
               request_handler::RequestHandler &handler,
               renderer::MapRenderer &map_renderer);

    void ReadMakeBaseRequest(std::istream &input);

    void ReadStatRequest(std::istream &input);

    void OutputRequest(std::ostream &ouput);

    void SerializeCatalogue();

    void DeserializeCatalogue();

private:

    json::Dict OutputMap(const Request &request) const;

    void ProcessRouteRequest();

    json::Dict OutputBusRequest(const Request &request);

    json::Dict OutputStopRequest(const Request &request);

    json::Dict OutputRouteRequest(const Request &request);

    TransportCatalogue &catalogue_;
    request_handler::RequestHandler &handler_;
    renderer::MapRenderer &map_renderer_;
    std::vector<Request> stop_requests_;
    std::vector<Request> bus_requests_;
    json::Array stat_requests_;
    json::Dict render_settings_;
    json::Dict routing_settings_;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> stop_to_stop_distance_requests_;
    std::string serialization_file_name;
};
} // namespace json_reader
} // namespace transport_catalogue