#pragma once

#include "geo.h"

#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace transport_catalogue
{
namespace domain
{

class Stop;
class Bus;
using BusPtr = Bus *;
using StopPtr = Stop *;

class Bus
{
public:
    Bus() = default;
    explicit Bus(const std::string &bus_name, bool is_circle_ = false)
        : name_(bus_name), is_circle_(is_circle_)
    {
    }
    explicit Bus(const std::string &bus_name, std::vector<Stop *> stops_vec, bool is_circle_ = false)
        : name_(bus_name), stops_(stops_vec), is_circle_(is_circle_)
    {
    }

    void AddStop(StopPtr stop)
    {
        stops_.push_back(stop);
    }

    const std::string &GetName() const
    {
        return name_;
    }

    const std::vector<Stop *> GetStops() const
    {
        return stops_;
    }
    bool IsCircle() const
    {
        return is_circle_;
    }

    size_t CalculateUniqueStops() const
    {
        std::unordered_set<Stop *> unique(stops_.begin(), stops_.end());
        return unique.size();
    }

private:
    std::string name_;
    std::vector<Stop *> stops_;
    bool is_circle_ = false;
};

double CalculateGeographicalDistance(const BusPtr);

class Stop
{
public:
    Stop() = default;
    explicit Stop(std::string stop_name, geo::Coordinates coordinates)
        : name_(std::move(stop_name)), coordinates_(coordinates)
    {
    }

    const std::string &GetName() const
    {
        return name_;
    }

    geo::Coordinates GetCoordinates() const
    {
        return coordinates_;
    }

private:
    std::string name_;
    geo::Coordinates coordinates_;
};
} // namespace domain
} //transport_catalogue