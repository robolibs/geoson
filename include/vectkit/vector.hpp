#pragma once

#include "vectkit/types.hpp"
#include "vectkit/vectkit.hpp"

#include <algorithm>
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>

namespace vectkit {

    struct Element {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
        std::string type;

        Element(const Geometry &geom, const std::unordered_map<std::string, std::string> &props = {},
                const std::string &elem_type = "")
            : geometry(geom), properties(props), type(elem_type) {}
    };

    class Vector {
      private:
        dp::Polygon field_boundary_;
        std::unordered_map<std::string, std::string> field_properties_;
        std::vector<Element> elements_;

        dp::Geo datum_;
        dp::Euler heading_;
        CRS crs_;

        std::unordered_map<std::string, std::string> global_properties_;

      public:
        Vector() = delete;

        explicit Vector(const dp::Polygon &field_boundary, const dp::Geo &datum = dp::Geo{0.001, 0.001, 1.0},
                        const dp::Euler &heading = dp::Euler{0, 0, 0}, CRS crs = CRS::ENU)
            : field_boundary_(field_boundary), datum_(datum), heading_(heading), crs_(crs) {}

        static Vector fromFile(const std::filesystem::path &path) {
            auto fc = vectkit::read(path);

            if (fc.features.empty()) {
                throw std::runtime_error("Vector::fromFile: No features found in file");
            }

            std::optional<std::pair<dp::Polygon, std::unordered_map<std::string, std::string>>> field_data;

            for (const auto &feature : fc.features) {
                if (std::holds_alternative<dp::Polygon>(feature.geometry)) {
                    auto it = feature.properties.find("type");
                    if (it != feature.properties.end() && it->second == "field") {
                        field_data = std::make_pair(std::get<dp::Polygon>(feature.geometry), feature.properties);
                        break;
                    }
                }
            }

            if (!field_data) {
                for (const auto &feature : fc.features) {
                    if (std::holds_alternative<dp::Polygon>(feature.geometry)) {
                        field_data = std::make_pair(std::get<dp::Polygon>(feature.geometry), feature.properties);
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

        void toFile(const std::filesystem::path &path, CRS outputCrs = CRS::WGS) const {
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

            vectkit::write(fc, path, outputCrs);
        }

        const dp::Polygon &getFieldBoundary() const { return field_boundary_; }

        void setFieldBoundary(const dp::Polygon &boundary) { field_boundary_ = boundary; }

        const std::unordered_map<std::string, std::string> &getFieldProperties() const { return field_properties_; }

        void setFieldProperty(const std::string &key, const std::string &value) { field_properties_[key] = value; }

        void removeFieldProperty(const std::string &key) { field_properties_.erase(key); }

        size_t elementCount() const { return elements_.size(); }

        bool hasElements() const { return !elements_.empty(); }

        void clearElements() { elements_.clear(); }

        const Element &getElement(size_t index) const {
            if (index >= elements_.size())
                throw std::out_of_range("Element index out of range");
            return elements_[index];
        }

        Element &getElement(size_t index) {
            if (index >= elements_.size())
                throw std::out_of_range("Element index out of range");
            return elements_[index];
        }

        void addElement(const Geometry &geometry, const std::string &type = "",
                        const std::unordered_map<std::string, std::string> &properties = {}) {
            auto props = properties;
            if (!type.empty()) {
                props["type"] = type;
            }
            elements_.emplace_back(geometry, props, type);
        }

        void removeElement(size_t index) {
            if (index < elements_.size()) {
                elements_.erase(elements_.begin() + static_cast<std::ptrdiff_t>(index));
            }
        }

        void addPoint(const dp::Point &point, const std::string &type = "point",
                      const std::unordered_map<std::string, std::string> &properties = {}) {
            addElement(point, type, properties);
        }

        void addLine(const dp::Segment &line, const std::string &type = "line",
                     const std::unordered_map<std::string, std::string> &properties = {}) {
            addElement(line, type, properties);
        }

        void addPath(const std::vector<dp::Point> &path, const std::string &type = "path",
                     const std::unordered_map<std::string, std::string> &properties = {}) {
            addElement(path, type, properties);
        }

        void addPolygon(const dp::Polygon &polygon, const std::string &type = "polygon",
                        const std::unordered_map<std::string, std::string> &properties = {}) {
            addElement(polygon, type, properties);
        }

        std::vector<Element> getElementsByType(const std::string &type) const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                if (element.type == type) {
                    result.push_back(element);
                }
            }
            return result;
        }

        std::vector<Element> getPoints() const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                if (std::holds_alternative<dp::Point>(element.geometry)) {
                    result.push_back(element);
                }
            }
            return result;
        }

        std::vector<Element> getLines() const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                if (std::holds_alternative<dp::Segment>(element.geometry)) {
                    result.push_back(element);
                }
            }
            return result;
        }

        std::vector<Element> getPaths() const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                if (std::holds_alternative<std::vector<dp::Point>>(element.geometry)) {
                    result.push_back(element);
                }
            }
            return result;
        }

        std::vector<Element> getPolygons() const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                if (std::holds_alternative<dp::Polygon>(element.geometry)) {
                    result.push_back(element);
                }
            }
            return result;
        }

        std::vector<Element> filterByProperty(const std::string &key, const std::string &value) const {
            std::vector<Element> result;
            for (const auto &element : elements_) {
                auto it = element.properties.find(key);
                if (it != element.properties.end() && it->second == value) {
                    result.push_back(element);
                }
            }
            return result;
        }

        const dp::Geo &getDatum() const { return datum_; }

        void setDatum(const dp::Geo &datum) { datum_ = datum; }

        const dp::Euler &getHeading() const { return heading_; }

        void setHeading(const dp::Euler &heading) { heading_ = heading; }

        CRS getCRS() const { return crs_; }

        void setCRS(CRS crs) { crs_ = crs; }

        void setGlobalProperty(const std::string &key, const std::string &value) { global_properties_[key] = value; }

        std::string getGlobalProperty(const std::string &key, const std::string &default_value = "") const {
            auto it = global_properties_.find(key);
            return (it != global_properties_.end()) ? it->second : default_value;
        }

        const std::unordered_map<std::string, std::string> &getGlobalProperties() const { return global_properties_; }

        void removeGlobalProperty(const std::string &key) { global_properties_.erase(key); }

        auto begin() { return elements_.begin(); }
        auto end() { return elements_.end(); }
        auto begin() const { return elements_.begin(); }
        auto end() const { return elements_.end(); }
        auto cbegin() const { return elements_.cbegin(); }
        auto cend() const { return elements_.cend(); }
    };

} // namespace vectkit
