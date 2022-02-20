#pragma once

#include "domain.h"

#include <algorithm>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace transport_catalogue
{

class TransportCatalogue
{
public:
    template <typename Container>
    void AddBus(std::string bus_name, Container stops, bool is_circle = false)
    {
        domain::Bus tmp_bus(bus_name, is_circle);
        all_buses_.emplace_back(std::move(tmp_bus));
        domain::BusPtr current_bus = &all_buses_.back();
        bus_name_to_bus_[current_bus->GetName()] = current_bus;
        all_buses_names_.insert(current_bus->GetName());
        for (std::string_view stop_name : stops)
        {
            if (stop_name_to_stop_.count(stop_name))
            {
                stop_to_buses_[stop_name_to_stop_.at(stop_name)].insert(current_bus->GetName());
                auto stop = stop_name_to_stop_.at(stop_name);
                using_stops_.insert(stop->GetName());

                current_bus->AddStop(stop);
            }
        }
        if (!is_circle)
        {
            for (int i = stops.size() - 2; i >= 0; i--)
            {
                auto stop = stop_name_to_stop_.at(stops[i]);
                using_stops_.insert(stop->GetName());
                current_bus->AddStop(stop);
            }
        }
    }

    void AddStop(std::string stop_name, geo::Coordinates coordinates);

    void AddStopsDistance(const std::string &from_stop, const std::string &to_stop, int distance);

    //get Stop by stop_name
    domain::StopPtr GetStop(std::string_view stop_name) const;

    //get bus by bus_name
    domain::BusPtr GetBus(std::string_view bus_name) const;

    const std::set<std::string_view> &GetAllBusesNames() const
    {
        return all_buses_names_;
    }

    //return std::set buses passing through the stop_name
    const std::set<std::string_view> &GetStopInfo(std::string_view stop_name) const;

    //return std::set buses passing through the Stop
    const std::set<std::string_view> &GetStopInfo(domain::Stop *stop) const;

    const std::unordered_map<domain::Stop *, std::set<std::string_view>> &GetAllStopToBuses() const;

    const std::map<std::string_view, domain::StopPtr> &GetAllStops() const
    {
        return stop_name_to_stop_;
    }

    //calculate all distances between stops in bus route
    int CalculateRoute(const domain::BusPtr bus) const;

    const std::optional<std::set<std::string_view>> GetBusesThroughStop(std::string_view stop_name) const;

    size_t GetUsingStopsCount() const;

    std::deque<domain::Bus> GetAllBuses() const;

    int GetDistanceBetweenStops(std::string_view from, std::string_view to) const;

    std::set<std::string_view> GetAllUsingStops() const;

private:
    int GetDistanceBetweenStops(const domain::StopPtr from, const domain::StopPtr to) const;

    struct PairHasher
    {
        size_t operator()(const std::pair<const domain::StopPtr, const domain::StopPtr> pair_stops) const
        {
            size_t first_stop_hash = pointer_hasher_(pair_stops.first);
            size_t second_stop_hash = pointer_hasher_(pair_stops.second);
            return first_stop_hash * 37 + second_stop_hash * (37 * 37);
        }

    private:
        std::hash<const void *> pointer_hasher_;
    };

    std::deque<domain::Bus> all_buses_;
    std::deque<domain::Stop> all_stops_;
    std::set<std::string_view> all_buses_names_;
    std::set<std::string_view> using_stops_;
    std::map<std::string_view, domain::StopPtr> stop_name_to_stop_;
    std::unordered_map<std::string_view, domain::BusPtr> bus_name_to_bus_;
    std::unordered_map<domain::Stop *, std::set<std::string_view>> stop_to_buses_;
    std::unordered_map<std::pair<const domain::StopPtr, const domain::StopPtr>, int, PairHasher> distance_between_stops_;
};
} //namespace transport_catalogue