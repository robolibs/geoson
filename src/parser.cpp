#include "geoson/parser.hpp"

#include "json.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>
#include <vector>

namespace geoson {

    namespace {
        // RAII wrapper for json_value_s to ensure proper cleanup
        struct JsonDeleter {
            void operator()(json_value_s *ptr) const {
                if (ptr)
                    free(ptr);
            }
        };
        using JsonPtr = std::unique_ptr<json_value_s, JsonDeleter>;

        // Helper to find an element in a JSON object by key
        json_object_element_s *find_element(json_object_s *obj, const char *key) {
            if (!obj)
                return nullptr;
            for (auto *elem = obj->start; elem; elem = elem->next) {
                if (elem->name && strcmp(elem->name->string, key) == 0) {
                    return elem;
                }
            }
            return nullptr;
        }

        // Helper to get a string value from a JSON value
        std::string get_string(json_value_s *val) {
            if (!val || val->type != json_type_string)
                return "";
            auto *str = static_cast<json_string_s *>(val->payload);
            return std::string(str->string, str->string_size);
        }

        // Helper to get a number value from a JSON value
        double get_number(json_value_s *val) {
            if (!val || val->type != json_type_number)
                return 0.0;
            auto *num = static_cast<json_number_s *>(val->payload);
            return std::stod(std::string(num->number, num->number_size));
        }

        // Helper to get an object from a JSON value
        json_object_s *get_object(json_value_s *val) {
            if (!val || val->type != json_type_object)
                return nullptr;
            return static_cast<json_object_s *>(val->payload);
        }

        // Helper to get an array from a JSON value
        json_array_s *get_array(json_value_s *val) {
            if (!val || val->type != json_type_array)
                return nullptr;
            return static_cast<json_array_s *>(val->payload);
        }

        // Helper to serialize a JSON value to string
        std::string serialize_value(json_value_s *val) {
            if (!val)
                return "null";

            switch (val->type) {
            case json_type_string: {
                auto *str = static_cast<json_string_s *>(val->payload);
                return "\"" + std::string(str->string, str->string_size) + "\"";
            }
            case json_type_number: {
                auto *num = static_cast<json_number_s *>(val->payload);
                return std::string(num->number, num->number_size);
            }
            case json_type_true:
                return "true";
            case json_type_false:
                return "false";
            case json_type_null:
                return "null";
            case json_type_object: {
                auto *obj = static_cast<json_object_s *>(val->payload);
                std::string result = "{";
                bool first = true;
                for (auto *elem = obj->start; elem; elem = elem->next) {
                    if (!first)
                        result += ",";
                    first = false;
                    result += "\"" + std::string(elem->name->string, elem->name->string_size) + "\":";
                    result += serialize_value(elem->value);
                }
                result += "}";
                return result;
            }
            case json_type_array: {
                auto *arr = static_cast<json_array_s *>(val->payload);
                std::string result = "[";
                bool first = true;
                for (auto *elem = arr->start; elem; elem = elem->next) {
                    if (!first)
                        result += ",";
                    first = false;
                    result += serialize_value(elem->value);
                }
                result += "]";
                return result;
            }
            default:
                return "null";
            }
        }
    } // namespace

    namespace op {
        JsonPtr ReadFeatureCollection(const std::filesystem::path &file) {
            std::ifstream ifs(file);
            if (!ifs) {
                throw std::runtime_error("geoson::ReadFeatureCollection(): cannot open \"" + file.string() + '\"');
            }

            std::stringstream buffer;
            buffer << ifs.rdbuf();
            std::string content = buffer.str();

            json_value_s *root = json_parse(content.c_str(), content.size());
            if (!root) {
                throw std::runtime_error("geoson::ReadFeatureCollection(): failed to parse JSON");
            }

            JsonPtr j(root);

            auto *obj = get_object(j.get());
            if (!obj) {
                throw std::runtime_error(
                    "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");
            }

            auto *type_elem = find_element(obj, "type");
            if (!type_elem || type_elem->value->type != json_type_string) {
                throw std::runtime_error(
                    "geoson::ReadFeatureCollection(): top-level object has no string 'type' field");
            }

            std::string type = get_string(type_elem->value);

            if (type == "FeatureCollection") {
                return j;
            }

            // For Feature or bare geometry, we need to wrap it - but since json.hpp doesn't
            // support building JSON, we'll re-parse a constructed string
            if (type == "Feature") {
                std::string wrapped = R"({"type":"FeatureCollection","features":[)" + serialize_value(j.get()) + "]}";
                json_value_s *new_root = json_parse(wrapped.c_str(), wrapped.size());
                return JsonPtr(new_root);
            }

            // Bare geometry - wrap in Feature, then FeatureCollection
            std::string wrapped = R"({"type":"FeatureCollection","features":[{"type":"Feature","geometry":)" +
                                  serialize_value(j.get()) + R"(,"properties":{}}]})";
            json_value_s *new_root = json_parse(wrapped.c_str(), wrapped.size());
            return JsonPtr(new_root);
        }
    } // namespace op

