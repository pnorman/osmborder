
# OSMBorder

OSMBorder extracts the admin boundary data from an OSM planet file and assembles
all the pieces into linestrings for use in map renderers etc.

## Prerequisites

### Libosmium

    https://github.com/osmcode/libosmium
    http://osmcode.org/libosmium
    At least version 2.5.0 is needed.

### zlib (for PBF support)

    http://www.zlib.net/
    Debian/Ubuntu: zlib1g-dev

### Pandoc (optional, to build documentation)

    http://johnmacfarlane.net/pandoc/
    Debian/Ubuntu: pandoc
    (If pandoc is found by CMake, the manpages will automatically be built.)


## Building

You'll need the prerequisites including `libosmium` installed.

OSMBorder uses CMake for building:

    mkdir build
    cd build
    cmake ..
    make

Call `make doc` to build the Doxygen API documentation which will be available
in the `doc/html` directory.


## Testing

## Running


## Output

## Steps

## Options

    -v, --verbose

Gives you detailed information on what osmcoastline is doing, including timing.

Run `osmborder --help` to see all options.

## License

OSMCoastline is available under the GNU GPL version 3 or later.

## Authors

Paul Norman (penorman@mac.com)
Based on OSMCOastline by Jochen Topf (jochen@topf.org)
