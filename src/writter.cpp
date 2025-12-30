#include "geoson/writter.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace geoson {

    namespace {
        // Helper to escape a string for JSON
        std::string escape_string(const std::string &s) {
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
        std::string coords_to_json(double x, double y, double z, bool round_z = false) {
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
    } // namespace

    std::string geometryToJson(Geometry const &geom, const concord::Datum &datum, geoson::CRS outputCrs) {
        auto ptCoords = [&](concord::Point const &p) -> std::string {
            if (outputCrs == geoson::CRS::ENU) {
                return coords_to_json(p.x, p.y, p.z);
            } else {
                concord::ENU enu{p, datum};
                concord::WGS wgs = enu.toWGS();
                return coords_to_json(wgs.lon, wgs.lat, wgs.alt, true);
            }
        };

        return std::visit(
            [&](auto const &shape) -> std::string {
                using T = std::decay_t<decltype(shape)>;
                std::ostringstream oss;

                if constexpr (std::is_same_v<T, concord::Point>) {
                    oss << R"({"type":"Point","coordinates":)" << ptCoords(shape) << "}";
                } else if constexpr (std::is_same_v<T, concord::Line>) {
                    oss << R"({"type":"LineString","coordinates":[)" << ptCoords(shape.getStart()) << ","
                        << ptCoords(shape.getEnd()) << "]}";
                } else if constexpr (std::is_same_v<T, std::vector<concord::Point>>) {
                    oss << R"({"type":"LineString","coordinates":[)";
                    bool first = true;
                    for (auto const &p : shape) {
                        if (!first)
                            oss << ",";
                        first = false;
                        oss << ptCoords(p);
                    }
                    oss << "]}";
                } else if constexpr (std::is_same_v<T, concord::Polygon>) {
                    oss << R"({"type":"Polygon","coordinates":[[)";
                    bool first = true;
                    for (auto const &p : shape.getPoints()) {
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

    std::string featureToJson(Feature const &f, const concord::Datum &datum, geoson::CRS outputCrs) {
        std::ostringstream oss;
        oss << R"({"type":"Feature","properties":{)";

        bool first = true;
        for (auto const &kv : f.properties) {
            if (!first)
                oss << ",";
            first = false;
            oss << "\"" << escape_string(kv.first) << "\":\"" << escape_string(kv.second) << "\"";
        }

        oss << "}," << R"("geometry":)" << geometryToJson(f.geometry, datum, outputCrs) << "}";
        return oss.str();
    }

    std::string toJson(FeatureCollection const &fc, geoson::CRS outputCrs) {
        std::ostringstream oss;
        oss << R"({"type":"FeatureCollection","properties":{)";

        // CRS
        if (outputCrs == geoson::CRS::WGS) {
            oss << R"("crs":"EPSG:4326")";
        } else {
            oss << R"("crs":"ENU")";
        }

        // Datum
        oss << "," << R"("datum":[)" << std::setprecision(15) << fc.datum.lat << "," << fc.datum.lon << ","
            << fc.datum.alt << "]";

        // Heading
        oss << "," << R"("heading":)" << std::setprecision(15) << fc.heading.yaw;

        // Global properties
        for (const auto &[key, value] : fc.global_properties) {
            oss << ",\"" << escape_string(key) << "\":\"" << escape_string(value) << "\"";
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

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath,
                                geoson::CRS outputCrs) {
        std::string j = toJson(fc, outputCrs);
        std::ofstream ofs(outPath);
        if (!ofs)
            throw std::runtime_error("Cannot open for write: " + outPath.string());
        ofs << j << "\n";
    }

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath) {
        WriteFeatureCollection(fc, outPath, geoson::CRS::WGS);
    }

} // namespace geoson