    std::unordered_map<std::string, std::string> parseProperties(json_object_s *props) {
        std::unordered_map<std::string, std::string> m;
        if (!props)
            return m;

        for (auto *elem = props->start; elem; elem = elem->next) {
            std::string key(elem->name->string, elem->name->string_size);
            if (elem->value->type == json_type_string) {
                m[key] = get_string(elem->value);
            } else {
                m[key] = serialize_value(elem->value);
            }
        }
        return m;
    }

    concord::Point parsePoint(json_array_s *coords, const concord::Datum &datum, geoson::CRS crs) {
        if (!coords || coords->length < 2) {
            throw std::runtime_error("Invalid point coordinates");
        }

        auto *x_elem = coords->start;
        auto *y_elem = x_elem->next;
        auto *z_elem = y_elem ? y_elem->next : nullptr;

        double x = get_number(x_elem->value);
        double y = get_number(y_elem->value);
        double z = z_elem ? get_number(z_elem->value) : 0.0;

        if (crs == geoson::CRS::ENU) {
            return concord::Point{x, y, z};
        } else {
            concord::WGS wgs{y, x, z};
            concord::ENU enu = wgs.toENU(datum);
            return concord::Point{enu.x, enu.y, enu.z};
        }
    }

    Geometry parseLineString(json_array_s *coords, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<concord::Point> pts;
        if (!coords)
            return pts;

        for (auto *elem = coords->start; elem; elem = elem->next) {
            auto *pt_arr = get_array(elem->value);
            if (pt_arr) {
                pts.push_back(parsePoint(pt_arr, datum, crs));
            }
        }

        if (pts.size() == 2)
            return concord::Line{pts[0], pts[1]};
        else
            return pts;
    }

    concord::Polygon parsePolygon(json_array_s *coords, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<concord::Point> pts;
        if (!coords || !coords->start)
            return concord::Polygon{pts};

        // Get the first ring (outer ring)
        auto *ring_arr = get_array(coords->start->value);
        if (!ring_arr)
            return concord::Polygon{pts};

        for (auto *elem = ring_arr->start; elem; elem = elem->next) {
            auto *pt_arr = get_array(elem->value);
            if (pt_arr) {
                pts.push_back(parsePoint(pt_arr, datum, crs));
            }
        }

        return concord::Polygon{pts};
    }

