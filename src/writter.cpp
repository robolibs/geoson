#include "geoson/writter.hpp"

#include <boost/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace geoson {

    boost::json::value geometryToJson(Geometry const &geom, const concord::Datum &datum, geoson::CRS outputCrs) {
        auto ptCoords = [&](concord::Point const &p) -> boost::json::array {
            boost::json::array arr;
            if (outputCrs == geoson::CRS::ENU) {
                arr.push_back(p.x);
                arr.push_back(p.y);
                arr.push_back(p.z);
            } else {
                concord::ENU enu{p, datum};
                concord::WGS wgs = enu.toWGS();
                arr.push_back(wgs.lon);
                arr.push_back(wgs.lat);
                arr.push_back(static_cast<int>(std::round(wgs.alt)));
            }
            return arr;
        };

        return std::visit(
            [&](auto const &shape) -> boost::json::value {
                using T = std::decay_t<decltype(shape)>;
                boost::json::object j;
                if constexpr (std::is_same_v<T, concord::Point>) {
                    j["type"] = "Point";
                    j["coordinates"] = ptCoords(shape);
                } else if constexpr (std::is_same_v<T, concord::Line>) {
                    j["type"] = "LineString";
                    boost::json::array coords;
                    coords.push_back(ptCoords(shape.getStart()));
                    coords.push_back(ptCoords(shape.getEnd()));
                    j["coordinates"] = std::move(coords);
                } else if constexpr (std::is_same_v<T, concord::Path>) {
                    j["type"] = "LineString";
                    boost::json::array arr;
                    for (auto const &p : shape.getPoints())
                        arr.push_back(ptCoords(p));
                    j["coordinates"] = std::move(arr);
                } else if constexpr (std::is_same_v<T, concord::Polygon>) {
                    j["type"] = "Polygon";
                    boost::json::array rings;
                    boost::json::array ring;
                    for (auto const &p : shape.getPoints())
                        ring.push_back(ptCoords(p));
                    rings.push_back(std::move(ring));
                    j["coordinates"] = std::move(rings);
                }
                return j;
            },
            geom);
    }

    boost::json::value featureToJson(Feature const &f, const concord::Datum &datum, geoson::CRS outputCrs) {
        boost::json::object j;
        j["type"] = "Feature";
        boost::json::object props;
        for (auto const &kv : f.properties)
            props[kv.first] = kv.second;
        j["properties"] = std::move(props);
        j["geometry"] = geometryToJson(f.geometry, datum, outputCrs);
        return j;
    }

    boost::json::value toJson(FeatureCollection const &fc, geoson::CRS outputCrs) {
        boost::json::object j;
        j["type"] = "FeatureCollection";

        {
            boost::json::object P;

            switch (outputCrs) {
            case geoson::CRS::WGS:
                P["crs"] = "EPSG:4326";
                break;
            case geoson::CRS::ENU:
                P["crs"] = "ENU";
                break;
            }

            boost::json::array datum_arr;
            datum_arr.push_back(fc.datum.lat);
            datum_arr.push_back(fc.datum.lon);
            datum_arr.push_back(fc.datum.alt);
            P["datum"] = std::move(datum_arr);

            P["heading"] = fc.heading.yaw;

            for (const auto &[key, value] : fc.global_properties) {
                P[key] = value;
            }

            j["properties"] = std::move(P);
        }

        boost::json::array features;
        for (auto const &f : fc.features)
            features.push_back(featureToJson(f, fc.datum, outputCrs));
        j["features"] = std::move(features);

        return j;
    }

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath,
                                geoson::CRS outputCrs) {
        auto j = toJson(fc, outputCrs);
        std::ofstream ofs(outPath);
        if (!ofs)
            throw std::runtime_error("Cannot open for write: " + outPath.string());
        ofs << boost::json::serialize(j) << "\n";
    }

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath) {
        WriteFeatureCollection(fc, outPath, geoson::CRS::WGS);
    }

} // namespace geoson
