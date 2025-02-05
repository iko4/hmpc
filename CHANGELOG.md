# hmpc Changelog

The format is inspired by [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning is done following [Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## Unreleased

### Added

- `bool_cast` to explicitly convert values to booleans.

### Changed

- Message ID for non-extended operations changed: Use same communicator for receivers as for senders.
- Message ID counter per message kind/datatype.

### Fixed

- Missing session feature (cmake).


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
