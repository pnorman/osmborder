
# OSMBorder

OSMBorder extracts the admin boundary data from an OSM planet file and assembles
all the pieces into linestrings for use in map renderers etc.

## Prerequisites

### Libosmium

    https://github.com/osmcode/libosmium
    http://osmcode.org/libosmium
    At least version 2.7.0 is needed.

### zlib (for PBF support)

    http://www.zlib.net/
    Debian/Ubuntu: zlib1g-dev

### Pandoc (optional, to build documentation)

    http://johnmacfarlane.net/pandoc/
    Debian/Ubuntu: pandoc
    (If pandoc is found by CMake, the manpages will automatically be built.)


## Building

You'll need the prerequisites including `libosmium` installed.
Libosmium is search in the folders mentioned in https://github.com/pnorman/osmborder/blob/master/cmake/FindOsmium.cmake#L59

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
  dividing_line bool,
  disputed bool,
  maritime bool,
  way Geometry(LineString, 3857));
\copy osmborder_lines FROM osmborder_lines.csv

CREATE INDEX osmborder_lines_way_idx ON osmborder_lines USING gist (way) WITH (fillfactor=100);
CLUSTER osmborder_lines USING osmborder_lines_way_idx;
CREATE INDEX osmborder_lines_way_low_idx ON osmborder_lines USING gist (way) WITH (fillfactor=100) WHERE admin_level <= 4;
```

The indexes are optional, but useful if rendering maps.

## Tags used

OSMBorder uses tags on the way and its parent relations. It does **not** consider geometry, relation roles, or non-way
relation members.

### admin_level

The admin_level is the lowest `admin_level` value of the parent relations. The way tags are not considered.

### disputed
The presence of `disputed=yes`, `dispute=yes`, or `border_status=dispute` on the ways is used to indicate part of a border is disputed. All the tags function the same, but `disputed=yes` is my preference. Relation tags are not considered.

### maritime
`maritime=yes` or `natural=coastline` indicates a maritime border for the purposes of rendering. Relations are not considered, nor intersection with water areas.

## Options

    -v, --verbose

Gives you detailed information on what osmborder is doing, including timing.

Run `osmborder --help` to see all options.

## License

OSMBorder is available under the GNU GPL version 3 or later.

## Authors

Paul Norman (penorman@mac.com)  
Based on OSMCoastline by Jochen Topf (jochen@topf.org)
