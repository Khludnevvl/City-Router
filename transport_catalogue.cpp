#include "transport_catalogue.h"
#include "geo.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace transport_catalogue
{
using namespace domain;
void TransportCatalogue::AddStop(std::string stop_name, geo::Coordinates coordinates)
{
    all_stops_.push_back(Stop(stop_name, coordinates));
    stop_name_to_stop_[all_stops_.back().GetName()] = &all_stops_.back();
}

void TransportCatalogue::AddStopsDistance(const std::string &from_stop, const std::string &to_stop, int distance)
{
    const std::pair<const StopPtr, const StopPtr> stop_pair(GetStop(from_stop), GetStop(to_stop));
    distance_between_stops_[stop_pair] = distance;
}

StopPtr TransportCatalogue::GetStop(std::string_view stop) const
{
    const auto iter = stop_name_to_stop_.find(stop);
    if (iter == stop_name_to_stop_.end())
    {
        return nullptr;
    }

    return iter->second;
}

BusPtr TransportCatalogue::GetBus(std::string_view bus) const
{
    auto iter = bus_name_to_bus_.find(bus);
    if (iter == bus_name_to_bus_.end())
    {
        return nullptr;
    }

    return iter->second;
}

const std::unordered_map<Stop *, std::set<std::string_view>> &TransportCatalogue::GetAllStopToBuses() const
{
    return stop_to_buses_;
}

const std::optional<std::set<std::string_view>> TransportCatalogue::GetBusesThroughStop(std::string_view stop_name) const
{
    domain::StopPtr stop = GetStop(stop_name);
    if (stop == nullptr)
    {
        return std::nullopt;
    }
    const auto &stop_to_buses = GetAllStopToBuses();
    auto iter = stop_to_buses.find(stop);

    if (iter == stop_to_buses.end())
    {
        const static std::set<std::string_view> empty_set;
        return empty_set;
    }
    return iter->second;
}

int TransportCatalogue::CalculateRoute(const BusPtr bus) const
{
    auto stops = bus->GetStops();
    if (stops.empty())
    {
        return 0;
    }
    double result = 0;

    for (size_t i = 0; i < stops.size() - 1; i++)
    {
        result = result + GetDistanceBetweenStops(stops[i], stops[i + 1]);
    }
    return result;
}
int TransportCatalogue::GetDistanceBetweenStops(std::string_view from, std::string_view to) const
{
    return GetDistanceBetweenStops(GetStop(from), GetStop(to));
}

int TransportCatalogue::GetDistanceBetweenStops(const StopPtr from, const StopPtr to) const
{

    const auto iter_from_to = distance_between_stops_.find({from, to});
    if (iter_from_to != distance_between_stops_.end())
    {
        return iter_from_to->second;
    }

    const auto iter_to_from = distance_between_stops_.find({to, from});
    if (iter_to_from != distance_between_stops_.end())
    {
        return iter_to_from->second;
    }
    return 0;
}

size_t TransportCatalogue::GetUsingStopsCount() const
{
    return stop_to_buses_.size();
}

std::set<std::string_view> TransportCatalogue::GetAllUsingStops() const
{
    return using_stops_;
}

std::deque<domain::Bus> TransportCatalogue::GetAllBuses() const
{
    return all_buses_;
}

} //namespace transport_catalogue