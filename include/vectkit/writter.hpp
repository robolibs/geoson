#pragma once

#include "vectkit/types.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace vectkit {

    namespace detail {
        // Helper to escape a string for JSON
        inline std::string escape_string(const std::string &s) {
            std::string result;
            result.reserve(s.size() + 2);
            for (char c : s) {
                switch (c) {
                case '"':
                    result += "\\\"";
                    break;
                case '\\':
                    result += "\\\\";
                    break;
                case '\b':
                    result += "\\b";
                    break;
                case '\f':
                    result += "\\f";
                    break;
                case '\n':
                    result += "\\n";
                    break;
                case '\r':
                    result += "\\r";
                    break;
                case '\t':
                    result += "\\t";
                    break;
                default:
                    result += c;
                    break;
                }
            }
            return result;
        }

        // Helper to build a JSON array of coordinates
        inline std::string coords_to_json(double x, double y, double z, bool round_z = false) {
            std::ostringstream oss;
            oss << "[" << std::setprecision(15) << x << "," << y << ",";
            if (round_z) {
                oss << static_cast<int>(std::round(z));
            } else {
                oss << std::setprecision(15) << z;
            }
            oss << "]";
            return oss.str();
        }
    } // namespace detail

    inline std::string geometryToJson(Geometry const &geom, const dp::Geo &datum, vectkit::CRS outputCrs) {
        auto ptCoords = [&](dp::Point const &p) -> std::string {
            if (outputCrs == vectkit::CRS::ENU) {
                return detail::coords_to_json(p.x, p.y, p.z);
            } else {
                concord::frame::ENU enu{p, datum};
                auto wgs = concord::frame::to_wgs(enu);
                return detail::coords_to_json(wgs.longitude, wgs.latitude, wgs.altitude, true);
            }
        };

        return std::visit(
            [&](auto const &shape) -> std::string {
                using T = std::decay_t<decltype(shape)>;
                std::ostringstream oss;

                if constexpr (std::is_same_v<T, dp::Point>) {
                    oss << R"({"type":"Point","coordinates":)" << ptCoords(shape) << "}";
                } else if constexpr (std::is_same_v<T, dp::Segment>) {
                    oss << R"({"type":"LineString","coordinates":[)" << ptCoords(shape.start) << ","
                        << ptCoords(shape.end) << "]}";
                } else if constexpr (std::is_same_v<T, std::vector<dp::Point>>) {
                    oss << R"({"type":"LineString","coordinates":[)";
                    bool first = true;
                    for (auto const &p : shape) {
                        if (!first)
                            oss << ",";
                        first = false;
                        oss << ptCoords(p);
                    }
                    oss << "]}";
                } else if constexpr (std::is_same_v<T, dp::Polygon>) {
                    oss << R"({"type":"Polygon","coordinates":[[)";
                    bool first = true;
                    for (auto const &p : shape.vertices) {
                        if (!first)
                            oss << ",";
                        first = false;
                        oss << ptCoords(p);
                    }
                    oss << "]]}";
                }
                return oss.str();
            },
            geom);
    }

    inline std::string featureToJson(Feature const &f, const dp::Geo &datum, vectkit::CRS outputCrs) {
        std::ostringstream oss;
        oss << R"({"type":"Feature","properties":{)";

        bool first = true;
        for (auto const &kv : f.properties) {
            if (!first)
                oss << ",";
            first = false;
            oss << "\"" << detail::escape_string(kv.first) << "\":\"" << detail::escape_string(kv.second) << "\"";
        }

        oss << "}," << R"("geometry":)" << geometryToJson(f.geometry, datum, outputCrs) << "}";
        return oss.str();
    }

    inline std::string toJson(FeatureCollection const &fc, vectkit::CRS outputCrs) {
        std::ostringstream oss;
        oss << R"({"type":"FeatureCollection","properties":{)";

        // CRS
        if (outputCrs == vectkit::CRS::WGS) {
            oss << R"("crs":"EPSG:4326")";
        } else {
            oss << R"("crs":"ENU")";
        }

        // Datum - GeoJSON uses [longitude, latitude, altitude] order
        oss << "," << R"("datum":[)" << std::setprecision(15) << fc.datum.longitude << "," << fc.datum.latitude << ","
            << fc.datum.altitude << "]";

        // Heading
        oss << "," << R"("heading":)" << std::setprecision(15) << fc.heading.yaw;

        // Global properties
        for (const auto &[key, value] : fc.global_properties) {
            oss << ",\"" << detail::escape_string(key) << "\":\"" << detail::escape_string(value) << "\"";
        }

        oss << "}," << R"("features":[)";

        bool first = true;
        for (auto const &f : fc.features) {
            if (!first)
                oss << ",";
            first = false;
            oss << featureToJson(f, fc.datum, outputCrs);
        }

        oss << "]}";
        return oss.str();
    }

    inline void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath,
                                       vectkit::CRS outputCrs) {
        std::string j = toJson(fc, outputCrs);
        std::ofstream ofs(outPath);
        if (!ofs)
            throw std::runtime_error("Cannot open for write: " + outPath.string());
        ofs << j << "\n";
    }

    inline void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath) {
        WriteFeatureCollection(fc, outPath, vectkit::CRS::WGS);
    }

} // namespace vectkit
