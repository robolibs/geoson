#include "geoson/geoson.hpp"
#include <iostream>

int main() {
    try {
        // 1) Read your GeoJSON
        auto fc = geoson::read("misc/field4.geojson");

        // 2) Print what we got
        std::cout << fc << "\n";

        // 3) Tweak the datum (for example bump the latitude by +0.1°)
        fc.datum.latitude += 5.1;
        std::cout << "After tweak, new datum is: " << fc.datum.latitude << ", " << fc.datum.longitude << ", "
                  << fc.datum.altitude << "\n";

        // 4) Save back out
        geoson::write(fc, "misc/field4.geojson");
        std::cout << "Saved modified GeoJSON to misc/field4_modified.geojson\n";
    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
