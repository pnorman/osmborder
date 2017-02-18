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

#include "outputter-csv.hpp"

CsvOutputter::CsvOutputter(const char *output_file) : m_out(output_file) {}
void CsvOutputter::output_line(const osmium::Way &way, int admin_level,
                               bool dividing_line, bool disputed, bool maritime)
{
    m_out << way.id() << "\t"
          // parent_admin_levels is already escaped.
          << std::to_string(admin_level) << "\t"
          << ((dividing_line) ? ("true") : ("false")) << "\t"
          << ((disputed) ? ("true") : ("false")) << "\t"
          << ((maritime) ? ("true") : ("false")) << "\t"
          << m_factory.create_linestring(way) << "\n";
}