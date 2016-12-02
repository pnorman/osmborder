
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
1. Filter the planet with osmborder_filter
  ```sh
  osmborder_filter -o filtered.osm.pbf planet-latest.osm.pbf
  ```
2. Create linestrings with osmborder
```sh
osmborder -o osmborder_lines.csv filtered.osm.pbf
```

## Output
OSMBorder outputs a tab-delimited file that can be loaded directly into PostgreSQL. This requires a suitable table, which can be created, loaded, optimized, and indexed with

```sql
CREATE TABLE osmborder_lines (
  osm_id bigint,
  admin_level int,
  disputed bool,
  maritime bool,
  way Geometry(LineString, 4326));
\copy osmborder_lines FROM osmborder_lines.csv

CREATE INDEX osmborder_lines_way_idx ON osmborder_lines USING gist (way) WITH (fillfactor=100);
CLUSTER osmborder_lines USING osmborder_lines_way_idx;
CREATE INDEX osmborder_lines_way_low_idx ON osmborder_lines USING gist (way) WITH (fillfactor=100) WHERE admin_level <= 4;
```

The indexes are optional, but useful if rendering maps.

## Steps

## Options

    -v, --verbose

Gives you detailed information on what osmborder is doing, including timing.

Run `osmborder --help` to see all options.

## License

OSMBorder is available under the GNU GPL version 3 or later.

## Authors

Paul Norman (penorman@mac.com)  
Based on OSMCOastline by Jochen Topf (jochen@topf.org)
