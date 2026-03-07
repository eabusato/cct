# FASE 16 Release Notes

## Summary

FASE 16 delivers the first executable freestanding bridge between CCT and LBOS.
The phase closes at **16D.4** with `--profile freestanding`, ASM emission, bridge packaging, and full host-regression preservation.

## Highlights

### 16A — Freestanding Profile Foundations

- Explicit profile selection (`host` / `freestanding`) in CLI flow.
- Freestanding semantic restrictions for unsupported host/runtime modules.
- Lowering matrix and diagnostic contracts consolidated for the bridge subset.

### 16B — Runtime and Kernel Bridge Surface

- Freestanding runtime shim (`src/runtime/cct_freestanding_rt.h/.c`) integrated.
- `cct/kernel` module delivered for bridge-safe services.
- Explicit freestanding entry contract (`--entry` -> `cct_fn_<mod>_<entry>`) stabilized.

### 16C — ASM Toolchain Path

- `--emit-asm` pipeline stabilized for Intel GAS syntax.
- Canonical ASM validation gate added (`tools/validate_freestanding_asm.sh`).
- End-to-end flow (`emit-asm -> as --32 -> nm`) closed with regression coverage.

### 16D — Bridge Packaging and Closure

- `make lbos-bridge` publishes canonical bridge artifact under `build/lbos-bridge/`.
- Linkability evidence and closure governance documents published.
- Phase closure completed with full regression green and handoff to FASE 17.

## Compatibility Notes

- Default host behavior remains unchanged.
- Freestanding profile intentionally blocks host-only modules and unsupported dynamic paths.
- `third_party/cct-boot` remained read-only during the phase.

## Operational References

- `./cct --profile freestanding --emit-asm --entry <rituale> <file.cct>`
- `make lbos-bridge`
- `tools/validate_freestanding_asm.sh`

## Verification Artifacts

- `docs/bootstrap/FASE_16_HANDOFF.md`
- `docs/bootstrap/CCT_ABI_V0_LBOS.md`
- `docs/bootstrap/CCT_SYMBOL_NAMING_V0.md`
- `docs/bootstrap/CCT_LOWERING_MATRIX_V0.md`
- `docs/bootstrap/CCT_ASM_SYNTAX_DECISION.md`
- `docs/bootstrap/EVIDENCIA_LINK_16D2.md`

## Known Limits Carried Forward

- No self-hosting in FASE 16.
- ORDO payload language support remains out of scope.
- Freestanding numeric/runtime surface remains intentionally conservative.
