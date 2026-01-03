#include "geoson/geoson.hpp"
#include <iostream>

int main() {
    try {
        // 1) Read a GeoJSON file (could be in WGS or ENU format)
        auto fc = geoson::read("misc/field4.geojson");

        std::cout << "Original file information:\n";
        std::cout << fc << "\n";

        // 2) Demonstrate that internal representation is always in Point coordinates
        std::cout << "\nInternal representation is always in Point (ENU/local) coordinates\n";
        std::cout << "regardless of input CRS format.\n\n";

        // 3) Save in WGS format (converts internal Point coordinates to WGS84)
        std::cout << "Saving to WGS84 format...\n";
        geoson::write(fc, "misc/output_wgs84.geojson", geoson::CRS::WGS);

        // 4) Save in ENU format (outputs Point coordinates directly)
        std::cout << "Saving to ENU format...\n";
        geoson::write(fc, "misc/output_enu.geojson", geoson::CRS::ENU);

        // 5) Save in original format (uses the CRS from the input file)
        std::cout << "Saving in original format (default behavior)...\n";
        geoson::write(fc, "misc/output_original.geojson");

        std::cout << "\nThree files created demonstrating CRS conversion:\n";
        std::cout << "- misc/output_wgs84.geojson (WGS84/EPSG:4326 format)\n";
        std::cout << "- misc/output_enu.geojson (ENU/local coordinate format)\n";
        std::cout << "- misc/output_original.geojson (same as input format)\n";

        std::cout << "\nNote: Internal representation always stores coordinates as Point (ENU/local),\n";
        std::cout << "but you can choose the output CRS when writing.\n";

    } catch (std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
