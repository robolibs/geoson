#include <doctest/doctest.h>

#include "vectkit/vectkit.hpp"
#include <filesystem>
#include <fstream>

namespace dp = ::datapod;

TEST_CASE("Writer - Write and read back") {
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0.0, 0.0, 2.0};

    // Create features using public API approach
    std::vector<vectkit::Feature> features;

    // Point feature
    concord::earth::WGS wgsPoint{52.1, 5.1, 10.0};
    auto enuPoint = concord::frame::to_enu(datum, wgsPoint);
    dp::Point point{enuPoint.east(), enuPoint.north(), enuPoint.up()};
    std::unordered_map<std::string, std::string> pointProps;
    pointProps["name"] = "test_point";
    features.emplace_back(vectkit::Feature{point, pointProps});

    // Line feature
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

    vectkit::FeatureCollection fc{datum, heading, std::move(features), {}};

    const std::filesystem::path test_file = "/tmp/test_output.geojson";

    SUBCASE("Write WGS format and read back") {
        vectkit::write(fc, test_file, vectkit::CRS::WGS);

        // Verify file exists
        CHECK(std::filesystem::exists(test_file));

        // Read back and verify content
        auto loaded_fc = vectkit::read(test_file);

        CHECK(loaded_fc.datum.latitude == doctest::Approx(52.0));
        CHECK(loaded_fc.datum.longitude == doctest::Approx(5.0));
        CHECK(loaded_fc.datum.altitude == doctest::Approx(0.0));
        CHECK(loaded_fc.heading.yaw == doctest::Approx(2.0));
        CHECK(loaded_fc.features.size() == 2);

        // Check point feature
        CHECK(std::holds_alternative<dp::Point>(loaded_fc.features[0].geometry));
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        // Check line feature
        CHECK(std::holds_alternative<dp::Segment>(loaded_fc.features[1].geometry));
        CHECK(loaded_fc.features[1].properties["name"] == "test_line");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Write ENU format and read back") {
        vectkit::write(fc, test_file, vectkit::CRS::ENU);

        // Verify file exists
        CHECK(std::filesystem::exists(test_file));

        // Read back and verify content
        auto loaded_fc = vectkit::read(test_file);

        CHECK(loaded_fc.datum.latitude == doctest::Approx(52.0));
        CHECK(loaded_fc.features.size() == 2);

        std::filesystem::remove(test_file);
    }

    SUBCASE("Write to invalid path throws") {
        CHECK_THROWS_AS(vectkit::write(fc, "/invalid/path/file.geojson"), std::runtime_error);
    }
}

TEST_CASE("Writer - Default CRS output") {
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0.0, 0.0, 1.5};

    std::vector<vectkit::Feature> features;
    concord::earth::WGS wgsCoord{52.1, 5.1, 10.0};
    auto enu = concord::frame::to_enu(datum, wgsCoord);
    dp::Point point{enu.east(), enu.north(), enu.up()};
    std::unordered_map<std::string, std::string> props;
    props["name"] = "test_point";
    features.emplace_back(vectkit::Feature{point, props});

    vectkit::FeatureCollection fc{datum, heading, std::move(features), {}};

    const std::filesystem::path test_file = "/tmp/test_default_output.geojson";

    SUBCASE("Default write (should be WGS)") {
        vectkit::write(fc, test_file); // No CRS specified - should default to WGS

        // Verify file exists and can be read back
        CHECK(std::filesystem::exists(test_file));

        auto loaded_fc = vectkit::read(test_file);
        CHECK(loaded_fc.features.size() == 1);
        CHECK(loaded_fc.features[0].properties["name"] == "test_point");

        std::filesystem::remove(test_file);
    }
}
