/*

  Copyright 2012-2016 Jochen Topf <jochen@topf.org>.
  Copyright 2016 Paul Norman <penorman@mac.com>.

  This file is part of OSMBorder.

  OSMBorder is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OSMBorder is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OSMBorder.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <algorithm>
#include <iostream>
#include <string>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif

#include <osmium/geom/wkb.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/index/map/all.hpp>
#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>

namespace osmium {
class Area;
}
namespace osmium {
class Node;
}
namespace osmium {
class Relation;
}
namespace osmium {
class Way;
}

#include "adminhandler.hpp"
#include "options.hpp"
#include "outputter-csv.hpp"
#include "return_codes.hpp"
#include "stats.hpp"

// Global debug marker
bool debug;

// If there are more than this many warnings, the program exit code will indicate an error.
const unsigned int max_warnings = 500;

/* ================================================== */

std::string memory_usage()
{
    osmium::MemoryUsage mem;
    std::ostringstream s;
    s << "Memory used: current: " << mem.current()
      << " MBytes, peak: " << mem.peak() << " MBytes\n";
    return s.str();
}

/* ================================================== */

// This class acts like NodeLocationsForWays but only stores specific nodes
// Also, only positive. TODO: Add in negative support
template <typename TStoragePosIDs>
class SpecificNodeLocationsForWays
    : public osmium::handler::NodeLocationsForWays<TStoragePosIDs>
{

    // some var for a set of IDs to keep
public:
    explicit SpecificNodeLocationsForWays(TStoragePosIDs &storage_pos)
    : osmium::handler::NodeLocationsForWays<TStoragePosIDs>(storage_pos)
    {
    }

    void node(const osmium::Node &node)
    {
        if (true) {
            osmium::handler::NodeLocationsForWays<TStoragePosIDs>::node(node);
        }
    }
    void way(osmium::Way &way)
    {
        osmium::handler::NodeLocationsForWays<TStoragePosIDs>::way(way);
    }
};

// TODO: Cover all admin_levels
// This is a map instead of something like an array of chars because admin_levels can extend past 9
const std::map<std::string, const int> AdminHandler::admin_levels = {
    {"2", 2}, {"3", 3}, {"4", 4},   {"5", 5},   {"6", 6},  {"7", 7},
    {"8", 8}, {"9", 9}, {"10", 10}, {"11", 11}, {"12", 12}};

int main(int argc, char *argv[])
{
    Stats stats;
    unsigned int warnings = 0;
    unsigned int errors = 0;

    // Parse command line and setup 'options' object with them.
    Options options(argc, argv);

    // The vout object is an output stream we can write to instead of
    // std::cerr. Nothing is written if we are not in verbose mode.
    // The running time will be prepended to output lines.
    osmium::util::VerboseOutput vout(options.verbose);

    debug = options.debug;

    vout << "Writing to file '" << options.output_file << "'.\n";

    std::ofstream output(options.output_file);

    osmium::io::File infile{argv[optind]};

    std::vector<std::shared_ptr<Outputter>> outputters;
    outputters.push_back(std::make_shared<CsvOutputter>(output));

    AdminHandler admin_handler(outputters);

    {
        vout << "Reading relations in pass 1.\n";
        osmium::io::Reader reader(infile, osmium::osm_entity_bits::relation);
        osmium::apply(reader, admin_handler);
        reader.close();
        vout << memory_usage();
    }
    {
        vout << "Reading ways pass 2.\n";
        osmium::io::Reader reader(infile, osmium::osm_entity_bits::way);
        osmium::apply(reader, admin_handler.m_handler_pass2);
        reader.close();
        vout << memory_usage();
    }
    typedef osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type,
                                                osmium::Location>
        index_type;
    typedef SpecificNodeLocationsForWays<index_type> location_handler_type;
    index_type index;
    location_handler_type location_handler{index};
    {
        vout << "Reading nodes pass 3.\n";
        osmium::io::Reader reader(infile, osmium::osm_entity_bits::node);
        osmium::apply(reader, location_handler);
        reader.close();
        vout << memory_usage();
    }
    vout << "Building linestrings.\n";
    osmium::io::Reader reader(infile, osmium::osm_entity_bits::way);
    osmium::apply(reader, location_handler, admin_handler);

    vout << "All done.\n";
    vout << memory_usage();

    std::cerr << "There were " << warnings << " warnings.\n";
    std::cerr << "There were " << errors << " errors.\n";

    if (errors || warnings > max_warnings) {
        return return_code_error;
    } else if (warnings) {
        return return_code_warning;
    } else {
        return return_code_ok;
    }
}
