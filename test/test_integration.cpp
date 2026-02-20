#include <doctest/doctest.h>

#include "vectkit/vectkit.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace dp = ::datapod;

TEST_CASE("Integration - Round-trip conversion") {
    // Create original feature collection
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0.0, 0.0, 2.0};

    std::vector<vectkit::Feature> features;

    // Add various geometry types
    concord::earth::WGS wgsPoint{52.1, 5.1, 10.0};
    auto enuPoint = concord::frame::to_enu(datum, wgsPoint);
    dp::Point point{enuPoint.east(), enuPoint.north(), enuPoint.up()};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    pointProps["category"] = "landmark";
    features.emplace_back(vectkit::Feature{point, pointProps});

    concord::earth::WGS wgsStart{52.1, 5.1, 0.0};
    concord::earth::WGS wgsEnd{52.2, 5.2, 0.0};
    auto enuStart = concord::frame::to_enu(datum, wgsStart);
    auto enuEnd = concord::frame::to_enu(datum, wgsEnd);
    dp::Point start{enuStart.east(), enuStart.north(), enuStart.up()};
    dp::Point end{enuEnd.east(), enuEnd.north(), enuEnd.up()};
    dp::Segment line{start, end};
    std::unordered_map<std::string, std::string> lineProps;
    lineProps["name"] = "test_line";
    features.emplace_back(vectkit::Feature{line, lineProps});

    // Path feature
    std::vector<dp::Point> pathPoints;
    std::vector<concord::earth::WGS> pathWgsPoints = {{52.1, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.3, 5.3, 0.0}};
    for (const auto &wgs : pathWgsPoints) {
        auto enu = concord::frame::to_enu(datum, wgs);
        pathPoints.emplace_back(enu.east(), enu.north(), enu.up());
    }
    std::unordered_map<std::string, std::string> pathProps;
    pathProps["name"] = "test_path";
    features.emplace_back(vectkit::Feature{pathPoints, pathProps});

    // Polygon feature
    std::vector<dp::Point> polygonPoints;
    std::vector<concord::earth::WGS> polygonWgsPoints = {
        {52.1, 5.1, 0.0}, {52.2, 5.1, 0.0}, {52.2, 5.2, 0.0}, {52.1, 5.2, 0.0}, {52.1, 5.1, 0.0}};
    for (const auto &wgs : polygonWgsPoints) {
        auto enu = concord::frame::to_enu(datum, wgs);
        polygonPoints.emplace_back(enu.east(), enu.north(), enu.up());
    }
    dp::Polygon polygon{dp::Vector<dp::Point>{polygonPoints.begin(), polygonPoints.end()}};
    std::unordered_map<std::string, std::string> polygonProps;
    polygonProps["name"] = "test_polygon";
    features.emplace_back(vectkit::Feature{polygon, polygonProps});

    vectkit::FeatureCollection original{datum, heading, std::move(features), {}};

    // Write to file
    const std::filesystem::path test_file = "/tmp/round_trip_test.geojson";
    vectkit::WriteFeatureCollection(original, test_file);

    // Read back from file
    auto loaded = vectkit::ReadFeatureCollection(test_file);

    // Verify the content matches
    // Note: No CRS comparison since internal representation is always Point coordinates
    CHECK(loaded.datum.latitude == doctest::Approx(original.datum.latitude));
    CHECK(loaded.datum.longitude == doctest::Approx(original.datum.longitude));
    CHECK(loaded.datum.altitude == doctest::Approx(original.datum.altitude));
    CHECK(loaded.heading.yaw == doctest::Approx(original.heading.yaw));
    CHECK(loaded.features.size() == original.features.size());

    // Clean up
    std::filesystem::remove(test_file);
}

TEST_CASE("Integration - Read existing GeoJSON file") {
    // This test assumes there's a test file in misc/
    auto fc = vectkit::ReadFeatureCollection(PROJECT_DIR "/misc/field4.geojson");

    // Note: datum in file is [lon, lat, alt] = [5.650, 51.9877, 0.0]
    // dp::Geo stores as {latitude, longitude, altitude}
    CHECK(fc.datum.latitude == doctest::Approx(51.9877));
    CHECK(fc.datum.longitude == doctest::Approx(5.65));
    CHECK(fc.datum.altitude == doctest::Approx(0.0));
    CHECK(fc.heading.yaw == doctest::Approx(0.0));
    CHECK(fc.features.size() == 1);

    // Check first feature is a polygon
    CHECK(std::holds_alternative<dp::Polygon>(fc.features[0].geometry));
}

TEST_CASE("Integration - Modify and save") {
    // Load a file
    auto fc = vectkit::ReadFeatureCollection(PROJECT_DIR "/misc/field4.geojson");

    // Modify the datum
    fc.datum.latitude += 5.1;

    // Save it
    const std::filesystem::path output_file = "/tmp/modified_test.geojson";
    vectkit::WriteFeatureCollection(fc, output_file);

    // Read it back
    auto modified = vectkit::ReadFeatureCollection(output_file);

    // Verify the modification
    CHECK(modified.datum.latitude == doctest::Approx(57.0877)); // 51.9877 + 5.1

    // Clean up
    std::filesystem::remove(output_file);
}

TEST_CASE("Integration - CRS flavor handling") {
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0.0, 0.0, 1.5};

    SUBCASE("WGS flavor - coordinates should be converted") {
        std::vector<vectkit::Feature> features;

        // Create point using WGS coordinates -> ENU -> Point
        concord::earth::WGS wgsCoord{52.1, 5.1, 10.0};
        auto enu = concord::frame::to_enu(datum, wgsCoord);
        dp::Point point{enu.east(), enu.north(), enu.up()};
        std::unordered_map<std::string, std::string> props;
        props["name"] = "test_point";
        features.emplace_back(vectkit::Feature{point, props});

        vectkit::FeatureCollection fc{datum, heading, std::move(features), {}};

        // Write to file with WGS output and verify by reading back
        const std::filesystem::path test_file = "/tmp/test_wgs_output.geojson";
        vectkit::write(fc, test_file, vectkit::CRS::WGS);

        // Read the file back to verify WGS output
        auto loaded_fc = vectkit::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }

    SUBCASE("ENU flavor - coordinates should be direct") {
        std::vector<vectkit::Feature> features;

        // Create point with direct ENU coordinates
        dp::Point point{100.0, 200.0, 10.0}; // Direct x,y,z
        std::unordered_map<std::string, std::string> props;
        props["name"] = "test_point";
        features.emplace_back(vectkit::Feature{point, props});

        vectkit::FeatureCollection fc{datum, heading, std::move(features), {}};

        // Write to file with ENU output and verify by reading back
        const std::filesystem::path test_file = "/tmp/test_enu_output.geojson";
        vectkit::write(fc, test_file, vectkit::CRS::ENU);

        // Read the file back to verify ENU output
        auto loaded_fc = vectkit::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }
}
