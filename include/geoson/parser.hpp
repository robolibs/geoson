#pragma once

#include "geoson/types.hpp"
#include <filesystem>
#include <iosfwd>

namespace geoson {

    FeatureCollection ReadFeatureCollection(const std::filesystem::path &file);

    std::ostream &operator<<(std::ostream &os, FeatureCollection const &fc);

} // namespace geoson
