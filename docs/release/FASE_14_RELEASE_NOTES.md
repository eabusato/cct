# FASE 14 Release Notes

## Highlights

- Canonical diagnostic taxonomy reinforced (`error|warning|note|hint`).
- Canonical exit-code policy consolidated across strict gates and CLI paths.
- `sigilo ... --explain` support extended to operational sigilo commands.
- Deterministic sigilo diagnostic ordering formalized.
- Documentation hardening block completed (`14B1..14B4`), including:
  - public/private publication policy and manifests;
  - operational guide;
  - release template pack.
- Technical-audit/release-validation block completed (`14C*`, `14D*`) with:
  - expanded regression matrix;
  - stress/soak and performance budget checks;
  - packaging reproducibility and RC validation matrices.

## Compatibility Notes

- No intentional breaking changes to stable command surface.
- Explain-mode output remains opt-in to avoid script noise.

## References

- `docs/release/FASE_14_RELEASE_NOTES.md`
- `docs/release/FASE_15_RELEASE_NOTES.md`
- Internal governance artifacts are tracked in private release documentation.
