/*

  Copyright 2012-2016 Jochen Topf <jochen@topf.org>.

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

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef _MSC_VER
# include <unistd.h>
#else
# include <io.h>
#endif

#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/visitor.hpp>
#include <osmium/area/assembler.hpp>

#include "return_codes.hpp"

#include "options.hpp"
#include "stats.hpp"
#include "util.hpp"
#include "admin_collector.hpp"

// Global debug marker
bool debug;

// If there are more than this many warnings, the program exit code will indicate an error.
const unsigned int max_warnings = 500;

/* ================================================== */

std::string memory_usage() {
    osmium::MemoryUsage mem;
    std::ostringstream s;
    s << "Memory used: current: " << mem.current() << " MBytes, peak: " << mem.peak() << " MBytes\n";
    return s.str();
}

/* ================================================== */

class AdminHandler : public osmium::handler::Handler {
public:
    void node(const osmium::Node& node) {
        std::cout << 'n' << node.id() << "\n";
    }

    void way(const osmium::Way& way) {
        std::cout << 'w' << way.id() << "\n";
    }

    void area(const osmium::Area& area) {
        std::cout << 'a' << area.id() << "\n";
    }

    void relation(const osmium::Relation& relation) {
        std::cout << 'r' << relation.id() << "\n";
    }


};
int main(int argc, char *argv[]) {
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

    osmium::io::File infile{argv[optind]};

    osmium::area::Assembler::config_type assembler_config;
    AdminCollector<osmium::area::Assembler> collector{};

    vout << "Reading relations in pass 1.\n";
    osmium::io::Reader reader1(infile, osmium::osm_entity_bits::relation);
    collector.read_relations(reader1);

    vout << memory_usage();

    vout << "Reading pass 2.\n";
    osmium::io::Reader reader2(infile, osmium::osm_entity_bits::way);

    AdminHandler admin_handler;

    osmium::apply(reader2, collector.handler([&admin_handler](osmium::memory::Buffer&& buffer) {
        osmium::apply(buffer, admin_handler);}));

    vout << "All done.\n";
    vout << memory_usage();

    std::cout << "There were " << warnings << " warnings.\n";
    std::cout << "There were " << errors << " errors.\n";

    if (errors || warnings > max_warnings) {
        return return_code_error;
    } else if (warnings) {
        return return_code_warning;
    } else {
        return return_code_ok;
    }
}

