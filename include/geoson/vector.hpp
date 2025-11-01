#pragma once

#include "geoson/types.hpp"
#include <filesystem>
#include <functional>
#include <optional>
#include <stdexcept>

namespace geoson {

    struct Element {
        Geometry geometry;
        std::unordered_map<std::string, std::string> properties;
        std::string type;

        Element(const Geometry &geom, const std::unordered_map<std::string, std::string> &props = {},
                const std::string &elem_type = "");
    };

    class Vector {
      private:
        concord::Polygon field_boundary_;
        std::unordered_map<std::string, std::string> field_properties_;
        std::vector<Element> elements_;

        concord::Datum datum_;
        concord::Euler heading_;
        CRS crs_;

        std::unordered_map<std::string, std::string> global_properties_;

      public:
        Vector() = delete;

        explicit Vector(const concord::Polygon &field_boundary,
                        const concord::Datum &datum = concord::Datum{0.001, 0.001, 1.0},
                        const concord::Euler &heading = concord::Euler{0, 0, 0}, CRS crs = CRS::ENU);

        static Vector fromFile(const std::filesystem::path &path);

        void toFile(const std::filesystem::path &path, CRS outputCrs = CRS::WGS) const;

        const concord::Polygon &getFieldBoundary() const;
        void setFieldBoundary(const concord::Polygon &boundary);

        const std::unordered_map<std::string, std::string> &getFieldProperties() const;
        void setFieldProperty(const std::string &key, const std::string &value);
        void removeFieldProperty(const std::string &key);

        size_t elementCount() const;
        bool hasElements() const;
        void clearElements();

        const Element &getElement(size_t index) const;
        Element &getElement(size_t index);

        void addElement(const Geometry &geometry, const std::string &type = "",
                        const std::unordered_map<std::string, std::string> &properties = {});

        void removeElement(size_t index);

        void addPoint(const concord::Point &point, const std::string &type = "point",
                      const std::unordered_map<std::string, std::string> &properties = {});

        void addLine(const concord::Line &line, const std::string &type = "line",
                     const std::unordered_map<std::string, std::string> &properties = {});

        void addPath(const concord::Path &path, const std::string &type = "path",
                     const std::unordered_map<std::string, std::string> &properties = {});

        void addPolygon(const concord::Polygon &polygon, const std::string &type = "polygon",
                        const std::unordered_map<std::string, std::string> &properties = {});

        std::vector<Element> getElementsByType(const std::string &type) const;

        std::vector<Element> getPoints() const;

        std::vector<Element> getLines() const;

        std::vector<Element> getPaths() const;

        std::vector<Element> getPolygons() const;

        std::vector<Element> filterByProperty(const std::string &key, const std::string &value) const;

        const concord::Datum &getDatum() const;
        void setDatum(const concord::Datum &datum);

        const concord::Euler &getHeading() const;
        void setHeading(const concord::Euler &heading);

        CRS getCRS() const;
        void setCRS(CRS crs);

        void setGlobalProperty(const std::string &key, const std::string &value);
        std::string getGlobalProperty(const std::string &key, const std::string &default_value = "") const;
        const std::unordered_map<std::string, std::string> &getGlobalProperties() const;
        void removeGlobalProperty(const std::string &key);

        auto begin() { return elements_.begin(); }
        auto end() { return elements_.end(); }
        auto begin() const { return elements_.begin(); }
        auto end() const { return elements_.end(); }
        auto cbegin() const { return elements_.cbegin(); }
        auto cend() const { return elements_.cend(); }
    };

} // namespace geoson
