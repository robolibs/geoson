#pragma once

#include "parser.hpp"
#include "types.hpp"
#include "writter.hpp"

namespace geoson {

    FeatureCollection read(const std::filesystem::path &file);

    void write(const FeatureCollection &fc, const std::filesystem::path &outPath, CRS outputCrs);

    void write(const FeatureCollection &fc, const std::filesystem::path &outPath);

} // namespace geoson
