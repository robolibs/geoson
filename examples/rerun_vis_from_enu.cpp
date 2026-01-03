#include "datapod/datapod.hpp"
#include "geoson/geoson.hpp"
#include "rerun.hpp"
#include <iostream>

namespace gs = geoson;
namespace dp = datapod;
namespace cc = concord;

inline rerun::LatLon enu_to_latlon(const dp::Point &enu_pt, const dp::Geo &datum) {
    cc::frame::ENU enu{enu_pt.x, enu_pt.y, enu_pt.z, datum};
    auto wgs = cc::frame::to_wgs(enu);
    return rerun::LatLon{float(wgs.latitude), float(wgs.longitude)};
}

int main() {
    auto rec = std::make_shared<rerun::RecordingStream>("geoson", "examples");
    if (rec->connect_grpc("rerun+http://0.0.0.0:9876/proxy").is_err()) {
        std::cerr << "Failed to connect to rerun\n";
        return 1;
    }
    rec->log("", rerun::Clear::RECURSIVE);
    rec->log_with_static("", true, rerun::Clear::RECURSIVE);

    auto wgs_file = gs::read(PROJECT_DIR "/misc/field4_enu.geojson");

    dp::Vector<dp::Point> points;

    for (auto &feature : wgs_file.features) {
        if (auto *polygon = std::get_if<dp::Polygon>(&feature.geometry)) {
            std::cout << "Found polygon with " << polygon->vertices.size() << " vertices\n";
            for (const auto &vertex : polygon->vertices) {
                std::cout << "  Vertex: (" << vertex.x << ", " << vertex.y << ", " << vertex.z << ")\n";
                points.push_back(dp::Point(vertex.x, vertex.y, vertex.z));
            }
        }
    }

    std::vector<std::array<float, 3>> enu_pts;
    std::vector<rerun::LatLon> wgs_pts;

    for (const auto &p : points) {
        enu_pts.push_back({float(p.x), float(p.y), 0.0f});
        wgs_pts.push_back(enu_to_latlon(p, wgs_file.datum));
    }

    rec->log_static("/field", rerun::LineStrips3D(rerun::components::LineStrip3D(enu_pts))
                                  .with_colors({{rerun::Color(70, 120, 70)}})
                                  .with_radii({{0.2f}}));

    auto geo_headland = rerun::components::GeoLineString::from_lat_lon(wgs_pts);
    rec->log_static("/field", rerun::archetypes::GeoLineStrings(geo_headland)
                                  .with_colors({{rerun::Color(70, 120, 70)}})
                                  .with_radii({{0.5f}}));

    return 0;
}
