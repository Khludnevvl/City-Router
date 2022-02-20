#include "domain.h"
#include "geo.h"

namespace transport_catalogue
{
namespace domain
{

double CalculateGeographicalDistance(const BusPtr bus)
{
    auto stops = bus->GetStops();
    if (stops.empty())
    {
        return 0;
    }
    double result = 0;
    for (size_t i = 0; i < stops.size() - 1; i++)
    {
        result = result + geo::ComputeClosestDistance(stops[i]->GetCoordinates(), stops[i + 1]->GetCoordinates());
    }
    return result;
}

} //namespace domain
} // namespace transport_catalogue