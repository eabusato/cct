# FASE 13 + 13M Omniversal Showcase

Arquivos:
- `examples/fase13_omniversal_13m.cct` (base)
- `examples/fase13_omniversal_13m_variant.cct` (variant para diff/check)

## 1) Build + run

```bash
./cct examples/fase13_omniversal_13m.cct
./examples/fase13_omniversal_13m
```

```bash
./cct examples/fase13_omniversal_13m_variant.cct
./examples/fase13_omniversal_13m_variant
```

## 2) Sigilo-only artifacts

```bash
./cct --sigilo-only examples/fase13_omniversal_13m.cct
./cct --sigilo-only examples/fase13_omniversal_13m_variant.cct
```

## 3) FASE 13 CLI workflows

Inspect:

```bash
./cct sigilo inspect examples/fase13_omniversal_13m.sigil --summary
```

Validate:

```bash
./cct sigilo validate examples/fase13_omniversal_13m.sigil --consumer-profile current-default --summary
```

Diff:

```bash
./cct sigilo diff examples/fase13_omniversal_13m.sigil examples/fase13_omniversal_13m_variant.sigil --summary
```

Check (strict):

```bash
./cct sigilo check examples/fase13_omniversal_13m.sigil examples/fase13_omniversal_13m_variant.sigil --strict --summary
```

Baseline update/check:

```bash
./cct sigilo baseline update examples/fase13_omniversal_13m.sigil --baseline /tmp/fase13_omniversal_13m.baseline
./cct sigilo baseline check examples/fase13_omniversal_13m.sigil --baseline /tmp/fase13_omniversal_13m.baseline --summary
```

## 4) 13M operators highlighted in code

- `**` em potência (`(base + passo) ** 2`, `pulso ** 2`)
- `//` em divisão inteira floor
- `%%` em módulo euclidiano
- mistura em expressões compostas com precedência explícita
