#pragma once

#include <concord/concord.hpp>
#include <datapod/datapod.hpp>

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace dp = ::datapod;

namespace geoson {
    // Internal geometry representation: all coordinates are stored as Point (ENU/local system)
    // Regardless of input CRS, coordinates are converted to local coordinate system during parsing
    using Geometry = std::variant<dp::Point, dp::Segment, std::vector<dp::Point>, dp::Polygon>;

    // Simple CRS representation - used for input parsing and output formatting
    enum class CRS { WGS, ENU };

    struct Feature {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
    };

    struct FeatureCollection {
        dp::Geo datum;
        dp::Euler heading;
        std::vector<Feature> features; // All geometries stored in Point (ENU/local) coordinates
        std::unordered_map<std::string, std::string> global_properties; // Global properties for the collection
    };

} // namespace geoson
