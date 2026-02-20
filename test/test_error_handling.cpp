#include <doctest/doctest.h>

#include "vectkit/vectkit.hpp"
#include <filesystem>
#include <fstream>

namespace dp = ::datapod;

TEST_CASE("Error Handling - Invalid JSON") {
    const std::filesystem::path test_file = "/tmp/invalid.geojson";

    SUBCASE("Malformed JSON") {
        std::ofstream ofs(test_file);
        ofs << "{ invalid json content }";
        ofs.close();

        CHECK_THROWS(vectkit::ReadFeatureCollection(test_file));

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing type field") {
        std::ofstream ofs(test_file);
        ofs << R"({"features": []})";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file),
                          "vectkit::ReadFeatureCollection(): top-level object has no string 'type' field");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-string type field") {
        std::ofstream ofs(test_file);
        ofs << R"({"type": 123, "features": []})";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file),
                          "vectkit::ReadFeatureCollection(): top-level object has no string 'type' field");

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - Missing required properties") {
    const std::filesystem::path test_file = "/tmp/missing_props.geojson";

    SUBCASE("Missing properties object") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Properties not an object") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": "invalid",
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "missing top-level 'properties'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing CRS") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing string 'crs'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-string CRS") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": 123,
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing string 'crs'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing datum") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Invalid datum - not array") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": "invalid",
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Invalid datum - too few elements") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0],
                "heading": 0.0
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing array 'datum' of ≥3 numbers");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing heading") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0]
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing numeric 'heading'");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Non-numeric heading") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": "invalid"
            },
            "features": []
        })";
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "'properties' missing numeric 'heading'");

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - Invalid geometry in files") {
    const std::filesystem::path test_file = "/tmp/invalid_geom.geojson";

    SUBCASE("Invalid point coordinates - too few") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1]
                    },
                    "properties": {}
                }
            ]
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        CHECK_THROWS(vectkit::ReadFeatureCollection(test_file));

        std::filesystem::remove(test_file);
    }

    SUBCASE("Invalid geometry type") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature", 
                    "geometry": {
                        "type": "InvalidType",
                        "coordinates": [5.1, 52.1]
                    },
                    "properties": {}
                }
            ]
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        // Should not throw but may result in empty geometry list
        auto fc = vectkit::ReadFeatureCollection(test_file);
        // The feature should be skipped or handled gracefully

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - File I/O errors") {
    SUBCASE("ReadFeatureCollection - nonexistent file") {
        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection("/nonexistent/path/file.geojson"),
                          doctest::Contains("vectkit::ReadFeatureCollection(): cannot open"));
    }

    SUBCASE("WriteFeatureCollection - invalid directory") {
        dp::Geo datum{52.0, 5.0, 0.0};
        dp::Euler heading{0.0, 0.0, 0.0};
        std::vector<vectkit::Feature> features;

        vectkit::FeatureCollection fc{datum, heading, std::move(features), {}};

        CHECK_THROWS_WITH(vectkit::WriteFeatureCollection(fc, "/nonexistent/directory/file.geojson"),
                          doctest::Contains("Cannot open for write"));
    }
}

TEST_CASE("Error Handling - Unknown CRS in files") {
    const std::filesystem::path test_file = "/tmp/unknown_crs.geojson";

    SUBCASE("Unknown CRS string") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "UNKNOWN:12345",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "Unknown CRS string: UNKNOWN:12345");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Case sensitivity") {
        const std::string invalid_content = R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "epsg:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": []
        })";

        std::ofstream ofs(test_file);
        ofs << invalid_content;
        ofs.close();

        CHECK_THROWS_WITH(vectkit::ReadFeatureCollection(test_file), "Unknown CRS string: epsg:4326");

        std::filesystem::remove(test_file);
    }
}

TEST_CASE("Error Handling - Robust parsing") {
    const std::filesystem::path test_file = "/tmp/robust_test.geojson";

    SUBCASE("Features with null geometry are skipped") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": null,
                    "properties": {"name": "null_geom"}
                },
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1, 52.1, 0.0]
                    },
                    "properties": {"name": "valid_point"}
                }
            ]
        })";
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);

        // Should only have one feature (the null geometry one is skipped)
        CHECK(fc.features.size() == 1);
        CHECK(fc.features[0].properties.at("name") == "valid_point");

        std::filesystem::remove(test_file);
    }

    SUBCASE("Missing properties in feature defaults to empty") {
        std::ofstream ofs(test_file);
        ofs << R"({
            "type": "FeatureCollection",
            "properties": {
                "crs": "EPSG:4326",
                "datum": [5.0, 52.0, 0.0],
                "heading": 0.0
            },
            "features": [
                {
                    "type": "Feature",
                    "geometry": {
                        "type": "Point",
                        "coordinates": [5.1, 52.1, 0.0]
                    }
                }
            ]
        })";
        ofs.close();

        auto fc = vectkit::ReadFeatureCollection(test_file);

        CHECK(fc.features.size() == 1);
        CHECK(fc.features[0].properties.empty());

        std::filesystem::remove(test_file);
    }
}
