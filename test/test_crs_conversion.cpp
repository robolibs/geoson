#include "doctest/doctest.h"
#include "vectkit/vectkit.hpp"
#include <filesystem>
#include <fstream>

namespace dp = ::datapod;

TEST_CASE("CRS conversion during output") {
    // Create a simple test GeoJSON with WGS coordinates
    const std::string test_geojson_str = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [5.0, 52.0, 100.0],
            "heading": 45.0
        },
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [5.1, 52.1, 105.0]
                },
                "properties": {"name": "test_point"}
            }
        ]
    })";

    // Write test file
    std::ofstream ofs("test_crs_input.geojson");
    ofs << test_geojson_str;
    ofs.close();

    SUBCASE("Parse and verify internal representation") {
        auto fc = vectkit::read("test_crs_input.geojson");

        // Check that we parsed correctly
        CHECK(fc.features.size() == 1);

        // Internal representation should be in Point coordinates
        auto *point = std::get_if<dp::Point>(&fc.features[0].geometry);
        REQUIRE(point != nullptr);

        // The coordinates should have been converted from WGS to ENU/Point coordinates
        CHECK(point->x != 5.1);  // Should be different from original lon
        CHECK(point->y != 52.1); // Should be different from original lat
    }

    SUBCASE("Output in different CRS formats") {
        auto fc = vectkit::read("test_crs_input.geojson");

        // Test output in WGS format
        vectkit::write(fc, "test_output_wgs.geojson", vectkit::CRS::WGS);
        auto fc_wgs = vectkit::read("test_output_wgs.geojson");

        // Test output in ENU format
        vectkit::write(fc, "test_output_enu.geojson", vectkit::CRS::ENU);
        auto fc_enu = vectkit::read("test_output_enu.geojson");

        // Both should have the same internal representation after parsing
        auto *point_wgs = std::get_if<dp::Point>(&fc_wgs.features[0].geometry);
        auto *point_enu = std::get_if<dp::Point>(&fc_enu.features[0].geometry);
        REQUIRE(point_wgs != nullptr);
        REQUIRE(point_enu != nullptr);

        // Internal coordinates should be approximately the same (within floating point precision)
        CHECK(std::abs(point_wgs->x - point_enu->x) < 1e-6);
        CHECK(std::abs(point_wgs->y - point_enu->y) < 1e-6);
        CHECK(std::abs(point_wgs->z - point_enu->z) < 1e-6);
    }

    // Cleanup
    std::filesystem::remove("test_crs_input.geojson");
    std::filesystem::remove("test_output_wgs.geojson");
    std::filesystem::remove("test_output_enu.geojson");
}