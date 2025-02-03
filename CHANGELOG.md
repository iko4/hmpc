# hmpc Changelog

Versioning is done following [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Unreleased

### Added

- `bool_cast` to explicitly convert values to booleans

### Fixed

- Missing session feature (cmake)


## Version 0.5.0 - 2025-01-30

### Added

- Changelog.
- Begin working on documentation of concepts and library design.
- Feature: Session IDs (networking).
- Test: Tag of underlying datatype (networking).

### Changes

- Upgrade dependencies and build environment.
- Change file extension for keys and certificates.
- Message format is changed (see documentation for details).
    - Bit width of metadata members changed.
    - Signature comes last (if enabled).
    - Session ID added.

### Fixed

- Missing path to OpenMP libraries in `LD_LIBRARY_PATH` for container build.

### Security

- Generate distinct keys for certificates and signing.
- Session identifiers can be enabled.


## Version 0.4.0 - 2024-12-29

Initial public release.
