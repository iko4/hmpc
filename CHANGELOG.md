# hmpc Changelog

The format is inspired by [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning is done following [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## Unreleased

### Changed

- Update container image to Ubuntu 24.04.
- Format via standard library instead of fmt library.


## Version 0.5.2 - 2025-03-04

### Added

- Make library (and binary tools) installable and support finding it (cmake).
- "default" workflow (cmake).
- Separate "development" and "library-development" container image targets.
- MPC API for simple secret-sharing.

## Changed

- Require at least CMake version 3.25.0.
- Container user to "hmpc-dev".
- Attach C++ version to library (cmake).


## Version 0.5.1 - 2025-02-10

### Added

- `bool_cast` to explicitly convert values to booleans.
- Feature: Check consistency of collective communication operations (networking).

### Changed

- Message ID for non-extended operations changed: Use same communicator for receivers as for senders.
- Message ID counter per message kind/datatype.
- Data is hashed independently of all other metadata for signatures.

### Fixed

- Missing session feature (cmake).

### Security

- Collective communication operations can be checked for consistency.


## Version 0.5.0 - 2025-01-30

### Added

- Changelog.
- Begin working on documentation of concepts and library design.
- Feature: Session IDs (networking).
- Test: Tag of underlying datatype (networking).

### Changed

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
