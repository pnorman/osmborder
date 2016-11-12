/*

  Copyright 2012-2016 Jochen Topf <jochen@topf.org>.
  Copyright 2016 Paul Norman <penorman@mac.com>.

  This file is part of OSMBorder, and is based on osmborder_filter.cpp
  from OSMCoastline

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
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <iostream>
#include <string>
#include <vector>

#include <osmium/io/any_input.hpp>
#include <osmium/io/error.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/header.hpp>
#include <osmium/io/input_iterator.hpp>
#include <osmium/io/output_iterator.hpp>
#include <osmium/io/pbf_output.hpp>
#include <osmium/osm.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/entity_bits.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>

#include "return_codes.hpp"

void print_help() {
    std::cout << "osmborder_filter [OPTIONS] OSMFILE\n"
              << "\nOptions:\n"
              << "  -h, --help           - This help message\n"
              << "  -o, --output=OSMFILE - Where to write output (default: none)\n"
              << "  -v, --verbose        - Verbose output\n"
              << "  -V, --version        - Show version and exit\n"
              << "\n";
}

int main(int argc, char* argv[]) {
    std::string output_filename;
    bool verbose = false;

    static struct option long_options[] = {
        {"help",         no_argument, 0, 'h'},
        {"output", required_argument, 0, 'o'},
        {"verbose",      no_argument, 0, 'v'},
        {"version",      no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    while (1) {
        int c = getopt_long(argc, argv, "ho:vV", long_options, 0);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                print_help();
                std::exit(return_code_ok);
            case 'o':
                output_filename = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            case 'V':
                std::cout << "osmborder_filter version " OSMBORDER_VERSION "\n"
                          << "Copyright (C) 2012-2016  Jochen Topf <jochen@topf.org>\n"
                          << "License: GNU GENERAL PUBLIC LICENSE Version 3 <http://gnu.org/licenses/gpl.html>.\n"
                          << "This is free software: you are free to change and redistribute it.\n"
                          << "There is NO WARRANTY, to the extent permitted by law.\n";
                std::exit(return_code_ok);
            default:
                std::exit(return_code_fatal);
        }
    }

    // The vout object is an output stream we can write to instead of
    // std::cerr. Nothing is written if we are not in verbose mode.
    // The running time will be prepended to output lines.
    osmium::util::VerboseOutput vout(verbose);

    if (output_filename.empty()) {
        std::cerr << "Missing -o/--output=OSMFILE option\n";
        std::exit(return_code_cmdline);
    }

    if (optind != argc - 1) {
        std::cerr << "Usage: osmborder_filter [OPTIONS] OSMFILE\n";
        std::exit(return_code_cmdline);
    }

    osmium::io::Header header;
    header.set("generator", "osmborder_filter");
    header.add_box(osmium::Box{-180.0, -90.0, 180.0, 90.0});

    osmium::io::File infile{argv[optind]};

    try {
        osmium::io::Writer writer{output_filename, header};
        auto output_it = osmium::io::make_output_iterator(writer);

        std::vector<osmium::object_id_type> way_ids;
        std::vector<osmium::object_id_type> node_ids;

        vout << "Reading relations (1st pass through input file)...\n";
        {
            osmium::io::Reader reader{infile, osmium::osm_entity_bits::relation};
            auto relations = osmium::io::make_input_iterator_range<const osmium::Relation>(reader);
            for (const osmium::Relation& relation : relations) {
                if (relation.tags().has_tag("boundary", "administrative")) {
                    *output_it++ = relation;
                    for (const auto& rm : relation.members()) {
                        if (rm.type() == osmium::item_type::way) {
                            way_ids.push_back(rm.ref());
                        }
                    }
                }
            }
            reader.close();
        }

        vout << "Preparing way ID list...\n";
        std::sort(way_ids.begin(), way_ids.end());

        vout << "Reading ways (2nd pass through input file)...\n";

        {
            osmium::io::Reader reader{infile, osmium::osm_entity_bits::way};
            auto ways = osmium::io::make_input_iterator_range<const osmium::Way>(reader);

            auto first = way_ids.begin();
            auto last = std::unique(way_ids.begin(), way_ids.end());

            for (const osmium::Way& way : ways) {
                // Advance the target list to the first possible way
                while (*first < way.id() && first != last) {
                    ++first;
                }

                if (way.id() == *first) {
                    *output_it++ = way;
                    for (const auto& nr : way.nodes()) {
                        node_ids.push_back(nr.ref());
                    }
                    if (first != last) {
                        ++first;
                    }
                }
            }

        }

        vout << "Preparing node ID list...\n";
        std::sort(node_ids.begin(), node_ids.end());
        auto last = std::unique(node_ids.begin(), node_ids.end());

        vout << "Reading nodes (3rd pass through input file)...\n";
        {
            osmium::io::Reader reader{infile, osmium::osm_entity_bits::node};
            auto nodes = osmium::io::make_input_iterator_range<const osmium::Node>(reader);

            auto first = node_ids.begin();
            std::copy_if(nodes.cbegin(), nodes.cend(), output_it, [&first, &last](const osmium::Node& node){
                while (*first < node.id() && first != last) {
                    ++first;
                }

                if (node.id() == *first) {
                    if (first != last) {
                        ++first;
                    }
                    return true;
                }
            });

            reader.close();
        }

        writer.close();
    } catch (const osmium::io_error& e) {
        std::cerr << "io error: " << e.what() << "'\n";
        std::exit(return_code_fatal);
    }

    vout << "All done.\n";
    osmium::MemoryUsage mem;
    if (mem.current() > 0) {
        vout << "Memory used: current: " << mem.current() << " MBytes\n"
             << "             peak:    " << mem.peak() << " MBytes\n";
    }
}

