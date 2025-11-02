#include "geoson/parser.hpp"

#include <boost/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

namespace geoson {

    namespace op {
        boost::json::value ReadFeatureCollection(const std::filesystem::path &file) {
            std::ifstream ifs(file);
            if (!ifs) {
                throw std::runtime_error("geoson::ReadFeatureCollection(): cannot open \"" + file.string() + '\"');
            }

            std::stringstream buffer;
            buffer << ifs.rdbuf();
            boost::json::value j = boost::json::parse(buffer.str());

            if (!j.is_object() || !j.as_object().contains("type") || !j.as_object().at("type").is_string()) {
                throw std::runtime_error(
                    "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");
            }

            auto type = std::string(j.as_object().at("type").as_string());
            if (type == "FeatureCollection") {
                return j;
            }
            if (type == "Feature") {
                boost::json::object fc;
                fc["type"] = "FeatureCollection";
                boost::json::array features;
                features.push_back(j);
                fc["features"] = std::move(features);
                return fc;
            }

            boost::json::object feat;
            feat["type"] = "Feature";
            feat["geometry"] = j;
            feat["properties"] = boost::json::object();
            boost::json::object fc;
            fc["type"] = "FeatureCollection";
            boost::json::array features;
            features.push_back(std::move(feat));
            fc["features"] = std::move(features);
            return fc;
        }
    } // namespace op

    using json = boost::json::value;

    std::unordered_map<std::string, std::string> parseProperties(const json &props) {
        std::unordered_map<std::string, std::string> m;
        auto const &obj = props.as_object();
        m.reserve(obj.size());
        for (auto const &item : obj) {
            if (item.value().is_string())
                m[std::string(item.key())] = std::string(item.value().as_string());
            else
                m[std::string(item.key())] = boost::json::serialize(item.value());
        }
        return m;
    }

    concord::Point parsePoint(const json &coords, const concord::Datum &datum, geoson::CRS crs) {
        auto const &arr = coords.as_array();
        double x = boost::json::value_to<double>(arr.at(0));
        double y = boost::json::value_to<double>(arr.at(1));
        double z = arr.size() > 2 ? boost::json::value_to<double>(arr.at(2)) : 0.0;

        if (crs == geoson::CRS::ENU) {
            return concord::Point{x, y, z};
        } else {
            concord::WGS wgs{y, x, z};
            concord::ENU enu = wgs.toENU(datum);
            return concord::Point{enu.x, enu.y, enu.z};
        }
    }

    Geometry parseLineString(const json &coords, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<concord::Point> pts;
        auto const &arr = coords.as_array();
        pts.reserve(arr.size());
        for (auto const &c : arr)
            pts.push_back(parsePoint(c, datum, crs));
        if (pts.size() == 2)
            return concord::Line{pts[0], pts[1]};
        else
            return concord::Path{pts};
    }

    concord::Polygon parsePolygon(const json &coords, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<concord::Point> pts;
        auto const &arr = coords.as_array();
        auto const &ring = arr.at(0);
        auto const &ring_arr = ring.as_array();
        pts.reserve(ring_arr.size());
        for (auto const &c : ring_arr)
            pts.push_back(parsePoint(c, datum, crs));
        return concord::Polygon{pts};
    }

    std::vector<Geometry> parseGeometry(const json &geom, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<Geometry> out;
        auto const &obj = geom.as_object();
        auto type = std::string(obj.at("type").as_string());

        if (type == "Point") {
            out.emplace_back(parsePoint(obj.at("coordinates"), datum, crs));
        } else if (type == "LineString") {
            out.emplace_back(parseLineString(obj.at("coordinates"), datum, crs));
        } else if (type == "Polygon") {
            out.emplace_back(parsePolygon(obj.at("coordinates"), datum, crs));
        } else if (type == "MultiPoint") {
            auto const &coords_arr = obj.at("coordinates").as_array();
            for (auto const &c : coords_arr)
                out.emplace_back(parsePoint(c, datum, crs));
        } else if (type == "MultiLineString") {
            auto const &coords_arr = obj.at("coordinates").as_array();
            for (auto const &linegeoson : coords_arr)
                out.emplace_back(parseLineString(linegeoson, datum, crs));
        } else if (type == "MultiPolygon") {
            auto const &coords_arr = obj.at("coordinates").as_array();
            for (auto const &poly : coords_arr)
                out.emplace_back(parsePolygon(poly, datum, crs));
        } else if (type == "GeometryCollection") {
            auto const &geoms_arr = obj.at("geometries").as_array();
            for (auto const &sub : geoms_arr) {
                auto subs = parseGeometry(sub, datum, crs);
                out.insert(out.end(), subs.begin(), subs.end());
            }
        }
        return out;
    }

