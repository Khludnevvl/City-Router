
#include "map_renderer.h"
#include "SvgLib/svg.h"
#include "domain.h"
#include "geo.h"
#include "json_reader.h"

namespace transport_catalogue
{

namespace renderer
{
using namespace domain;

svg::Document &MapRenderer::PrintRoad(const domain::BusPtr bus, svg::Document &doc)
{

    auto stops = bus->GetStops();
    svg::Polyline road;

    for (size_t i = 0; i < stops.size(); i++)
    {
        road.AddPoint((*projector_)(stops[i]->GetCoordinates()));
    }

    road.SetStrokeColor(settings_.color_palette[current_color_index]);

    bus_to_color[bus] = settings_.color_palette[current_color_index];

    current_color_index++;
    if (settings_.color_palette.size() == current_color_index)
    {
        current_color_index = 0;
    }
    road.SetStrokeWidth(settings_.line_width).SetFillColor(svg::NoneColor).SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    road.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(road);
    return doc;
}

svg::Document &MapRenderer::PrintBusName(const domain::BusPtr bus, svg::Document &doc) const
{
    const auto &bus_stops = bus->GetStops();
    svg::Text bus_name_first;
    bus_name_first.SetPosition((*projector_)(bus_stops[0]->GetCoordinates())).SetOffset(settings_.bus_label_offset).SetFontSize(settings_.bus_label_font_size);
    bus_name_first.SetFontFamily("Verdana").SetFontWeight("bold").SetData(bus->GetName());
    svg::Text underlayer = bus_name_first;
    underlayer.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color).SetStrokeWidth(settings_.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    bus_name_first.SetFillColor(bus_to_color.at(bus));
    doc.Add(underlayer);
    doc.Add(bus_name_first);

    if (!bus->IsCircle() && bus_stops[bus_stops.size() / 2] != bus_stops[0])
    {
        const auto last_position = (*projector_)(bus_stops[bus_stops.size() / 2]->GetCoordinates());
        svg::Text bus_name_second = bus_name_first;
        bus_name_second.SetPosition(last_position);
        svg::Text underlayer_second = underlayer;
        underlayer_second.SetPosition(last_position);
        doc.Add(underlayer_second);
        doc.Add(bus_name_second);
    }
    return doc;
}
svg::Document &MapRenderer::PrintStopSymbol(const domain::StopPtr stop, svg::Document &doc) const
{
    svg::Circle stop_symbol;
    stop_symbol.SetRadius(settings_.stop_radius);
    stop_symbol.SetCenter((*projector_)(stop->GetCoordinates()));
    stop_symbol.SetFillColor("white");
    doc.Add(stop_symbol);
    return doc;
}
svg::Document &MapRenderer::PrintStopName(const domain::StopPtr stop, svg::Document &doc) const
{
    svg::Text stop_name;
    stop_name.SetPosition((*projector_)(stop->GetCoordinates())).SetOffset(settings_.stop_label_offset).SetFontSize(settings_.stop_label_font_size);
    stop_name.SetFontFamily("Verdana").SetData(stop->GetName());
    svg::Text underlayer = stop_name;
    stop_name.SetFillColor("black");
    underlayer.SetFillColor(settings_.underlayer_color).SetStrokeColor(settings_.underlayer_color).SetStrokeWidth(settings_.underlayer_width);
    underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND).SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc.Add(underlayer);
    doc.Add(stop_name);
    return doc;
}

svg::Document &MapRenderer::Render(svg::Document &doc, std::vector<domain::BusPtr> &buses_to_render, const std::vector<domain::StopPtr> &stops_to_render)
{
    std::vector<geo::Coordinates> all_stops_coordinates;
    all_stops_coordinates.reserve(stops_to_render.size());
    for (const auto &stop : stops_to_render)
    {
        all_stops_coordinates.push_back(stop->GetCoordinates());
    }

    CalibrateMap(all_stops_coordinates);

    for (const auto &bus_ptr : buses_to_render)
    {
        PrintRoad(bus_ptr, doc);
    }

    for (const auto &bus_ptr : buses_to_render)
    {
        PrintBusName(bus_ptr, doc);
    }
    for (const auto &stop_ptr : stops_to_render)
    {
        PrintStopSymbol(stop_ptr, doc);
    }
    for (const auto &stop_ptr : stops_to_render)
    {
        PrintStopName(stop_ptr, doc);
    }
    return doc;
}

void MapRenderer::CalibrateMap(const std::vector<geo::Coordinates> &all_stops_coordinates)
{
    projector_ = SphereProjector(all_stops_coordinates.begin(), all_stops_coordinates.end(), settings_.width, settings_.height, settings_.padding);
}

} //namespace renderer
} //namespace transport_catalogue
