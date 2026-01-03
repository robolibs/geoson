#include <doctest/doctest.h>

#include "geoson/vector.hpp"
#include <filesystem>
#include <fstream>

namespace dp = ::datapod;

TEST_CASE("Vector - Basic Construction") {
    dp::Geo datum{52.0, 5.0, 0.0};
    dp::Euler heading{0, 0, 0.5};

    // Create field boundary polygon
    dp::Polygon fieldBoundary{dp::Vector<dp::Point>{{dp::Point{0.0, 0.0, 0.0}, dp::Point{100.0, 0.0, 0.0},
                                                     dp::Point{100.0, 100.0, 0.0}, dp::Point{0.0, 100.0, 0.0}}}};

    SUBCASE("Constructor with default parameters") {
        geoson::Vector vector(fieldBoundary);

        CHECK(vector.elementCount() == 0);
        CHECK_FALSE(vector.hasElements());
        CHECK(vector.getCRS() == geoson::CRS::ENU);
    }

    SUBCASE("Constructor with all parameters") {
        geoson::Vector vector(fieldBoundary, datum, heading, geoson::CRS::WGS);

        CHECK(vector.getDatum().latitude == doctest::Approx(52.0));
        CHECK(vector.getDatum().longitude == doctest::Approx(5.0));
        CHECK(vector.getHeading().yaw == doctest::Approx(0.5));
        CHECK(vector.getCRS() == geoson::CRS::WGS);
    }
}

TEST_CASE("Vector - Element Management") {
    dp::Polygon fieldBoundary{dp::Vector<dp::Point>{{dp::Point{0.0, 0.0, 0.0}, dp::Point{100.0, 0.0, 0.0},
                                                     dp::Point{100.0, 100.0, 0.0}, dp::Point{0.0, 100.0, 0.0}}}};
    geoson::Vector vector(fieldBoundary);

    SUBCASE("Add and retrieve elements") {
        dp::Point point{50.0, 50.0, 0.0};
        vector.addPoint(point, "waypoint", {{"id", "wp1"}});

        CHECK(vector.elementCount() == 1);
        CHECK(vector.hasElements());

        const auto &element = vector.getElement(0);
        CHECK(element.type == "waypoint");
        CHECK(element.properties.at("id") == "wp1");
        CHECK(std::holds_alternative<dp::Point>(element.geometry));
    }

    SUBCASE("Add different geometry types") {
        dp::Point point{25.0, 25.0, 0.0};
        dp::Segment line{{10.0, 10.0, 0.0}, {90.0, 90.0, 0.0}};
        std::vector<dp::Point> pathPoints = {{20.0, 20.0, 0.0}, {40.0, 40.0, 0.0}, {60.0, 60.0, 0.0}};

        vector.addPoint(point, "marker");
        vector.addLine(line, "boundary");
        vector.addPath(pathPoints, "route");

        CHECK(vector.elementCount() == 3);
        CHECK(vector.getPoints().size() == 1);
        CHECK(vector.getLines().size() == 1);
        CHECK(vector.getPaths().size() == 1);
    }

    SUBCASE("Filter by type and properties") {
        vector.addPoint({10.0, 10.0, 0.0}, "marker", {{"color", "red"}});
        vector.addPoint({20.0, 20.0, 0.0}, "marker", {{"color", "blue"}});
        vector.addPoint({30.0, 30.0, 0.0}, "waypoint", {{"color", "red"}});

        auto markers = vector.getElementsByType("marker");
        CHECK(markers.size() == 2);

        auto redItems = vector.filterByProperty("color", "red");
        CHECK(redItems.size() == 2);

        auto blueItems = vector.filterByProperty("color", "blue");
        CHECK(blueItems.size() == 1);
    }

    SUBCASE("Remove elements") {
        vector.addPoint({10.0, 10.0, 0.0}, "marker");
        vector.addPoint({20.0, 20.0, 0.0}, "waypoint");

        CHECK(vector.elementCount() == 2);

        vector.removeElement(0);
        CHECK(vector.elementCount() == 1);
        CHECK(vector.getElement(0).type == "waypoint");

        vector.clearElements();
        CHECK(vector.elementCount() == 0);
        CHECK_FALSE(vector.hasElements());
    }
}

