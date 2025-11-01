#include "geoson/geoson.hpp"

namespace geoson {

    FeatureCollection read(const std::filesystem::path &file) { return ReadFeatureCollection(file); }

    void write(const FeatureCollection &fc, const std::filesystem::path &outPath, CRS outputCrs) {
        WriteFeatureCollection(fc, outPath, outputCrs);
    }

    void write(const FeatureCollection &fc, const std::filesystem::path &outPath) {
        WriteFeatureCollection(fc, outPath);
    }

} // namespace geoson
