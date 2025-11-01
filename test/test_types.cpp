#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <variant>

TEST_CASE("Types - Basic Geometry Variant") {
    SUBCASE("Point geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        // Convert WGS to ENU to Point
        concord::WGS wgsCoord{52.1, 5.1, 10.0};
        concord::ENU enu = wgsCoord.toENU(datum);
        concord::Point point{enu.x, enu.y, enu.z};

        geoson::Geometry geom = point;

        CHECK(std::holds_alternative<concord::Point>(geom));
        auto *p = std::get_if<concord::Point>(&geom);
        REQUIRE(p != nullptr);
        // Verify by converting back to WGS
        concord::ENU pointEnu{*p, datum};
        concord::WGS backToWgs = pointEnu.toWGS();
        CHECK(backToWgs.lat == doctest::Approx(52.1));
        CHECK(backToWgs.lon == doctest::Approx(5.1));
        CHECK(backToWgs.alt == doctest::Approx(10.0));
    }

    SUBCASE("Line geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        concord::WGS wgsStart{52.1, 5.1, 0.0};
        concord::WGS wgsEnd{52.2, 5.2, 0.0};
        concord::ENU enuStart = wgsStart.toENU(datum);
        concord::ENU enuEnd = wgsEnd.toENU(datum);
        concord::Point start{enuStart.x, enuStart.y, enuStart.z};
        concord::Point end{enuEnd.x, enuEnd.y, enuEnd.z};
        concord::Line line{start, end};

        geoson::Geometry geom = line;

        CHECK(std::holds_alternative<concord::Line>(geom));
    }

    SUBCASE("Path geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        std::vector<concord::Point> points;
        std::vector<concord::WGS> wgsPoints = {{52.1, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.3, 5.3, 0.0}};
        for (const auto &wgs : wgsPoints) {
            concord::ENU enu = wgs.toENU(datum);
            points.emplace_back(enu.x, enu.y, enu.z);
        }
        concord::Path path{points};

        geoson::Geometry geom = path;

        CHECK(std::holds_alternative<concord::Path>(geom));
    }

    SUBCASE("Polygon geometry") {
        concord::Datum datum{52.0, 5.0, 0.0};
        std::vector<concord::Point> points;
        std::vector<concord::WGS> wgsPoints = {
            {52.1, 5.1, 0.0}, {52.2, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.1, 5.2, 0.0}, {52.1, 5.1, 0.0}};
        for (const auto &wgs : wgsPoints) {
            concord::ENU enu = wgs.toENU(datum);
            points.emplace_back(enu.x, enu.y, enu.z);
        }
        concord::Polygon polygon{points};

        geoson::Geometry geom = polygon;

        CHECK(std::holds_alternative<concord::Polygon>(geom));
    }
}

TEST_CASE("Types - Feature") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::WGS wgsCoord{52.1, 5.1, 10.0};
    concord::ENU enu = wgsCoord.toENU(datum);
    concord::Point point{enu.x, enu.y, enu.z};

    std::unordered_map<std::string, std::string> properties;
    properties["name"] = "test_feature";
    properties["type"] = "point_of_interest";

    geoson::Feature feature{point, properties};

    CHECK(std::holds_alternative<concord::Point>(feature.geometry));
    CHECK(feature.properties.size() == 2);
    CHECK(feature.properties["name"] == "test_feature");
    CHECK(feature.properties["type"] == "point_of_interest");
}

TEST_CASE("Types - FeatureCollection") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    // Create some features
    std::vector<geoson::Feature> features;

    // Add a point feature
    concord::WGS wgsPoint{52.1, 5.1, 10.0};
    concord::ENU enuPoint = wgsPoint.toENU(datum);
    concord::Point point{enuPoint.x, enuPoint.y, enuPoint.z};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    // Add a line feature
    concord::WGS wgsStart{52.1, 5.1, 0.0};
    concord::WGS wgsEnd{52.2, 5.2, 0.0};
    concord::ENU enuStart = wgsStart.toENU(datum);
    concord::ENU enuEnd = wgsEnd.toENU(datum);
    concord::Point start{enuStart.x, enuStart.y, enuStart.z};
    concord::Point end{enuEnd.x, enuEnd.y, enuEnd.z};
    concord::Line line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(geoson::Feature{line, lineProps});

    geoson::FeatureCollection fc{datum, heading, std::move(features), {}};

    // Note: Internal representation is always Point coordinates, no CRS stored
    CHECK(fc.datum.lat == doctest::Approx(52.0));
    CHECK(fc.datum.lon == doctest::Approx(5.0));
    CHECK(fc.datum.alt == doctest::Approx(0.0));
    CHECK(fc.heading.yaw == doctest::Approx(2.0));
    CHECK(fc.features.size() == 2);

    // Check first feature (point)
    CHECK(std::holds_alternative<concord::Point>(fc.features[0].geometry));
    CHECK(fc.features[0].properties["name"] == "test_point");

    // Check second feature (line)
    CHECK(std::holds_alternative<concord::Line>(fc.features[1].geometry));
    CHECK(fc.features[1].properties["name"] == "test_line");
}
