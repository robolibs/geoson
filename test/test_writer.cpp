#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>

TEST_CASE("Writer - Write and read back") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 2.0};

    // Create features using public API approach
    std::vector<geoson::Feature> features;

    // Point feature
    concord::WGS wgsPoint{52.1, 5.1, 10.0};
    concord::ENU enuPoint = wgsPoint.toENU(datum);
    concord::Point point{enuPoint.x, enuPoint.y, enuPoint.z};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, pointProps});

    // Line feature
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

    const std::filesystem::path test_file = "/tmp/test_output.geojson";

    SUBCASE("Write WGS format and read back") {
        geoson::write(fc, test_file, geoson::CRS::WGS);

        // Verify file exists
        CHECK(std::filesystem::exists(test_file));

        // Read back and verify content
        auto loaded_fc = geoson::read(test_file);

        CHECK(loaded_fc.datum.lat == doctest::Approx(52.0));
        CHECK(loaded_fc.datum.lon == doctest::Approx(5.0));
        CHECK(loaded_fc.datum.alt == doctest::Approx(0.0));
        CHECK(loaded_fc.heading.yaw == doctest::Approx(2.0));
        CHECK(loaded_fc.features.size() == 2);

        // Check point feature
        CHECK(std::holds_alternative<concord::Point>(loaded_fc.features[0].geometry));
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        // Check line feature
        CHECK(std::holds_alternative<concord::Line>(loaded_fc.features[1].geometry));
        CHECK(loaded_fc.features[1].properties["name"] == "test_line");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Write ENU format and read back") {
        geoson::write(fc, test_file, geoson::CRS::ENU);

        // Verify file exists
        CHECK(std::filesystem::exists(test_file));

        // Read back and verify content
        auto loaded_fc = geoson::read(test_file);

        CHECK(loaded_fc.datum.lat == doctest::Approx(52.0));
        CHECK(loaded_fc.features.size() == 2);

        std::filesystem::remove(test_file);
    }

    SUBCASE("Write to invalid path throws") {
        CHECK_THROWS_AS(geoson::write(fc, "/invalid/path/file.geojson"), std::runtime_error);
    }
}

TEST_CASE("Writer - Default CRS output") {
    concord::Datum datum{52.0, 5.0, 0.0};
    concord::Euler heading{0.0, 0.0, 1.5};

    std::vector<geoson::Feature> features;
    concord::WGS wgsCoord{52.1, 5.1, 10.0};
    concord::ENU enu = wgsCoord.toENU(datum);
    concord::Point point{enu.x, enu.y, enu.z};
    std::unordered_map<std::string, std::string> props;
    props["name"] = "test_point";
    features.emplace_back(geoson::Feature{point, props});

    geoson::FeatureCollection fc{datum, heading, std::move(features), {}};

    const std::filesystem::path test_file = "/tmp/test_default_output.geojson";

    SUBCASE("Default write (should be WGS)") {
        geoson::write(fc, test_file); // No CRS specified - should default to WGS

        // Verify file exists and can be read back
        CHECK(std::filesystem::exists(test_file));

        auto loaded_fc = geoson::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }
}