    geoson::CRS parseCRS(const std::string &s) {
        if (s == "EPSG:4326" || s == "WGS84" || s == "WGS")
            return geoson::CRS::WGS;
        else if (s == "ENU" || s == "ECEF")
            return geoson::CRS::ENU;
        throw std::runtime_error("Unknown CRS string: " + s);
    }

    FeatureCollection ReadFeatureCollection(const std::filesystem::path &file) {
        auto fc_json = op::ReadFeatureCollection(file);
        auto const &fc_obj = fc_json.as_object();

        if (!fc_obj.contains("properties") || !fc_obj.at("properties").is_object())
            throw std::runtime_error("missing top-level 'properties'");
        auto const &P = fc_obj.at("properties").as_object();

        if (!P.contains("crs") || !P.at("crs").is_string())
            throw std::runtime_error("'properties' missing string 'crs'");
        if (!P.contains("datum") || !P.at("datum").is_array() || P.at("datum").as_array().size() < 3)
            throw std::runtime_error("'properties' missing array 'datum' of ≥3 numbers");
        if (!P.contains("heading") || !P.at("heading").is_number())
            throw std::runtime_error("'properties' missing numeric 'heading'");

        auto crsVal = parseCRS(std::string(P.at("crs").as_string()));
        auto const &A = P.at("datum").as_array();
        concord::Datum d{boost::json::value_to<double>(A[0]), boost::json::value_to<double>(A[1]),
                         boost::json::value_to<double>(A[2])};
        double yaw = boost::json::value_to<double>(P.at("heading"));
        concord::Euler euler{0.0, 0.0, yaw};

        FeatureCollection fc;
        fc.datum = d;
        fc.heading = euler;
        fc.features.reserve(fc_obj.at("features").as_array().size());

        for (const auto &[key, value] : P) {
            if (key != "crs" && key != "datum" && key != "heading") {
                if (value.is_string()) {
                    fc.global_properties[std::string(key)] = std::string(value.as_string());
                } else {
                    fc.global_properties[std::string(key)] = boost::json::serialize(value);
                }
            }
        }

        for (auto const &feat : fc_obj.at("features").as_array()) {
            auto const &feat_obj = feat.as_object();
            if (!feat_obj.contains("geometry") || feat_obj.at("geometry").is_null())
                continue;
            auto geoms = parseGeometry(feat_obj.at("geometry"), d, crsVal);
            std::unordered_map<std::string, std::string> props_map;
            if (feat_obj.contains("properties")) {
                props_map = parseProperties(feat_obj.at("properties"));
            }
            for (auto &g : geoms)
                fc.features.emplace_back(Feature{std::move(g), props_map});
        }

        return fc;
    }

    std::ostream &operator<<(std::ostream &os, FeatureCollection const &fc) {
        os << "DATUM: " << fc.datum.lat << ", " << fc.datum.lon << ", " << fc.datum.alt << "\n"
           << "HEADING: " << fc.heading.yaw << "\n";
        os << "FEATURES: " << fc.features.size() << "\n";

        for (auto const &f : fc.features) {
            auto &v = f.geometry;
            if (std::get_if<concord::Polygon>(&v)) {
                os << "  POLYGON\n";
            } else if (std::get_if<concord::Line>(&v)) {
                os << "  LINE\n";
            } else if (std::get_if<concord::Path>(&v)) {
                os << "  PATH\n";
            } else if (std::get_if<concord::Point>(&v)) {
                os << "   POINT\n";
            }
            if (f.properties.size() > 0)
                os << "    PROPS:" << f.properties.size() << "\n";
        }

        return os;
    }

} // namespace geoson
