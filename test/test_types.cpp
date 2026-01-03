#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <variant>

namespace dp = ::datapod;

TEST_CASE("Types - Basic Geometry Variant") {
    SUBCASE("Point geometry") {
        dp::Geo datum{52.0, 5.0, 0.0};
        // Convert WGS to ENU to Point
        concord::earth::WGS wgsCoord{52.1, 5.1, 10.0};
        auto enu = concord::frame::to_enu(datum, wgsCoord);
        dp::Point point{enu.east(), enu.north(), enu.up()};

        geoson::Geometry geom = point;

        CHECK(std::holds_alternative<dp::Point>(geom));
        auto *p = std::get_if<dp::Point>(&geom);
        REQUIRE(p != nullptr);
        // Verify by converting back to WGS
        concord::frame::ENU pointEnu{*p, datum};
        auto backToWgs = concord::frame::to_wgs(pointEnu);
        CHECK(backToWgs.latitude == doctest::Approx(52.1));
        CHECK(backToWgs.longitude == doctest::Approx(5.1));
        CHECK(backToWgs.altitude == doctest::Approx(10.0));
    }

    SUBCASE("Line geometry") {
        dp::Geo datum{52.0, 5.0, 0.0};
        concord::earth::WGS wgsStart{52.1, 5.1, 0.0};
        concord::earth::WGS wgsEnd{52.2, 5.2, 0.0};
        auto enuStart = concord::frame::to_enu(datum, wgsStart);
        auto enuEnd = concord::frame::to_enu(datum, wgsEnd);
        dp::Point start{enuStart.east(), enuStart.north(), enuStart.up()};
        dp::Point end{enuEnd.east(), enuEnd.north(), enuEnd.up()};
        dp::Segment line{start, end};

        geoson::Geometry geom = line;

        CHECK(std::holds_alternative<dp::Segment>(geom));
    }

    SUBCASE("Path geometry") {
        dp::Geo datum{52.0, 5.0, 0.0};
        std::vector<dp::Point> points;
        std::vector<concord::earth::WGS> wgsPoints = {{52.1, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.3, 5.3, 0.0}};
        for (const auto &wgs : wgsPoints) {
            auto enu = concord::frame::to_enu(datum, wgs);
            points.emplace_back(enu.east(), enu.north(), enu.up());
        }

        geoson::Geometry geom = points;

        CHECK(std::holds_alternative<std::vector<dp::Point>>(geom));
    }

    SUBCASE("Polygon geometry") {
        dp::Geo datum{52.0, 5.0, 0.0};
        std::vector<dp::Point> points;
        std::vector<concord::earth::WGS> wgsPoints = {
            {52.1, 5.1, 0.0}, {52.2, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.1, 5.2, 0.0}, {52.1, 5.1, 0.0}};
        for (const auto &wgs : wgsPoints) {
            auto enu = concord::frame::to_enu(datum, wgs);
            points.emplace_back(enu.east(), enu.north(), enu.up());
        }
        dp::Polygon polygon{dp::Vector<dp::Point>{points.begin(), points.end()}};

        geoson::Geometry geom = polygon;

        CHECK(std::holds_alternative<dp::Polygon>(geom));
    }
}

TEST_CASE("Types - Feature") {
    dp::Geo datum{52.0, 5.0, 0.0};
    concord::earth::WGS wgsCoord{52.1, 5.1, 10.0};
    auto enu = concord::frame::to_enu(datum, wgsCoord);
    dp::Point point{enu.east(), enu.north(), enu.up()};

    std::unordered_map<std::string, std::string> properties;
    properties["name"] = "test_feature";
    properties["type"] = "point_of_interest";

    geoson::Feature feature{point, properties};

    CHECK(std::holds_alternative<dp::Point>(feature.geometry));
    CHECK(feature.properties.size() == 2);
    CHECK(feature.properties["name"] == "test_feature");
    CHECK(feature.properties["type"] == "point_of_interest");
}

TEST_CASE("Types - FeatureCollection") {
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0.0, 0.0, 2.0};

    // Create some features
    std::vector<geoson::Feature> features;

    // Add a point feature
    concord::earth::WGS wgsPoint{52.1, 5.1, 10.0};
    auto enuPoint = concord::frame::to_enu(datum, wgsPoint);
    dp::Point point{enuPoint.east(), enuPoint.north(), enuPoint.up()};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    // Add a line feature
    concord::earth::WGS wgsStart{52.1, 5.1, 0.0};
    concord::earth::WGS wgsEnd{52.2, 5.2, 0.0};
    auto enuStart = concord::frame::to_enu(datum, wgsStart);
    auto enuEnd = concord::frame::to_enu(datum, wgsEnd);
    dp::Point start{enuStart.east(), enuStart.north(), enuStart.up()};
    dp::Point end{enuEnd.east(), enuEnd.north(), enuEnd.up()};
    dp::Segment line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(geoson::Feature{line, lineProps});

    geoson::FeatureCollection fc{datum, heading, std::move(features), {}};

    // Note: Internal representation is always Point coordinates, no CRS stored
    CHECK(fc.datum.latitude == doctest::Approx(52.0));
    CHECK(fc.datum.longitude == doctest::Approx(5.0));
    CHECK(fc.datum.altitude == doctest::Approx(0.0));
    CHECK(fc.heading.yaw == doctest::Approx(2.0));
    CHECK(fc.features.size() == 2);

    // Check first feature (point)
    CHECK(std::holds_alternative<dp::Point>(fc.features[0].geometry));
    CHECK(fc.features[0].properties["name"] == "test_point");

    // Check second feature (line)
    CHECK(std::holds_alternative<dp::Segment>(fc.features[1].geometry));
    CHECK(fc.features[1].properties["name"] == "test_line");
}
