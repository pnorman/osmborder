#ifndef OUTPUTTER_HPP
#define OUTPUTTER_HPP
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
#include <osmium/osm/way.hpp>
/**
 * Takes the information for one way and outputs it to a file
 */
class Outputter
{
public:
    virtual void output_line(const osmium::Way &way, int admin_level,
                             bool dividing_line, bool disputed,
                             bool maritime) = 0;
};

#endif // OUTPUTTER_HPP