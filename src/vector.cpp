#include "geoson/vector.hpp"
#include "geoson/geoson.hpp"

#include <algorithm>

namespace geoson {

    Element::Element(const Geometry &geom, const std::unordered_map<std::string, std::string> &props,
                     const std::string &elem_type)
        : geometry(geom), properties(props), type(elem_type) {}

    Vector::Vector(const concord::Polygon &field_boundary, const concord::Datum &datum, const concord::Euler &heading,
                   CRS crs)
        : field_boundary_(field_boundary), datum_(datum), heading_(heading), crs_(crs) {}

    Vector Vector::fromFile(const std::filesystem::path &path) {
        auto fc = geoson::read(path);

        if (fc.features.empty()) {
            throw std::runtime_error("Vector::fromFile: No features found in file");
        }

        std::optional<std::pair<concord::Polygon, std::unordered_map<std::string, std::string>>> field_data;

        for (const auto &feature : fc.features) {
            if (std::holds_alternative<concord::Polygon>(feature.geometry)) {
                auto it = feature.properties.find("type");
                if (it != feature.properties.end() && it->second == "field") {
                    field_data = std::make_pair(std::get<concord::Polygon>(feature.geometry), feature.properties);
                    break;
                }
            }
        }

        if (!field_data) {
            for (const auto &feature : fc.features) {
                if (std::holds_alternative<concord::Polygon>(feature.geometry)) {
                    field_data = std::make_pair(std::get<concord::Polygon>(feature.geometry), feature.properties);
                    break;
                }
            }
        }

        if (!field_data) {
            throw std::runtime_error("Vector::fromFile: No polygon found to use as field boundary");
        }

        Vector vector(field_data->first, fc.datum, fc.heading);
        vector.field_properties_ = field_data->second;
        vector.global_properties_ = fc.global_properties;

        for (const auto &feature : fc.features) {
            auto type_it = feature.properties.find("type");
            bool isExplicitField = (type_it != feature.properties.end() && type_it->second == "field");

            if (!isExplicitField) {
                std::string elem_type = "unknown";
                if (type_it != feature.properties.end()) {
                    elem_type = type_it->second;
                }

                vector.elements_.emplace_back(feature.geometry, feature.properties, elem_type);
            }
        }

        return vector;
    }

    void Vector::toFile(const std::filesystem::path &path, CRS outputCrs) const {
        FeatureCollection fc;
        fc.datum = datum_;
        fc.heading = heading_;
        fc.global_properties = global_properties_;

        auto field_props = field_properties_;
        field_props["type"] = "field";
        fc.features.emplace_back(Feature{field_boundary_, field_props});

        for (const auto &element : elements_) {
            fc.features.emplace_back(Feature{element.geometry, element.properties});
        }

        geoson::write(fc, path, outputCrs);
    }

    const concord::Polygon &Vector::getFieldBoundary() const { return field_boundary_; }

    void Vector::setFieldBoundary(const concord::Polygon &boundary) { field_boundary_ = boundary; }

    const std::unordered_map<std::string, std::string> &Vector::getFieldProperties() const { return field_properties_; }

    void Vector::setFieldProperty(const std::string &key, const std::string &value) { field_properties_[key] = value; }

    void Vector::removeFieldProperty(const std::string &key) { field_properties_.erase(key); }

    size_t Vector::elementCount() const { return elements_.size(); }

    bool Vector::hasElements() const { return !elements_.empty(); }

    void Vector::clearElements() { elements_.clear(); }

    const Element &Vector::getElement(size_t index) const {
        if (index >= elements_.size())
            throw std::out_of_range("Element index out of range");
        return elements_[index];
    }

    Element &Vector::getElement(size_t index) {
        if (index >= elements_.size())
            throw std::out_of_range("Element index out of range");
        return elements_[index];
    }

    void Vector::addElement(const Geometry &geometry, const std::string &type,
                            const std::unordered_map<std::string, std::string> &properties) {
        auto props = properties;
        if (!type.empty()) {
            props["type"] = type;
        }
        elements_.emplace_back(geometry, props, type);
    }

    void Vector::removeElement(size_t index) {
        if (index < elements_.size()) {
            elements_.erase(elements_.begin() + index);
        }
    }

    void Vector::addPoint(const concord::Point &point, const std::string &type,
                          const std::unordered_map<std::string, std::string> &properties) {
        addElement(point, type, properties);
    }

    void Vector::addLine(const concord::Line &line, const std::string &type,
                         const std::unordered_map<std::string, std::string> &properties) {
        addElement(line, type, properties);
    }

    void Vector::addPath(const concord::Path &path, const std::string &type,
                         const std::unordered_map<std::string, std::string> &properties) {
        addElement(path, type, properties);
    }

    void Vector::addPolygon(const concord::Polygon &polygon, const std::string &type,
                            const std::unordered_map<std::string, std::string> &properties) {
        addElement(polygon, type, properties);
    }

    std::vector<Element> Vector::getElementsByType(const std::string &type) const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            if (element.type == type) {
                result.push_back(element);
            }
        }
        return result;
    }

    std::vector<Element> Vector::getPoints() const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            if (std::holds_alternative<concord::Point>(element.geometry)) {
                result.push_back(element);
            }
        }
        return result;
    }

    std::vector<Element> Vector::getLines() const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            if (std::holds_alternative<concord::Line>(element.geometry)) {
                result.push_back(element);
            }
        }
        return result;
    }

    std::vector<Element> Vector::getPaths() const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            if (std::holds_alternative<concord::Path>(element.geometry)) {
                result.push_back(element);
            }
        }
        return result;
    }

    std::vector<Element> Vector::getPolygons() const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            if (std::holds_alternative<concord::Polygon>(element.geometry)) {
                result.push_back(element);
            }
        }
        return result;
    }

    std::vector<Element> Vector::filterByProperty(const std::string &key, const std::string &value) const {
        std::vector<Element> result;
        for (const auto &element : elements_) {
            auto it = element.properties.find(key);
            if (it != element.properties.end() && it->second == value) {
                result.push_back(element);
            }
        }
        return result;
    }

    const concord::Datum &Vector::getDatum() const { return datum_; }

    void Vector::setDatum(const concord::Datum &datum) { datum_ = datum; }

    const concord::Euler &Vector::getHeading() const { return heading_; }

    void Vector::setHeading(const concord::Euler &heading) { heading_ = heading; }

    CRS Vector::getCRS() const { return crs_; }

    void Vector::setCRS(CRS crs) { crs_ = crs; }

    void Vector::setGlobalProperty(const std::string &key, const std::string &value) {
        global_properties_[key] = value;
    }

    std::string Vector::getGlobalProperty(const std::string &key, const std::string &default_value) const {
        auto it = global_properties_.find(key);
        return (it != global_properties_.end()) ? it->second : default_value;
    }

    const std::unordered_map<std::string, std::string> &Vector::getGlobalProperties() const {
        return global_properties_;
    }

    void Vector::removeGlobalProperty(const std::string &key) { global_properties_.erase(key); }

} // namespace geoson
