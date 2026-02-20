#pragma once

#include "parser.hpp"
#include "types.hpp"
#include "writter.hpp"

namespace vectkit {

    inline FeatureCollection read(const std::filesystem::path &file) { return ReadFeatureCollection(file); }

    inline void write(const FeatureCollection &fc, const std::filesystem::path &outPath, CRS outputCrs) {
        WriteFeatureCollection(fc, outPath, outputCrs);
    }

    inline void write(const FeatureCollection &fc, const std::filesystem::path &outPath) {
        WriteFeatureCollection(fc, outPath);
    }

} // namespace vectkit

namespace vk = vectkit;
