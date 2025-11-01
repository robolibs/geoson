# Changelog

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

