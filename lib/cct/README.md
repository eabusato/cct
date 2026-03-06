# Bibliotheca Canonica (Foundation)

This directory is the canonical root for CCT standard-library modules (`cct/...`).

Current baseline:
- Project status is `FASE 15D.4 completed`.
- The phase-by-phase list below is preserved as historical rollout traceability for the stdlib surface.

Status in FASE 11A:
- namespace and resolver contract established
- physical distribution path established
- foundational test stub module present

Status in FASE 11B.1:
- `verbum.cct` canonical text core module implemented

Status in FASE 11B.2:
- `fmt.cct` canonical formatting/conversion module implemented

Status in FASE 11C:
- `series.cct` canonical static-collection helpers implemented
- `alg.cct` canonical baseline algorithms implemented

Status in FASE 11D.1:
- `mem.cct` canonical memory utility module implemented

Status in FASE 11D.2:
- runtime-backed FLUXUS storage core delivered (`cct_rt_fluxus_*`)

Status in FASE 11D.3:
- `fluxus.cct` canonical dynamic-vector API implemented

Status in FASE 11E.1:
- `io.cct` canonical standard IO module implemented
- `fs.cct` canonical filesystem module implemented

Status in FASE 11E.2:
- `path.cct` canonical path ergonomics module implemented

Status in FASE 11F.1:
- `math.cct` canonical numeric utility module implemented
- `random.cct` canonical random utility module implemented

Status in FASE 11F.2:
- `parse.cct` canonical strict textual conversion module implemented
- `cmp.cct` canonical comparator module implemented
- `alg.cct` moderately expanded with `alg_binary_search` and `alg_sort_insertion`

Status in FASE 12C:
- `option.cct` canonical Option baseline module implemented
- `result.cct` canonical Result baseline module implemented

Status in FASE 12D.1:
- `map.cct` canonical hash-map baseline module implemented
- `set.cct` canonical hash-set baseline module implemented

Status in FASE 12D.2:
- `collection_ops.cct` canonical collection combinators implemented (`map/filter/fold/find/any/all`)

The module families listed above are delivered and maintained in the stable baseline through FASE 15D.4.
