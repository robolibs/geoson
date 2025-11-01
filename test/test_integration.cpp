#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

TEST_CASE("Integration - Round-trip conversion") {
    // Create original feature collection
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    std::vector<geoson::Feature> features;

    // Add various geometry types
    concord::WGS wgsPoint{52.1, 5.1, 10.0};
    concord::ENU enuPoint = wgsPoint.toENU(datum);
    concord::Point point{enuPoint.x, enuPoint.y, enuPoint.z};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    pointProps["category"] = "landmark";
    features.emplace_back(geoson::Feature{point, pointProps});

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

    // Path feature
    std::vector<concord::Point> pathPoints;
    std::vector<concord::WGS> pathWgsPoints = {{52.1, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.3, 5.3, 0.0}};
    for (const auto &wgs : pathWgsPoints) {
        concord::ENU enu = wgs.toENU(datum);
        pathPoints.emplace_back(enu.x, enu.y, enu.z);
    }
    concord::Path path{pathPoints};
    std::unordered_map<std::string, std::string> pathProps;
    pathProps["name"] = "test_path";
    features.emplace_back(geoson::Feature{path, pathProps});

    // Polygon feature
    std::vector<concord::Point> polygonPoints;
    std::vector<concord::WGS> polygonWgsPoints = {
        {52.1, 5.1, 0.0}, {52.2, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.1, 5.2, 0.0}, {52.1, 5.1, 0.0}};
    for (const auto &wgs : polygonWgsPoints) {
        concord::ENU enu = wgs.toENU(datum);
        polygonPoints.emplace_back(enu.x, enu.y, enu.z);
    }
    concord::Polygon polygon{polygonPoints};
    std::unordered_map<std::string, std::string> polygonProps;
    polygonProps["name"] = "test_polygon";
    features.emplace_back(geoson::Feature{polygon, polygonProps});

    geoson::FeatureCollection original{datum, heading, std::move(features), {}};

    // Write to file
    const std::filesystem::path test_file = "/tmp/round_trip_test.geojson";
    geoson::WriteFeatureCollection(original, test_file);

    // Read back from file
    auto loaded = geoson::ReadFeatureCollection(test_file);

    // Verify the content matches
    // Note: No CRS comparison since internal representation is always Point coordinates
    CHECK(loaded.datum.lat == doctest::Approx(original.datum.lat));
    CHECK(loaded.datum.lon == doctest::Approx(original.datum.lon));
    CHECK(loaded.datum.alt == doctest::Approx(original.datum.alt));
    CHECK(loaded.heading.yaw == doctest::Approx(original.heading.yaw));
    CHECK(loaded.features.size() == original.features.size());

    // Clean up
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Read existing GeoJSON file") {
    // This test assumes there's a test file in misc/
    auto fc = geoson::ReadFeatureCollection("../misc/field4.geojson");

    // Note: Internal representation is always Point coordinates, no CRS stored
    CHECK(fc.datum.lat == doctest::Approx(72.4)); // File was modified by main example
    CHECK(fc.datum.lon == doctest::Approx(4.4));
    CHECK(fc.datum.alt == doctest::Approx(50));
    CHECK(fc.heading.yaw == doctest::Approx(2));
    CHECK(fc.features.size() == 1);

    // Check first feature is a polygon
    CHECK(std::holds_alternative<concord::Polygon>(fc.features[0].geometry));
}

TEST_CASE("Integration - Modify and save") {
    // Load a file
    auto fc = geoson::ReadFeatureCollection("../misc/field4.geojson");

    // Modify the datum
    fc.datum.lat += 5.1;

    // Save it
    const std::filesystem::path output_file = "/tmp/modified_test.geojson";
    geoson::WriteFeatureCollection(fc, output_file);

    // Read it back
    auto modified = geoson::ReadFeatureCollection(output_file);

    // Verify the modification
    CHECK(modified.datum.lat == doctest::Approx(77.5)); // 72.4 + 5.1

    // Clean up
    std::filesystem::remove(output_file);
}

TEST_CASE("Integration - CRS flavor handling") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 1.5};

    SUBCASE("WGS flavor - coordinates should be converted") {
        std::vector<geoson::Feature> features;

        // Create point using WGS coordinates -> ENU -> Point
        concord::WGS wgsCoord{52.1, 5.1, 10.0};
        concord::ENU enu = wgsCoord.toENU(datum);
        concord::Point point{enu.x, enu.y, enu.z};
        std::unordered_map<std::string, std::string> props;
        props["name"] = "test_point";
        features.emplace_back(geoson::Feature{point, props});

        geoson::FeatureCollection fc{datum, heading, std::move(features), {}};

        // Write to file with WGS output and verify by reading back
        const std::filesystem::path test_file = "/tmp/test_wgs_output.geojson";
        geoson::write(fc, test_file, geoson::CRS::WGS);

        // Read the file back to verify WGS output
        auto loaded_fc = geoson::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }

    SUBCASE("ENU flavor - coordinates should be direct") {
        std::vector<geoson::Feature> features;

        // Create point with direct ENU coordinates
        concord::Point point{100.0, 200.0, 10.0}; // Direct x,y,z
        std::unordered_map<std::string, std::string> props;
        props["name"] = "test_point";
        features.emplace_back(geoson::Feature{point, props});

        geoson::FeatureCollection fc{datum, heading, std::move(features), {}};

        // Write to file with ENU output and verify by reading back
        const std::filesystem::path test_file = "/tmp/test_enu_output.geojson";
        geoson::write(fc, test_file, geoson::CRS::ENU);

        // Read the file back to verify ENU output
        auto loaded_fc = geoson::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }
}
