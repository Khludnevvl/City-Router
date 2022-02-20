#pragma once

#include "SvgLib/svg.h"
#include "geo.h"
#include "transport_catalogue.h"

#include <deque>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace transport_catalogue
{
namespace renderer
{
inline const double EPSILON = 1e-6;

inline bool IsZero(double value)
{
    return std::abs(value) < EPSILON;
}

class SphereProjector
{
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                    double max_height, double padding)
        : padding_(padding)
    {
        if (points_begin == points_end)
        {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
                                                             { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs)
                                                             { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_))
        {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat))
        {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom)
        {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom)
        {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom)
        {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const
    {
        return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
    }

protected:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

struct RendererSettings
{
    void AddColor(svg::Color color_)
    {
        color_palette.push_back(std::move(color_));
    }

    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;
    int bus_label_font_size = 0;
    svg::Point bus_label_offset;
    int stop_label_font_size = 0;
    svg::Point stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width = 0;
    std::deque<svg::Color> color_palette;
};

class MapRenderer
{
public:
    MapRenderer() = default;

    MapRenderer(const RendererSettings &settings)
        : settings_(settings) {}

    void SetSettings(RendererSettings settings)
    {
        settings_ = std::move(settings);
    }

    svg::Document &PrintBusName(const domain::BusPtr bus, svg::Document &doc) const;
    svg::Document &PrintRoad(const domain::BusPtr bus, svg::Document &doc);
    svg::Document &PrintStopSymbol(const domain::StopPtr stop, svg::Document &doc) const;
    svg::Document &PrintStopName(const domain::StopPtr stop, svg::Document &doc) const;

    svg::Document &Render(svg::Document &doc, std::vector<domain::BusPtr> &buses_to_render, const std::vector<domain::StopPtr> &stop_to_render);

private:
    void CalibrateMap(const std::vector<geo::Coordinates> &all_stops_coordinates);

    RendererSettings settings_;
    std::optional<SphereProjector> projector_;
    size_t current_color_index = 0;
    std::map<const domain::BusPtr, svg::Color> bus_to_color;
};
} // namespace renderer
} // namespace transport_catalogue