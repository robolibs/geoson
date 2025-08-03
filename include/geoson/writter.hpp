#pragma once

#include <cmath>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include "geoson/types.hpp"

namespace geoson {

    /// helper to turn a single Geometry into its GeoJSON object
    inline nlohmann::json geometryToJson(Geometry const &geom, const concord::Datum &datum, geoson::CRS outputCrs) {
        // Helper to build coordinates based on desired output CRS
        // Internal representation is always in Point coordinates (ENU/local system)
        auto ptCoords = [&](concord::Point const &p) {
            if (outputCrs == geoson::CRS::ENU) {
                // ENU output: coordinates are already in local system, output directly as x,y,z
                return nlohmann::json::array({p.x, p.y, p.z});
            } else {
                // WGS output: convert Point to ENU with datum, then to WGS
                concord::ENU enu{p, datum};
                concord::WGS wgs = enu.toWGS();
                return nlohmann::json::array({wgs.lon, wgs.lat, static_cast<int>(std::round(wgs.alt))});
            }
        };

        return std::visit(
            [&](auto const &shape) -> nlohmann::json {
                using T = std::decay_t<decltype(shape)>;
                nlohmann::json j;
                if constexpr (std::is_same_v<T, concord::Point>) {
                    j["type"] = "Point";
                    j["coordinates"] = ptCoords(shape);
                } else if constexpr (std::is_same_v<T, concord::Line>) {
                    j["type"] = "LineString";
                    j["coordinates"] = nlohmann::json::array({ptCoords(shape.getStart()), ptCoords(shape.getEnd())});
                } else if constexpr (std::is_same_v<T, concord::Path>) {
                    j["type"] = "LineString";
                    nlohmann::json arr = nlohmann::json::array();
                    for (auto const &p : shape.getPoints())
                        arr.push_back(ptCoords(p));
                    j["coordinates"] = std::move(arr);
                } else if constexpr (std::is_same_v<T, concord::Polygon>) {
                    j["type"] = "Polygon";
                    nlohmann::json rings = nlohmann::json::array();
                    nlohmann::json ring = nlohmann::json::array();
                    for (auto const &p : shape.getPoints())
                        ring.push_back(ptCoords(p));
                    rings.push_back(std::move(ring));
                    j["coordinates"] = std::move(rings);
                }
                return j;
            },
            geom);
    }

    /// turn one Feature into its GeoJSON object
    inline nlohmann::json featureToJson(Feature const &f, const concord::Datum &datum, geoson::CRS outputCrs) {
        nlohmann::json j;
        j["type"] = "Feature";
        j["properties"] = nlohmann::json::object();
        for (auto const &kv : f.properties)
            j["properties"][kv.first] = kv.second;
        j["geometry"] = geometryToJson(f.geometry, datum, outputCrs);
        return j;
    }

    /// serialize a full FeatureCollection to GeoJSON with specified output CRS
    inline nlohmann::json toJson(FeatureCollection const &fc, geoson::CRS outputCrs) {
        nlohmann::json j;
        j["type"] = "FeatureCollection";

        // top‐level properties
        {
            auto &P = j["properties"];
            P = nlohmann::json::object();

            // crs → string (based on output CRS, not internal storage)
            switch (outputCrs) {
            case geoson::CRS::WGS:
                P["crs"] = "EPSG:4326";
                break;
            case geoson::CRS::ENU:
                P["crs"] = "ENU";
                break;
            }

            // datum array
            P["datum"] = nlohmann::json::array({fc.datum.lat, fc.datum.lon, fc.datum.alt});

            // yaw only
            P["heading"] = fc.heading.yaw;
            
            // Add global properties
            for (const auto& [key, value] : fc.global_properties) {
                P[key] = value;
            }
        }

        // features (use output CRS for coordinate conversion)
        j["features"] = nlohmann::json::array();
        for (auto const &f : fc.features)
            j["features"].push_back(featureToJson(f, fc.datum, outputCrs));

        return j;
    }

    /// serialize a full FeatureCollection to GeoJSON (defaults to ENU output format)
    inline nlohmann::json toJson(FeatureCollection const &fc) { return toJson(fc, geoson::CRS::ENU); }

    /// write GeoJSON out to disk with specified output CRS (pretty‐printed)
    inline void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath,
                                       geoson::CRS outputCrs) {
        auto j = toJson(fc, outputCrs);
        std::ofstream ofs(outPath);
        if (!ofs)
            throw std::runtime_error("Cannot open for write: " + outPath.string());
        ofs << j.dump(2) << "\n";
    }

    /// write GeoJSON out to disk (pretty‐printed) - defaults to WGS output format
    inline void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath) {
        WriteFeatureCollection(fc, outPath, geoson::CRS::WGS);
    }

} // namespace geoson
