#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace std::literals;


void PrintUsageMakeBase(std::ostream &stream = std::cerr) 
{
    stream << "Usage: make_base <input_file>\n"sv;
}

void PrintUsageProcessRequest(std::ostream &stream = std::cerr) 
{
    stream << "Usage: process_requests <input_file> <output_file>\n"sv;
}

void PrintUsage(std::ostream &stream = std::cerr)
{
    stream << "Usage: transport_catalogue [ make_base <input_file> ] | [ process_requests <input_file> <output_file> ] \n"sv;
}

int main(int argc, char *argv[])
{

    PrintUsage();

    if (argc == 1 || argc > 4)
    {
        PrintUsage();
        return 1;
    }
    transport_catalogue::TransportCatalogue catalogue;
    transport_catalogue::renderer::MapRenderer render;
    transport_catalogue::request_handler::RequestHandler handler(catalogue, render);
    transport_catalogue::json_reader::JsonReader reader(catalogue, handler, render);

    const std::string_view mode(argv[1]);
    if (mode == "make_base"sv)
    {
        if (argc != 3)
        {
            PrintUsageMakeBase();
            return 1;
        }

        std::ifstream input(argv[2], std::ios::binary);
        if (!input.is_open())
        {
            std::cerr << "Failed to open file \n" << argv[2];
            return 2;
        }
        reader.ReadMakeBaseRequest(input);
        reader.SerializeCatalogue();
        return 0;
    }
    else if (mode == "process_requests"sv)
    {
        if (argc != 4)
        {
            PrintUsageProcessRequest();
            return 1;
        }

        std::ifstream input(argv[2], std::ios::binary);
        if (!input.is_open())
        {
            std::cerr << "Failed to open file " << argv[2] << std::endl;
            return 2;
        }
        std::ofstream output(argv[3], std::ios::binary);
        if (!output.is_open())
        {
            std::cerr << "Failed to open file " << argv[3] << std::endl;
            return 2;
        }
        reader.ReadStatRequest(input);
        reader.DeserializeCatalogue();
        reader.OutputRequest(output);
        return 0;
    }
    else
    {
        PrintUsage();
        return 1;
    }
}
