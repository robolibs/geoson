#pragma once

#include "geoson/types.hpp"
#include <filesystem>

namespace geoson {

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath,
                                geoson::CRS outputCrs);

    void WriteFeatureCollection(FeatureCollection const &fc, std::filesystem::path const &outPath);

} // namespace geoson
