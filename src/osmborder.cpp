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

#include <iostream>
#include <string>
#include <algorithm>

#ifndef _MSC_VER
# include <unistd.h>
#else
# include <io.h>
#endif

#include <osmium/io/any_input.hpp>
#include <osmium/io/file.hpp>
#include <osmium/util/memory.hpp>
#include <osmium/util/verbose_output.hpp>
#include <osmium/handler.hpp>
#include <osmium/handler/node_locations_for_ways.hpp>
#include <osmium/visitor.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/index/map/all.hpp>
#include <osmium/geom/wkb.hpp>

namespace osmium { class Area; }
namespace osmium { class Node; }
namespace osmium { class Relation; }
namespace osmium { class Way; }

#include "return_codes.hpp"

#include "options.hpp"
#include "stats.hpp"

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

// This class acts like NodeLocationsForWays but only stores specific nodes
// Also, only positive. TODO: Add in negative support
template <typename TStoragePosIDs>
class SpecificNodeLocationsForWays : public osmium::handler::NodeLocationsForWays<TStoragePosIDs> {

    // some var for a set of IDs to keep
public:
    explicit SpecificNodeLocationsForWays(TStoragePosIDs& storage_pos) : osmium::handler::NodeLocationsForWays<TStoragePosIDs>(storage_pos) { }

    void node(const osmium::Node& node) {
        if (true) {
            osmium::handler::NodeLocationsForWays<TStoragePosIDs>::node(node);
        }
    }
    void way (osmium::Way& way) {
        osmium::handler::NodeLocationsForWays<TStoragePosIDs>::way(way);
    }
};


class AdminHandler : public osmium::handler::Handler {
// should be private
private:
    // p1
    // All relations we are interested in will be kept in this buffer
    osmium::memory::Buffer m_relations_buffer;
    // Mapping of way IDs to the offset to the parent relation
    typedef std::vector<size_t> RelationParents;
    typedef std::map<osmium::unsigned_object_id_type, RelationParents> WayRelations;
    WayRelations m_way_rels;


    osmium::geom::WKBFactory<> m_factory{osmium::geom::wkb_type::ewkb, osmium::geom::out_type::hex};
    static constexpr size_t initial_buffer_size = 1024 * 1024;

    static const std::map<std::string, const int> admin_levels;
    // Based on osm2pgsql escaping
    std::string escape(const std::string& src) {
        std::string dst;
        for (const char c: src) {
            switch(c) {
                case '\\':  dst.append("\\\\"); break;
                    //case 8:   dst.append("\\\b"); break;
                    //case 12:  dst.append("\\\f"); break;
                case '\n':  dst.append("\\\n"); break;
                case '\r':  dst.append("\\\r"); break;
                case '\t':  dst.append("\\\t"); break;
                    //case 11:  dst.append("\\\v"); break;
                default:    dst.push_back(c); break;
            }
        }
        return dst;
    }

    std::ostream& m_out;
public:
    // p2
    // All ways we're interested in
    osmium::memory::Buffer m_ways_buffer;

    /**
     * This handler operates on the ways-only pass and extracts way information, but can't
     * yet do geometries
     */
    class HandlerPass2 : public osmium::handler::Handler {
    public:
        osmium::memory::Buffer& m_ways_buffer;
        const WayRelations& m_way_rels;

        explicit HandlerPass2(osmium::memory::Buffer& ways_buffer, const WayRelations& way_rels) :
            m_ways_buffer(ways_buffer),
            m_way_rels(way_rels){
        }

        void way (const osmium::Way& way) {
            if (m_way_rels.count(way.id()) > 0){
                m_ways_buffer.add_item(way);
            }
        }
    };
    AdminHandler(std::ostream& out) :
        m_ways_buffer(initial_buffer_size, osmium::memory::Buffer::auto_grow::yes),
        m_relations_buffer(initial_buffer_size, osmium::memory::Buffer::auto_grow::yes),
        m_handler_pass2(m_ways_buffer, m_way_rels),
        m_out(out) {
    }

   void way(const osmium::Way& way) {
       std::vector<int> parent_admin_levels;
       bool disputed = false;
       bool maritime = false;

       // Tags on the way itself
       disputed = disputed || way.tags().has_tag("disputed","yes");
       disputed = disputed || way.tags().has_tag("dispute","yes");
       disputed = disputed || way.tags().has_tag("border_status","dispute");

       maritime = maritime || way.tags().has_tag("maritime","yes");
       maritime = maritime || way.tags().has_tag("natural","coastline");

       // Tags on the parent relations
       for (const auto& rel_offset : m_way_rels[way.id()]) {
           const osmium::TagList& tags = m_relations_buffer.get<const osmium::Relation>(rel_offset).tags();
           const char * admin_level = tags.get_value_by_key("admin_level", "");
           /* can't use admin_levels[] because [] is non-const, but there must be a better way? */
           auto admin_it = admin_levels.find(admin_level);
           if (admin_it != admin_levels.end()) {
               parent_admin_levels.push_back(admin_it->second);
           }
       }

       if (parent_admin_levels.size() > 0) {
           try {
               m_out << way.id()
                     // parent_admin_levels is already escaped.
                     << "\t" << *std::min_element(parent_admin_levels.begin(), parent_admin_levels.end())
                       << "\t" << ((disputed) ? ("true") : ("false"))
                       << "\t" << ((maritime) ? ("true") : ("false"))
                     << "\t" << m_factory.create_linestring(way) << "\n";
           } catch (osmium::geometry_error& e) {
               std::cerr << "Geometry error on way " << way.id() << ": " << e.what() << "\n";
           }
       }
   }

    void relation(const osmium::Relation& relation) {
        if (relation.tags().has_tag("boundary", "administrative")) {
            m_relations_buffer.add_item(relation);
            auto relation_offset = m_relations_buffer.commit();
/*            std::cout << 'r' << relation.id() << " at ro " << relation_offset;
            if (relation.tags().has_key("name")) {
                std::cout << " name=" << relation.tags().get_value_by_key("name");
            }
            std::cout << '\n';*/
            for (const auto &rm : relation.members()) {
                if (rm.type() == osmium::item_type::way) {
                    // Calls the default constructor if it doesn't exist, otherwise add to the back
                    // TODO: Can this call the constructor to create a vector of size N, where N=2?
                    m_way_rels[rm.ref()].push_back(relation_offset);
                    //std::cout << "rm " << rm.ref() << "\n";
                }
            }
        }
    }

    osmium::memory::Buffer& get_ways() {
        return m_ways_buffer;
    }

    void flush() {
    }
    // Handler for the pass2 ways
    HandlerPass2 m_handler_pass2;

};

// TODO: Cover all admin_levels
// This is a map instead of something like an array of chars because admin_levels can extend past 9
const std::map<std::string, const int> AdminHandler::admin_levels = {{"2", 2}, {"3", 3}, {"4", 4}, {"5", 5}, {"6", 6}, {"7", 7}, {"8", 8}};


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

    vout << "Writing to file '" << options.output_file << "'.\n";

    std::ofstream output (options.output_file);

    osmium::io::File infile{argv[optind]};

    AdminHandler admin_handler(output);

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
    typedef osmium::index::map::SparseMmapArray<osmium::unsigned_object_id_type, osmium::Location> index_type;
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

