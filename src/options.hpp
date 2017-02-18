#ifndef OPTIONS_HPP
#define OPTIONS_HPP

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

#include <string>

/**
 * This class encapsulates the command line parsing.
 */
class Options
{

public:
    /// Input OSM file name.
    std::string inputfile;

    /// Show debug output?
    bool debug;

    /// Output file name.
    std::string csv_output_file;

    /// Should output database be overwritten
    bool overwrite_output;

    /// EPSG code of output SRS.
    int epsg;

    /// Verbose output?
    bool verbose;

    Options(int argc, char *argv[]);

private:
    /**
     * Get EPSG code from text. This method knows about a few common cases
     * of specifying WGS84 or the "Google mercator" SRS. More are currently
     * not supported.
     */
    int get_epsg(const char *text);

    void print_help() const;

}; // class Options

#endif // OPTIONS_HPP
