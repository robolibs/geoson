
<img align="right" width="26%" src="./misc/logo.png">

Vectkit
===

A modern C++20 library for reading, writing, and manipulating GeoJSON files with type-safe geometry handling and coordinate reference system (CRS) support.

## Overview

Vectkit provides a clean interface for working with GeoJSON files ([RFC 7946](https://tools.ietf.org/html/rfc7946)). It converts GeoJSON elements into strongly-typed geometries backed by [Datapod](https://github.com/robolibs/datapod) and [Concord](https://github.com/robolibs/concord) for precise spatial operations. All coordinates are stored internally in ENU (East-North-Up) local coordinates regardless of input format.

## Features

- **GeoJSON I/O**: Read and write FeatureCollections, Features, and all geometry types
- **CRS support**: WGS84 (EPSG:4326) and ENU coordinate systems with automatic conversion
- **Unified internal representation**: All coordinates stored as local ENU/Point coordinates
- **Flexible output**: Write in WGS84 or ENU format regardless of input format
- **Datum-centric**: Datum (lat/lon/alt) anchors all ENU transformations
- **`Vector` abstraction**: Higher-level field boundary + typed elements API
- **Geometry types**: Point, Segment (Line), Path (multi-point), Polygon — plus Multi* and GeometryCollection
- **Global properties**: Key-value metadata on the FeatureCollection itself
- **Short namespace alias**: `vk::` for `vectkit::`

## Dependencies

- [Datapod](https://github.com/robolibs/datapod) — geometry primitives (`Point`, `Segment`, `Polygon`, etc.)
- [Concord](https://github.com/robolibs/concord) — coordinate system handling and WGS↔ENU conversions
- [Optinum](https://github.com/robolibs/optinum) — numerical utilities
- [Graphix](https://github.com/robolibs/graphix) — graphics/visualization support

## Quick Start

```cpp
#include "vectkit/vectkit.hpp"
#include <iostream>

int main() {
    // Read a GeoJSON file — coordinates converted to ENU internally
    auto fc = vectkit::read("field.geojson");

    std::cout << fc << "\n";  // prints datum, heading, feature summary

    // Modify the datum
    fc.datum.latitude += 0.1;

    // Write back — defaults to WGS84 output
    vectkit::write(fc, "field_out.geojson");

    // Write in ENU format explicitly
    vectkit::write(fc, "field_enu.geojson", vectkit::CRS::ENU);
}
```

The short alias `vk::` is also available:

```cpp
auto fc = vk::read("field.geojson");
vk::write(fc, "out.geojson", vk::CRS::WGS);
```

## GeoJSON Format

Vectkit expects a top-level `properties` object with three required fields:

```json
{
  "type": "FeatureCollection",
  "properties": {
    "crs": "EPSG:4326",
    "datum": [5.0, 52.0, 0.0],
    "heading": 0.0
  },
  "features": [...]
}
```

| Field | Description |
|-------|-------------|
| `crs` | `"EPSG:4326"` / `"WGS84"` / `"WGS"` for WGS84, or `"ENU"` / `"ECEF"` for local |
| `datum` | `[longitude, latitude, altitude]` — the ENU reference origin |
| `heading` | Yaw angle in degrees |

Any additional key-value pairs in `properties` are stored as `global_properties` on the `FeatureCollection`.

## API Reference

### Low-level: FeatureCollection

```cpp
#include "vectkit/vectkit.hpp"

// Read
vectkit::FeatureCollection fc = vectkit::read("input.geojson");
// or: vectkit::ReadFeatureCollection("input.geojson");

// Write (WGS84 default)
vectkit::write(fc, "out.geojson");
vectkit::write(fc, "out.geojson", vectkit::CRS::WGS);
vectkit::write(fc, "out.geojson", vectkit::CRS::ENU);
// or: vectkit::WriteFeatureCollection(fc, "out.geojson", vectkit::CRS::WGS);
```

#### FeatureCollection struct

```cpp
struct FeatureCollection {
    dp::Geo   datum;             // {latitude, longitude, altitude}
    dp::Euler heading;           // {roll, pitch, yaw}
    std::vector<Feature> features;
    std::unordered_map<std::string, std::string> global_properties;
};

struct Feature {
    Geometry geometry;           // variant: Point, Segment, vector<Point>, Polygon
    std::unordered_map<std::string, std::string> properties;
};

enum class CRS { WGS, ENU };
```

#### Iterating features

```cpp
for (const auto& feature : fc.features) {
    if (std::holds_alternative<dp::Point>(feature.geometry)) {
        auto& p = std::get<dp::Point>(feature.geometry);
        // p.x, p.y, p.z — ENU coordinates
    } else if (std::holds_alternative<dp::Segment>(feature.geometry)) {
        auto& seg = std::get<dp::Segment>(feature.geometry);
        // seg.start, seg.end — each a dp::Point
    } else if (std::holds_alternative<std::vector<dp::Point>>(feature.geometry)) {
        auto& path = std::get<std::vector<dp::Point>>(feature.geometry);
    } else if (std::holds_alternative<dp::Polygon>(feature.geometry)) {
        auto& poly = std::get<dp::Polygon>(feature.geometry);
        // poly.vertices — dp::Vector<dp::Point>
    }
}
```

### High-level: Vector

`Vector` wraps a `FeatureCollection` into a field boundary + typed elements model, useful for agricultural/robotics applications.

```cpp
#include "vectkit/vector.hpp"

// Load from file — first Polygon with type="field" becomes the boundary
auto vec = vectkit::Vector::fromFile("field.geojson");

// Access the field boundary
const dp::Polygon& boundary = vec.getFieldBoundary();

// Add elements
dp::Point  pt{10.0, 20.0, 0.0};
dp::Segment seg{pt, dp::Point{30.0, 40.0, 0.0}};
std::vector<dp::Point> path = { pt, seg.end };

vec.addPoint(pt,   "obstacle", {{"name", "tree"}});
vec.addLine(seg,   "row");
vec.addPath(path,  "route");
vec.addPolygon(zone, "headland");

// Query elements
auto points   = vec.getPoints();
auto lines    = vec.getLines();
auto paths    = vec.getPaths();
auto polygons = vec.getPolygons();

auto obstacles = vec.getElementsByType("obstacle");
auto named     = vec.filterByProperty("name", "tree");

// Datum / heading
vec.setDatum(dp::Geo{52.0, 5.0, 0.0});
vec.setHeading(dp::Euler{0, 0, 45.0});

// Global properties
vec.setGlobalProperty("field_id", "F42");
std::string id = vec.getGlobalProperty("field_id");

// Save
vec.toFile("out.geojson");                       // WGS84 (default)
vec.toFile("out_enu.geojson", vectkit::CRS::ENU);
```

#### Vector element iteration

```cpp
for (const auto& elem : vec) {
    // elem.geometry  — Geometry variant
    // elem.properties
    // elem.type      — string type tag
}

std::cout << "Total elements: " << vec.elementCount() << "\n";
```

## Coordinate System Flow

```
┌─────────────┐  parse   ┌──────────────────┐  write   ┌──────────────┐
│ WGS84 input │ ────────→│ Point / ENU      │ ───────→ │ WGS84 output │
│ [lon,lat,alt]│          │ [x, y, z]        │          │ [lon,lat,alt] │
└─────────────┘          │  (internal)      │          └──────────────┘
                         │                  │
┌─────────────┐  parse   │                  │  write   ┌──────────────┐
│ ENU input   │ ────────→│                  │ ───────→ │ ENU output   │
│ [x, y, z]   │          │                  │          │ [x, y, z]    │
└─────────────┘          └──────────────────┘          └──────────────┘
                                   ↑
                              datum anchors
                           all transformations
```

## Supported Geometry Types

| GeoJSON type | Internal type | Notes |
|---|---|---|
| `Point` | `dp::Point` | |
| `LineString` (2 pts) | `dp::Segment` | |
| `LineString` (3+ pts) | `std::vector<dp::Point>` | |
| `Polygon` | `dp::Polygon` | Exterior ring only |
| `MultiPoint` | multiple `dp::Point` features | |
| `MultiLineString` | multiple Segment/Path features | |
| `MultiPolygon` | multiple `dp::Polygon` features | |
| `GeometryCollection` | flattened into individual features | |

## Error Handling

```cpp
try {
    auto fc = vectkit::read("missing.geojson");
} catch (const std::runtime_error& e) {
    // "vectkit::ReadFeatureCollection(): cannot open \"missing.geojson\""
    // "missing top-level 'properties'"
    // "'properties' missing string 'crs'"
    // "'properties' missing array 'datum' of ≥3 numbers"
    // "'properties' missing numeric 'heading'"
    // "Unknown CRS string: <value>"
    // "Cannot open for write: <path>"
}
```

## Building

```bash
# Configure (first time or after dependency changes)
make config

# Build
make build

# Run tests
make test

# Run a single test
make test TEST=<test_name>

# Full reconfigure (clears cache)
make reconfig

# Compiler override
make config CC=clang
make config CC=gcc
```

## CMake Integration

```cmake
include(FetchContent)
FetchContent_Declare(vectkit
    GIT_REPOSITORY https://github.com/robolibs/vectkit.git
    GIT_TAG        0.0.8
)
FetchContent_MakeAvailable(vectkit)
target_link_libraries(my_target PRIVATE vectkit::vectkit)
```

## Acknowledgments

See [ACKNOWLEDGMENTS.md](./ACKNOWLEDGMENTS.md).
