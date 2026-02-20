# Changelog

## [0.0.7] - 2026-02-20

### <!-- 2 -->🚜 Refactor

- Rename GeoJSON library to Vectkit

## [0.0.6] - 2026-01-18

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Migrate to CMake for build and dependency management

## [0.0.4] - 2026-01-16

### <!-- 0 -->⛰️  Features

- Update compile_commands.json for improved build

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Update external dependencies and build system

## [0.0.3] - 2026-01-03

### <!-- 3 -->📚 Documentation

- Documentations

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Update dependency versions
- Add visualizer for ENU GeoJSON data
- Add Rerun visualization for WGS conversion

## [0.0.2] - 2025-12-31

### <!-- 0 -->⛰️  Features

- Revamp build system and refactor Geo/Datum types
- Convert to header-only library and refactor GeoJSON parsing
- Replace Boost.JSON with sheredom/json.h

### <!-- 1 -->🐛 Bug Fixes

- Update project version and define PROJECT_DIR

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Update dependencies and improve SIMD configuration
- Refactor build system and remove unused dependencies

### Build

- Update dev environment for new test architecture

## [2.2.0] - 2025-12-15

### <!-- 0 -->⛰️  Features

- Refactor path representation to use `std::vector<Point>`

## [2.1.0] - 2025-11-02

### <!-- 0 -->⛰️  Features

- Feat: Migrate JSON handling to Boost.JSON and update dependencies

### Build

- Refactor: Remove unused compiler settings
- Add Makefile commands for configuration management

## [2.0.0] - 2025-11-01

### <!-- 2 -->🚜 Refactor

- Refactor: Improve modularity and update build system

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Bump project version to 1.4.7

## [2.0.0] - 2025-11-01

### <!-- 0 -->⛰️  Features

- Refactor polygon drawing to separate implementation from interface

### <!-- 2 -->🚜 Refactor

- Move nlohmann/json include to .cpp file
- Refactor `geoson` library to improve modularity

## [1.4.7] - 2025-10-31

### Build

- Check for pre-existing nlohmann_json package before fetching

## [1.4.6] - 2025-08-03

### <!-- 1 -->🐛 Bug Fixes

- Close polygons by repeating the first point

## [1.4.5] - 2025-08-03

### <!-- 4 -->⚡ Performance

- Round altitude to nearest integer for WGS conversion

## [1.4.4] - 2025-08-03

### <!-- 0 -->⛰️  Features

- Set default output CRS to WGS

## [1.4.2] - 2025-07-07

### <!-- 0 -->⛰️  Features

- Refactor point selection and polygon drawing

## [1.4.1] - 2025-07-07

### <!-- 0 -->⛰️  Features

- Refactor polygon collection around a datum point

## [1.4.0] - 2025-07-07

### <!-- 0 -->⛰️  Features

- Develop geographic data collection via an embedded web server

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Update concord and refactor constructors for readability

## [1.3.1] - 2025-06-29

### <!-- 0 -->⛰️  Features

- Implement global properties for vector data

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Bumps concord to version 2.0.4

## [1.2.0] - 2025-06-28

### <!-- 0 -->⛰️  Features

- Refactor GeoJSON processing and update test infrastructure
- Introduce core geographical vector data structures

## [1.1.0] - 2025-06-15

### <!-- 0 -->⛰️  Features

- Feat: Provide GeoJSON read/write aliases
- Add CRS conversion to GeoJSON writer

### <!-- 2 -->🚜 Refactor

- Refactor feature collection CRS handling

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Switch FeatureCollection default output to ENU

## [1.0.0] - 2025-06-15

### <!-- 2 -->🚜 Refactor

- Refactor CRS handling and update documentation

## [0.2.0] - 2025-06-11

### <!-- 3 -->📚 Documentation

- Elaborate README documentation content

### <!-- 7 -->⚙️ Miscellaneous Tasks

- Refactor build system and add changelog