    std::vector<Geometry> parseGeometry(json_object_s *geom, const concord::Datum &datum, geoson::CRS crs) {
        std::vector<Geometry> out;
        if (!geom)
            return out;

        auto *type_elem = find_element(geom, "type");
        if (!type_elem)
            return out;

        std::string type = get_string(type_elem->value);
        auto *coords_elem = find_element(geom, "coordinates");
        auto *coords = coords_elem ? get_array(coords_elem->value) : nullptr;

        if (type == "Point") {
            if (coords) {
                out.emplace_back(parsePoint(coords, datum, crs));
            }
        } else if (type == "LineString") {
            if (coords) {
                out.emplace_back(parseLineString(coords, datum, crs));
            }
        } else if (type == "Polygon") {
            if (coords) {
                out.emplace_back(parsePolygon(coords, datum, crs));
            }
        } else if (type == "MultiPoint") {
            if (coords) {
                for (auto *elem = coords->start; elem; elem = elem->next) {
                    auto *pt_arr = get_array(elem->value);
                    if (pt_arr) {
                        out.emplace_back(parsePoint(pt_arr, datum, crs));
                    }
                }
            }
        } else if (type == "MultiLineString") {
            if (coords) {
                for (auto *elem = coords->start; elem; elem = elem->next) {
                    auto *line_arr = get_array(elem->value);
                    if (line_arr) {
                        out.emplace_back(parseLineString(line_arr, datum, crs));
                    }
                }
            }
        } else if (type == "MultiPolygon") {
            if (coords) {
                for (auto *elem = coords->start; elem; elem = elem->next) {
                    auto *poly_arr = get_array(elem->value);
                    if (poly_arr) {
                        out.emplace_back(parsePolygon(poly_arr, datum, crs));
                    }
                }
            }
        } else if (type == "GeometryCollection") {
            auto *geoms_elem = find_element(geom, "geometries");
            auto *geoms_arr = geoms_elem ? get_array(geoms_elem->value) : nullptr;
            if (geoms_arr) {
                for (auto *elem = geoms_arr->start; elem; elem = elem->next) {
                    auto *sub_obj = get_object(elem->value);
                    if (sub_obj) {
                        auto subs = parseGeometry(sub_obj, datum, crs);
                        out.insert(out.end(), subs.begin(), subs.end());
                    }
                }
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
        auto *fc_obj = get_object(fc_json.get());

        auto *props_elem = find_element(fc_obj, "properties");
        if (!props_elem || props_elem->value->type != json_type_object)
            throw std::runtime_error("missing top-level 'properties'");

        auto *P = get_object(props_elem->value);

        auto *crs_elem = find_element(P, "crs");
        if (!crs_elem || crs_elem->value->type != json_type_string)
            throw std::runtime_error("'properties' missing string 'crs'");

        auto *datum_elem = find_element(P, "datum");
        auto *datum_arr = datum_elem ? get_array(datum_elem->value) : nullptr;
        if (!datum_arr || datum_arr->length < 3)
            throw std::runtime_error("'properties' missing array 'datum' of ≥3 numbers");

        auto *heading_elem = find_element(P, "heading");
        if (!heading_elem || heading_elem->value->type != json_type_number)
            throw std::runtime_error("'properties' missing numeric 'heading'");

        auto crsVal = parseCRS(get_string(crs_elem->value));

        // Parse datum array
        auto *d0 = datum_arr->start;
        auto *d1 = d0->next;
        auto *d2 = d1->next;
        concord::Datum d{get_number(d0->value), get_number(d1->value), get_number(d2->value)};

        double yaw = get_number(heading_elem->value);
        concord::Euler euler{0.0, 0.0, yaw};

        FeatureCollection fc;
        fc.datum = d;
        fc.heading = euler;

        // Parse global properties (excluding crs, datum, heading)
        for (auto *elem = P->start; elem; elem = elem->next) {
            std::string key(elem->name->string, elem->name->string_size);
            if (key != "crs" && key != "datum" && key != "heading") {
                if (elem->value->type == json_type_string) {
                    fc.global_properties[key] = get_string(elem->value);
                } else {
                    fc.global_properties[key] = serialize_value(elem->value);
                }
            }
        }

        // Parse features
        auto *features_elem = find_element(fc_obj, "features");
        auto *features_arr = features_elem ? get_array(features_elem->value) : nullptr;
        if (features_arr) {
            for (auto *feat_elem = features_arr->start; feat_elem; feat_elem = feat_elem->next) {
                auto *feat_obj = get_object(feat_elem->value);
                if (!feat_obj)
                    continue;

                auto *geom_elem = find_element(feat_obj, "geometry");
                if (!geom_elem || geom_elem->value->type == json_type_null)
                    continue;

                auto *geom_obj = get_object(geom_elem->value);
                auto geoms = parseGeometry(geom_obj, d, crsVal);

                std::unordered_map<std::string, std::string> props_map;
                auto *feat_props_elem = find_element(feat_obj, "properties");
                if (feat_props_elem && feat_props_elem->value->type == json_type_object) {
                    props_map = parseProperties(get_object(feat_props_elem->value));
                }

                for (auto &g : geoms)
                    fc.features.emplace_back(Feature{std::move(g), props_map});
            }
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
            } else if (std::get_if<std::vector<concord::Point>>(&v)) {
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
