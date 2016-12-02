#ifndef ADMINHANDLER_HPP
#define ADMINHANDLER_HPP
/*

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

class AdminHandler : public osmium::handler::Handler {
private:
    // p1
    // All relations we are interested in will be kept in this buffer
    osmium::memory::Buffer m_relations_buffer;
    // Mapping of way IDs to the offset to the parent relation
    typedef std::vector<size_t> RelationParents;
    typedef std::map<osmium::unsigned_object_id_type, RelationParents> WayRelations;
    WayRelations m_way_rels;

    // p2
    // All ways we're interested in
    osmium::memory::Buffer m_ways_buffer;

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
    /**
     * This handler operates on the ways-only pass and extracts way information, but can't
     * yet do geometries since it doesn't have node information
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

    /* This is where the logic that handles tagging lives, getting tags from the way and parent rels */
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
            for (const auto &rm : relation.members()) {
                if (rm.type() == osmium::item_type::way) {
                    // Calls the default constructor if it doesn't exist, otherwise add to the back
                    // TODO: Can this call the constructor to create a vector of size N, where N=2?
                    m_way_rels[rm.ref()].push_back(relation_offset);
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
#endif // ADMINHANDLER_HPP