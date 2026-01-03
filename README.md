
<img align="right" width="26%" src="./misc/logo.png">

Geoson
===

A modern C++20 library for reading, writing, and manipulating GeoJSON files with type-saf// Export in multiple formats as needed
geoson::write(survey_data, "for_gis_wgs84.geojson", geoson::CRS::WGS);  // Global GIS
geoson::write(survey_data, "for_cad_enu.geojson", geoson::CRS::ENU);    // CAD software
geoson::write(survey_data, "archive_default.geojson");                  // ENU format (default)

// 4. The datum ensures perfect coordinate transformation between formats
// Internal Point representation provides consistency across all operationstry handling.

## Overview

Geoson is a high-performance C++ library that provides a clean interface for working with GeoJSON files ([RFC 7946](https://tools.ietf.org/html/rfc7946)). Built on top of [JSON for Modern C++](https://github.com/nlohmann/json), it converts GeoJSON elements into strongly-typed [Concord](https://github.com/smolfetch/concord) geometries for precise spatial operations.

## Features

### 🗂️ **Comprehensive GeoJSON Support**
- **FeatureCollection**: Read/write complete GeoJSON feature collections
- **Features**: Individual geographic features with properties
- **Geometries**: Point, Line, Path, Polygon support
- **Multi-geometries**: MultiPoint, MultiLineString, MultiPolygon
- **GeometryCollection**: Nested geometry collections

### 🌍 **Coordinate Reference Systems & Internal Representation**
- **WGS84** (EPSG:4326) - Global geodetic coordinates (latitude, longitude, altitude)
- **ENU** (East-North-Up) - Local coordinate systems relative to a datum point
- **Automatic CRS Detection**: Parses CRS from GeoJSON properties
- **Unified Internal Storage**: All coordinates stored internally as Point (ENU/local) coordinates
- **Flexible Output**: Write in either WGS84 or ENU format regardless of input format
- **Intelligent Coordinate Conversion**: Seamless transformation between coordinate systems

### 📍 **Datum-Centric Spatial Handling**
- **Reference Point Coordinates**: Critical datum (lat, lon, alt) for ENU transformations
- **Coordinate System Anchoring**: All ENU coordinates are relative to the datum
- **Heading/Orientation**: Euler angles for directional data and coordinate frame alignment
- **Type-safe Geometries**: Compile-time geometry type checking with CRS awareness
- **Bi-directional Conversion**: WGS ↔ ENU transformations preserve spatial relationships

### 🔧 **Developer Experience**
- **Convenient Aliases**: Simple `geoson::read()` and `geoson::write()` functions
- **Pretty Printing**: Human-readable output for debugging
- **Error Handling**: Comprehensive error reporting with context
- **File I/O**: Direct file reading/writing with path validation
- **Robust Parsing**: Handles malformed data gracefully

## Dependencies

- [Concord](https://github.com/smolfetch/concord) - Geometry and coordinate system handling
- [JSON for Modern C++](https://github.com/nlohmann/json) - JSON parsing and serialization

## Internal Representation & CRS Handling

Geoson uses a **unified internal representation** where all coordinates are stored as Point (ENU/local) coordinates, regardless of the input format. The CRS is only relevant during input parsing and output formatting - not for internal storage.

### Key Principles

1. **Parse Once, Store Consistently**: All input coordinates (WGS or ENU) are converted to Point (local) coordinates during parsing
2. **Internal Uniformity**: All geometry operations work with the same coordinate system internally
3. **No CRS Storage**: FeatureCollection doesn't store original CRS since internal representation is always Point coordinates
4. **Output Flexibility**: Choose WGS84 or ENU format when writing, independent of input format
5. **Datum-Centric**: The datum serves as the anchor point for all coordinate transformations

### Parsing Behavior

**Input Processing:**
- **WGS84 input** → Converted to Point (local/ENU) coordinates using datum → Stored internally
- **ENU input** → Used directly as Point (local) coordinates → Stored internally  
- **Result**: All geometries stored in consistent Point coordinate system

### Writing Behavior

**Output Processing:**
- **Internal Point coordinates** → Convert to chosen output format → Write to file
- **WGS84 output**: Point coordinates converted to lat/lon/alt using datum
- **ENU output**: Point coordinates written directly as local x/y/z
- **Default behavior**: ENU output (matches internal representation)

### Coordinate System Flow

```
┌─────────────┐    Parse     ┌──────────────────┐    Write      ┌─────────────┐
│ WGS84 Input │ ────────────→│ Point (Internal) │ ────────────→ │ WGS84 Output│
│ [lon,lat,alt]│              │ [x, y, z]        │               │ [lon,lat,alt]│
└─────────────┘              └──────────────────┘               └─────────────┘
                                       │
┌─────────────┐    Parse               │                 Write   ┌─────────────┐
│ ENU Input   │ ──────────────────────→│ ←────────────────────── │ ENU Output  │
│ [x, y, z]   │                        │                         │ [x, y, z]   │
└─────────────┘                        │                         └─────────────┘
                                       │ (Default)
                             ┌─────────▼─────────┐
                             │ All Operations    │
                             │ Work Here         │
                             │ (Point coords)    │
                             │ NO CRS STORED     │
                             └───────────────────┘
```

### The Critical Role of Datum

The **datum** is not just metadata—it's the **spatial anchor** that defines the coordinate system:

```cpp
// Datum coordinates (lat=52.0°N, lon=5.0°E, alt=0m) - Amsterdam area
concord::Datum datum{52.0, 5.0, 0.0};

// WGS84 Flavor: Global coordinates
// Point at latitude 52.1°, longitude 5.1° (about 11km northeast of datum) 
concord::Point wgs_point{concord::WGS{52.1, 5.1, 10.0}, datum/* converts to ENU internally */};

// ENU Flavor: Local coordinates  
// Point at 1000m East, 500m North, 10m Up from the datum
concord::Point enu_point{1000.0, 500.0, 10.0};
```

### Example Workflow

```cpp
// Step 1: Read WGS84 GeoJSON - coordinates automatically converted to Point (internal)
auto fc = geoson::read("global_data.geojson");  // Input: WGS84 coordinates
// Internal storage: All coordinates now in Point (local) format, no CRS stored

// Step 2: All processing works with consistent Point coordinates
// ... spatial operations, calculations, modifications ...

// Step 3: Write in different formats (choose output CRS at write time)
geoson::write(fc, "output_wgs84.geojson", geoson::CRS::WGS);  // Output: WGS84 format
geoson::write(fc, "output_enu.geojson", geoson::CRS::ENU);    // Output: ENU format  
geoson::write(fc, "output_default.geojson");                  // Output: ENU format (default)
```

### Datum Precision Impact

The datum location directly affects coordinate precision and usability:

```cpp
// High-precision datum for local surveying
concord::Datum survey_datum{52.123456, 5.654321, 12.345};

// All ENU coordinates will be relative to this precise reference point
// Perfect for: construction sites, precision agriculture, robotics

// Coarse datum for regional work  
concord::Datum regional_datum{52.0, 5.0, 0.0};

// Suitable for: city-level mapping, general GIS applications
```

### Internal Representation Workflow Example

```cpp
// 1. Read survey data (any input format - WGS or ENU)
auto survey_data = geoson::read("survey_data.geojson");
// Internal: All coordinates now stored as Point (local) coordinates

// 2. Process with local algorithms (distances, areas, etc.)
// All operations work with consistent Point coordinate system
// ... spatial processing on internal Point coordinates ...

// 3. Export in multiple formats as needed
geoson::write(survey_data, "for_gis_wgs84.geojson", geoson::CRS::WGS);  // Global GIS
geoson::write(survey_data, "for_cad_enu.geojson", geoson::CRS::ENU);    // CAD software
geoson::write(survey_data, "archive_original.geojson");                  // Original format

// 4. The datum ensures perfect coordinate transformation between formats
// Internal Point representation provides consistency across all operations
```

### Best Practices

1. **Choose datum wisely**: Place it at the center of your area of interest
2. **Consistent datum**: Use the same datum across related datasets  
3. **Precision matters**: More decimal places in datum = higher local accuracy
4. **Flavor selection**: Use ENU for local work, WGS84 for sharing/interoperability

## Quick Start

### Simple API with Convenient Aliases

```cpp
#include "geoson/geoson.hpp"
#include <iostream>

int main() {
    try {
        // Read GeoJSON file (automatically converts to internal Point coordinates)
        auto fc = geoson::read("data.geojson");
        
        // Print summary information
        std::cout << fc << std::endl;
        
        // Modify spatial reference (affects coordinate transformations)
        fc.datum.lat += 0.1;  // Shift datum north by 0.1 degrees
        fc.heading.yaw = 45.0; // Set heading to 45 degrees
        
        // Write in different formats using convenient aliases
        geoson::write(fc, "output_wgs84.geojson", geoson::CRS::WGS);  // WGS84 format
        geoson::write(fc, "output_enu.geojson", geoson::CRS::ENU);    // ENU format  
        geoson::write(fc, "output_default.geojson");                  // ENU format (default)
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
```

### Advanced Usage (Full Function Names)

```cpp
// Alternative: Use full function names for explicit control
auto fc = geoson::ReadFeatureCollection("data.geojson");
geoson::WriteFeatureCollection(fc, "output.geojson", geoson::CRS::WGS);
```

## API Reference

### Convenient Aliases

For streamlined usage, geoson provides simple aliases:

```cpp
// Reading
auto fc = geoson::read("input.geojson");

// Writing with specific output CRS
geoson::write(fc, "output.geojson", geoson::CRS::WGS);  // WGS84 output
geoson::write(fc, "output.geojson", geoson::CRS::ENU);  // ENU output

// Writing with original input CRS
geoson::write(fc, "output.geojson");  // Uses ENU format (internal representation)
```

### Full Function Names

For explicit control or when preferred:

```cpp
// Reading
auto fc = geoson::ReadFeatureCollection("input.geojson");

// Writing with specific output CRS  
geoson::WriteFeatureCollection(fc, "output.geojson", geoson::CRS::WGS);
geoson::WriteFeatureCollection(fc, "output.geojson", geoson::CRS::ENU);

// Writing with ENU format (default)
geoson::WriteFeatureCollection(fc, "output.geojson");
```

### Creating Geometries with CRS Awareness

```cpp
#include "geoson/geoson.hpp"

// Set up coordinate system and datum (critical for ENU transformations)
geoson::CRS crs = geoson::CRS::WGS;  // Will output in WGS84 flavor
concord::Datum datum{52.0, 5.0, 0.0};  // Amsterdam area reference point
concord::Euler heading{0.0, 0.0, 0.0};

// Create different geometry types
std::vector<geoson::Feature> features;

// Method 1: WGS coordinates (global lat/lon/alt)
// These coordinates are automatically converted to ENU internally using the datum
concord::Point wgs_point{concord::WGS{52.1, 5.1, 10.0}, datum};
std::unordered_map<std::string, std::string> pointProps;
pointProps["name"] = "Amsterdam Central";
pointProps["coordinate_type"] = "WGS84";
features.emplace_back(geoson::Feature{wgs_point, pointProps});

// Method 2: ENU coordinates (local east/north/up in meters)
// These are relative to the datum point
concord::Point enu_point{1000.0, 500.0, 10.0};  // 1km east, 500m north, 10m up from datum
std::unordered_map<std::string, std::string> enuProps;
enuProps["name"] = "Local Survey Point";
enuProps["coordinate_type"] = "ENU";
features.emplace_back(geoson::Feature{enu_point, enuProps});

// Line feature with mixed coordinate input
concord::Point start{concord::WGS{52.1, 5.1, 0.0}, datum};  // Global coordinates
concord::Point end{500.0, 1000.0, 0.0};  // Local ENU coordinates
concord::Line line{start, end};
std::unordered_map<std::string, std::string> lineProps;
lineProps["name"] = "Mixed Coordinate Line";
features.emplace_back(geoson::Feature{line, lineProps});

// Path feature (multiple points) - demonstrating coordinate flexibility
std::vector<concord::Point> pathPoints = {
    concord::Point{concord::WGS{52.1, 5.1, 0.0}, datum},    // Global coordinates
    concord::Point{1500.0, 1500.0, 0.0},                    // ENU coordinates  
    concord::Point{concord::WGS{52.2, 5.2, 0.0}, datum}     // Back to global
};
concord::Path path{pathPoints};
std::unordered_map<std::string, std::string> pathProps;
pathProps["name"] = "Survey Route";
pathProps["mixed_coordinates"] = "true";
features.emplace_back(geoson::Feature{path, pathProps});

// Polygon feature with consistent ENU coordinates for high precision
std::vector<concord::Point> polygonPoints = {
    concord::Point{0.0, 0.0, 0.0},        // At datum point
    concord::Point{100.0, 0.0, 0.0},      // 100m east
    concord::Point{100.0, 100.0, 0.0},    // 100m east, 100m north  
    concord::Point{0.0, 100.0, 0.0},      // 100m north
    concord::Point{0.0, 0.0, 0.0}         // Closed ring
};
concord::Polygon polygon{polygonPoints};
std::unordered_map<std::string, std::string> polyProps;
polyProps["name"] = "Survey Plot";
polyProps["area_m2"] = "10000";  // 100m x 100m = 10,000 m²
features.emplace_back(geoson::Feature{polygon, polyProps});

// Create feature collection (no CRS stored - always internal Point coordinates)
geoson::FeatureCollection fc{datum, heading, std::move(features)};

// Write in WGS84 format (global coordinates) - using convenient alias
geoson::write(fc, "global_output.geojson", geoson::CRS::WGS);
// Coordinates like [0.0, 0.0, 0.0] become [5.0, 52.0, 0.0] (datum location)

// Write in ENU format (local coordinates) - using convenient alias  
geoson::write(fc, "local_output.geojson", geoson::CRS::ENU);
// Coordinates remain as [0.0, 0.0, 0.0] (relative to datum)
```

### Coordinate System Conversion Examples

```cpp
// Reading and converting between formats using convenient aliases
auto global_data = geoson::read("global.geojson");  // Any input format
// Internal: All coordinates stored as Point (local) coordinates

// Export as WGS84 format
geoson::write(global_data, "output_wgs84.geojson", geoson::CRS::WGS);

// Export as ENU format  
geoson::write(global_data, "output_enu.geojson", geoson::CRS::ENU);

// Export in original input format
geoson::write(global_data, "output_original.geojson");
```

### Working with Properties and CRS Detection

```cpp
// Access feature properties and understand coordinate systems
for (const auto& feature : fc.features) {
    // Check geometry type
    if (std::holds_alternative<concord::Point>(feature.geometry)) {
        auto point = std::get<concord::Point>(feature.geometry);
        
        // Access the underlying ENU coordinates (always available)
        std::cout << "ENU coordinates: " << point.enu.east << ", " 
                  << point.enu.north << ", " << point.enu.up << std::endl;
                  
        // Convert back to WGS if needed
        auto wgs_coords = point.enu.toWGS(fc.datum);
        std::cout << "WGS coordinates: " << wgs_coords.lat << ", " 
                  << wgs_coords.lon << ", " << wgs_coords.alt << std::endl;
    }
    
    // Access properties and CRS metadata
    if (feature.properties.contains("name")) {
        std::cout << "Feature name: " << feature.properties.at("name") << std::endl;
    }
    
    if (feature.properties.contains("coordinate_type")) {
        std::cout << "Original coordinate type: " << feature.properties.at("coordinate_type") << std::endl;
    }
}

// Check the FeatureCollection's datum (no CRS stored internally)
std::cout << "Collection stores Point coordinates internally (no CRS)" << std::endl;
std::cout << "Datum: [" << fc.datum.lat << ", " << fc.datum.lon << ", " << fc.datum.alt << "]" << std::endl;
```

## Supported GeoJSON Features

| Feature | Status | CRS Support | Notes |
|---------|--------|-------------|-------|
| Point | ✅ | WGS84 + ENU | Single coordinate with automatic conversion |
| LineString | ✅ | WGS84 + ENU | 2 points → Line, 3+ points → Path |
| Polygon | ✅ | WGS84 + ENU | Exterior ring only, datum-aware coordinates |
| MultiPoint | ✅ | WGS84 + ENU | Multiple Point geometries |
| MultiLineString | ✅ | WGS84 + ENU | Multiple LineString geometries |
| MultiPolygon | ✅ | WGS84 + ENU | Multiple Polygon geometries |
| GeometryCollection | ✅ | WGS84 + ENU | Nested geometry collections |
| Feature | ✅ | WGS84 + ENU | Geometry + properties |
| FeatureCollection | ✅ | WGS84 + ENU | Collection with datum metadata (no CRS stored) |
| Custom CRS | ✅ | WGS84/ENU | Automatic detection and conversion |

### CRS Property Support

```json
{
  "type": "FeatureCollection",
  "properties": {
    "crs": "EPSG:4326",           // or "ENU" for local coordinates
    "datum": [52.0, 5.0, 0.0],   // Critical: lat, lon, alt reference point
    "heading": 0.0                // Optional: orientation in degrees
  },
  "features": [...]
}
```

## Error Handling

Geoson provides comprehensive error handling with descriptive messages, including CRS and datum validation:

```cpp
try {
    auto fc = geoson::ReadFeatureCollection("invalid.geojson");
} catch (const std::runtime_error& e) {
    // Catches parsing errors, file I/O errors, validation errors, CRS errors
    std::cerr << "Parsing failed: " << e.what() << std::endl;
}

// Example error messages for CRS and datum issues:
// - "Cannot open for write: /invalid/path/file.geojson"
// - "Unknown CRS string: EPSG:9999"
// - "'properties' missing string 'crs'"
// - "'properties' missing array 'datum' of ≥3 numbers"
// - "'properties' missing numeric 'heading'"
// - "geoson::ReadFeatureCollection(): cannot open \"missing.geojson\""

// CRS-specific validation errors:
try {
    geoson::parseCRS("invalid_crs");
} catch (const std::runtime_error& e) {
    // "Unknown CRS string: invalid_crs"
}

// Coordinate parsing errors (datum-dependent):
try {
    // Invalid coordinate array
    nlohmann::json coords = {5.1}; // Missing latitude
    geoson::parsePoint(coords, datum, geoson::CRS::WGS);
} catch (const nlohmann::json::out_of_range& e) {
    // Coordinate array access error
}
```

## Building

```bash
# Configure with tests enabled
make compile

# Build all targets
make build

# Run comprehensive test suite (includes CRS conversion and datum precision tests)
make test
```

## Use Cases and Benefits

### Internal Point Representation Benefits
- **Consistency**: All geometry operations work with the same coordinate system
- **Performance**: No coordinate conversion needed during processing
- **Precision**: Avoids repeated coordinate transformations that can accumulate errors
- **Simplicity**: Single coordinate system for all internal calculations

### Input/Output Flexibility
- **WGS84 Input**: Read standard GeoJSON files from web maps, GPS devices, GIS software
- **ENU Input**: Read high-precision survey data, CAD files, robotics datasets
- **WGS84 Output**: Export for web mapping, GIS integration, data sharing
- **ENU Output**: Export for CAD software, robotics, precision applications

### Workflow Applications

#### Global-to-Local Processing
```cpp
auto global_data = geoson::read("gps_tracks.geojson");        // WGS84 input
// ... process using local Point coordinates ...
geoson::write(global_data, "robot_path.geojson", geoson::CRS::ENU);  // ENU output
```

#### Local-to-Global Sharing  
```cpp
auto survey_data = geoson::read("field_survey.geojson");      // ENU input
// ... analyze using local Point coordinates ...
geoson::write(survey_data, "public_map.geojson", geoson::CRS::WGS);  // WGS84 output
```

#### Multi-Format Export
```cpp
auto data = geoson::read("source.geojson");                   // Any input format
geoson::write(data, "for_web.geojson", geoson::CRS::WGS);     // Web mapping
geoson::write(data, "for_cad.geojson", geoson::CRS::ENU);     // CAD integration
geoson::write(data, "archive.geojson");                       // ENU format (default)
```

### Performance Considerations

- **Memory efficiency**: Single internal coordinate system reduces storage overhead
- **Processing speed**: No coordinate conversion during geometry operations
- **Precision preservation**: Datum-based transformations maintain spatial accuracy
- **Conversion cost**: Input/output transformations are performed only when needed
- **Consistency**: All calculations use the same coordinate reference frame

## Acknowledgements

This library was originally inspired by [libgeojson](https://github.com/psalvaggio/libgeojson), but has been completely rewritten with modern C++20 features, CRS-aware coordinate handling, and a focus on type safety and datum-based spatial precision.
