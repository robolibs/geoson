#include <doctest/doctest.h>

#include "vectkit/vectkit.hpp"
#include <filesystem>
#include <fstream>

namespace dp = ::datapod;

TEST_CASE("Parser - Properties parsing through file") {
    const std::string test_file_content = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [5.0, 52.0, 0.0],
            "heading": 2.0
        },
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [5.1, 52.1, 10.0]
                },
                "properties": {
                    "name": "test_point",
                    "number": 42,
                    "boolean": true
                }
            }
        ]
    })";

    const std::filesystem::path test_file = "/tmp/test_properties.geojson";
    std::ofstream ofs(test_file);
    ofs << test_file_content;
    ofs.close();

    auto fc = vectkit::ReadFeatureCollection(test_file);

    CHECK(fc.features.size() == 1);
    const auto &feature = fc.features[0];
    CHECK(feature.properties.at("name") == "test_point");
    CHECK(feature.properties.at("number") == "42");
    CHECK(feature.properties.at("boolean") == "true");

    std::filesystem::remove(test_file);
}

TEST_CASE("Parser - Different geometry types") {
    SUBCASE("Point geometry") {
        const std::string test_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 2.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1, 52.1, 10.0]
                    },
                    "properties": {"name": "test_point"}
                }
            ]
        })";

        const std::filesystem::path test_file = "/tmp/test_point.geojson";
        std::ofstream ofs(test_file);
        ofs << test_content;
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);
        CHECK(fc.features.size() == 1);
        CHECK(std::holds_alternative<dp::Point>(fc.features[0].geometry));

        std::filesystem::remove(test_file);
    }

    SUBCASE("LineString geometry") {
        const std::string test_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 2.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "LineString",
                        "coordinates": [[5.1, 52.1, 0.0], [5.2, 52.2, 0.0]]
                    },
                    "properties": {"name": "test_line"}
                }
            ]
        })";

        const std::filesystem::path test_file = "/tmp/test_line.geojson";
        std::ofstream ofs(test_file);
        ofs << test_content;
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);
        CHECK(fc.features.size() == 1);
        CHECK(std::holds_alternative<dp::Segment>(fc.features[0].geometry));

        std::filesystem::remove(test_file);
    }

    SUBCASE("Polygon geometry") {
        const std::string test_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 2.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Polygon",
                        "coordinates": [[[5.1, 52.1, 0.0], [5.2, 52.1, 0.0], [5.2, 52.2, 0.0], [5.1, 52.2, 0.0], [5.1, 52.1, 0.0]]]
                    },
                    "properties": {"name": "test_polygon"}
                }
            ]
        })";

        const std::filesystem::path test_file = "/tmp/test_polygon.geojson";
        std::ofstream ofs(test_file);
        ofs << test_content;
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);
        CHECK(fc.features.size() == 1);
        CHECK(std::holds_alternative<dp::Polygon>(fc.features[0].geometry));

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Parser - File operations") {
    const std::string test_file_content = R"({
        "type": "FeatureCollection",
        "properties": {
            "crs": "EPSG:4326",
            "datum": [5.0, 52.0, 0.0],
            "heading": 2.0
        },
        "features": [
            {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates": [5.1, 52.1, 10.0]
                },
                "properties": {
                    "name": "test_point"
                }
            }
        ]
    })";

    const std::filesystem::path test_file = "/tmp/test_vectkit.geojson";

    SUBCASE("ReadFeatureCollection - valid file") {
        std::ofstream ofs(test_file);
        ofs << test_file_content;
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);

        CHECK(fc.datum.latitude == doctest::Approx(52.0));
        CHECK(fc.datum.longitude == doctest::Approx(5.0));
        CHECK(fc.datum.altitude == doctest::Approx(0.0));
        CHECK(fc.heading.yaw == doctest::Approx(2.0));
        CHECK(fc.features.size() == 1);

        const auto &feature = fc.features[0];
        CHECK(std::holds_alternative<dp::Point>(feature.geometry));
        CHECK(feature.properties.at("name") == "test_point");

        std::filesystem::remove(test_file);
    }

    SUBCASE("ReadFeatureCollection - nonexistent file throws") {
        CHECK_THROWS_AS(vectkit::ReadFeatureCollection("/nonexistent/file.geojson"), std::runtime_error);
    }

    SUBCASE("ReadFeatureCollection - missing properties throws") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "features": []
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        std::filesystem::remove(test_file);
    }
}