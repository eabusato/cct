# FASE 11 Technical Release Notes

## Release Intent

FASE 11 closes the Bibliotheca Canonica baseline and prepares CCT for public technical usage with a coherent standard-library kit.

## Delivered Blocks

- 11A: stdlib platform contract and reserved namespace `cct/...`
- 11B: text + formatting core (`verbum`, `fmt`)
- 11C: collections baseline (`series`, `alg`)
- 11D: memory + dynamic vector (`mem`, `fluxus` + runtime storage core)
- 11E: io/files/path (`io`, `fs`, `path`)
- 11F: utility baseline (`math`, `random`, `parse`, `cmp`, moderate `alg` extras)
- 11G: canonical public showcases + sigilo stdlib metadata integration
- 11H: subset freeze, stability matrix, packaging/install consolidation

## Canonical Stable Highlights

- `cct/verbum`, `cct/fmt`, `cct/series`, `cct/fluxus`, `cct/mem`
- `cct/io`, `cct/fs`, `cct/path`
- `cct/math`, `cct/parse`, `cct/cmp`

## Canonical Experimental Highlights

- `cct/random` (PRNG baseline, non-crypto scope)
- `cct/alg` (moderate algorithm subset, controlled future expansion)

## Installation and Packaging

- Build: `make`
- Full regression: `make test`
- Relocatable bundle: `make dist`
- Optional install: `make install [PREFIX=...]`
- Uninstall: `make uninstall [PREFIX=...]`

See `docs/install.md` for full details.

## Showcase Entry Points

- `examples/showcase_stdlib_string_11g.cct`
- `examples/showcase_stdlib_collection_11g.cct`
- `examples/showcase_stdlib_io_fs_11g.cct`
- `examples/showcase_stdlib_parse_math_random_11g.cct`
- `examples/showcase_stdlib_modular_11g_main.cct`

## Known Boundaries (Deliberate)

- No package-manager/version-graph in this phase
- No heavy statistical/crypto stdlib
- No broad OS abstraction framework
- No redesign of sigilo visual core in this release cycle

## Forward Link

FASE 12 starts from a frozen, documented, and test-backed standard-library baseline.
