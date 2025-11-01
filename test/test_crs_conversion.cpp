#include "doctest/doctest.h"
#include "geoson/geoson.hpp"
#include <filesystem>
#include <fstream>

TEST_CASE("CRS conversion during output") {
    // Create a simple test GeoJSON with WGS coordinates
    const std::string test_geojson_str = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [52.0, 5.0, 100.0],
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
        auto fc = geoson::read("test_crs_input.geojson");

        // Check that we parsed correctly
        CHECK(fc.features.size() == 1);

        // Internal representation should be in Point coordinates
        auto *point = std::get_if<concord::Point>(&fc.features[0].geometry);
        REQUIRE(point != nullptr);

        // The coordinates should have been converted from WGS to ENU/Point coordinates
        CHECK(point->x != 5.1);  // Should be different from original lon
        CHECK(point->y != 52.1); // Should be different from original lat
    }

    SUBCASE("Output in different CRS formats") {
        auto fc = geoson::read("test_crs_input.geojson");

        // Test output in WGS format
        geoson::write(fc, "test_output_wgs.geojson", geoson::CRS::WGS);
        auto fc_wgs = geoson::read("test_output_wgs.geojson");

        // Test output in ENU format
        geoson::write(fc, "test_output_enu.geojson", geoson::CRS::ENU);
        auto fc_enu = geoson::read("test_output_enu.geojson");

        // Both should have the same internal representation after parsing
        auto *point_wgs = std::get_if<concord::Point>(&fc_wgs.features[0].geometry);
        auto *point_enu = std::get_if<concord::Point>(&fc_enu.features[0].geometry);
        REQUIRE(point_wgs != nullptr);
        REQUIRE(point_enu != nullptr);

        // Internal coordinates should be approximately the same (within floating point precision)
        CHECK(std::abs(point_wgs->x - point_enu->x) < 1e-10);
        CHECK(std::abs(point_wgs->y - point_enu->y) < 1e-10);
        CHECK(std::abs(point_wgs->z - point_enu->z) < 1e-10);
    }

    // Cleanup
    std::filesystem::remove("test_crs_input.geojson");
    std::filesystem::remove("test_output_wgs.geojson");
    std::filesystem::remove("test_output_enu.geojson");
}