TEST_CASE("Vector - Field Management") {
    dp::Polygon fieldBoundary{dp::Vector<dp::Point>{
        {dp::Point{0.0, 0.0, 0.0}, dp::Point{50.0, 0.0, 0.0}, dp::Point{50.0, 50.0, 0.0}, dp::Point{0.0, 50.0, 0.0}}}};
    geoson::Vector vector(fieldBoundary);

    SUBCASE("Field properties") {
        vector.setFieldProperty("name", "Test Field");
        vector.setFieldProperty("crop", "corn");

        const auto &props = vector.getFieldProperties();
        CHECK(props.at("name") == "Test Field");
        CHECK(props.at("crop") == "corn");

        vector.removeFieldProperty("crop");
        CHECK(props.find("crop") == props.end());
    }

    SUBCASE("Field boundary") {
        const auto &boundary = vector.getFieldBoundary();
        CHECK(boundary.vertices.size() == 4);

        dp::Polygon newBoundary{dp::Vector<dp::Point>{{dp::Point{0.0, 0.0, 0.0}, dp::Point{100.0, 0.0, 0.0},
                                                       dp::Point{100.0, 100.0, 0.0}, dp::Point{0.0, 100.0, 0.0}}}};
        vector.setFieldBoundary(newBoundary);

        CHECK(vector.getFieldBoundary().vertices.size() == 4);
    }
}

TEST_CASE("Vector - File I/O") {
    dp::Polygon fieldBoundary{dp::Vector<dp::Point>{{dp::Point{0.0, 0.0, 0.0}, dp::Point{100.0, 0.0, 0.0},
                                                     dp::Point{100.0, 100.0, 0.0}, dp::Point{0.0, 100.0, 0.0}}}};
    dp::Geo datum{52.0, 5.0, 0.0};

    geoson::Vector originalVector(fieldBoundary, datum);
    originalVector.setFieldProperty("name", "Test Field");
    originalVector.addPoint({50.0, 50.0, 0.0}, "center", {{"important", "true"}});
    originalVector.addLine({{10.0, 10.0, 0.0}, {90.0, 90.0, 0.0}}, "diagonal");

    std::filesystem::path testFile = std::filesystem::temp_directory_path() / "test_vector.geojson";

    SUBCASE("Save and load vector") {
        // Save to file
        originalVector.toFile(testFile);
        CHECK(std::filesystem::exists(testFile));

        // Load from file
        auto loadedVector = geoson::Vector::fromFile(testFile);

        CHECK(loadedVector.elementCount() == 2);
        CHECK(loadedVector.getDatum().latitude == doctest::Approx(52.0));
        CHECK(loadedVector.getFieldProperties().at("name") == "Test Field");

        auto points = loadedVector.getPoints();
        CHECK(points.size() == 1);
        CHECK(points[0].type == "center");
        CHECK(points[0].properties.at("important") == "true");

        auto lines = loadedVector.getLines();
        CHECK(lines.size() == 1);
        CHECK(lines[0].type == "diagonal");

        // Cleanup
        std::filesystem::remove(testFile);
    }

    SUBCASE("Save with different CRS") {
        originalVector.toFile(testFile, geoson::CRS::WGS);
        CHECK(std::filesystem::exists(testFile));

        auto loadedVector = geoson::Vector::fromFile(testFile);
        CHECK(loadedVector.elementCount() == 2);

        std::filesystem::remove(testFile);
    }
}

TEST_CASE("Vector - Error Handling") {
    SUBCASE("Out of range access") {
        dp::Polygon fieldBoundary{dp::Vector<dp::Point>{{dp::Point{0.0, 0.0, 0.0}, dp::Point{10.0, 0.0, 0.0},
                                                         dp::Point{10.0, 10.0, 0.0}, dp::Point{0.0, 10.0, 0.0}}}};
        geoson::Vector vector(fieldBoundary);

        CHECK_THROWS_AS(vector.getElement(0), std::out_of_range);
        CHECK_THROWS_AS(vector.getElement(10), std::out_of_range);
    }

    SUBCASE("File not found") {
        std::filesystem::path nonExistentFile = "/tmp/does_not_exist.geojson";
        CHECK_THROWS(geoson::Vector::fromFile(nonExistentFile));
    }
}

TEST_CASE("Vector - Iterators") {
    dp::Polygon fieldBoundary{dp::Vector<dp::Point>{
        {dp::Point{0.0, 0.0, 0.0}, dp::Point{10.0, 0.0, 0.0}, dp::Point{10.0, 10.0, 0.0}, dp::Point{0.0, 10.0, 0.0}}}};
    geoson::Vector vector(fieldBoundary);

    vector.addPoint({1.0, 1.0, 0.0}, "p1");
    vector.addPoint({2.0, 2.0, 0.0}, "p2");
    vector.addPoint({3.0, 3.0, 0.0}, "p3");

    SUBCASE("Range-based for loop") {
        int count = 0;
        for (const auto &element : vector) {
            CHECK(element.type.starts_with("p"));
            count++;
        }
        CHECK(count == 3);
    }

    SUBCASE("Iterator access") {
        auto it = vector.begin();
        CHECK(it->type == "p1");
        ++it;
        CHECK(it->type == "p2");
        ++it;
        CHECK(it->type == "p3");
        ++it;
        CHECK(it == vector.end());
    }
}
