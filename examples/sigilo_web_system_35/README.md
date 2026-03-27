# Sigilo Web System 35

Stable modular Sigilo example with composed system view and realistic trace animation.

Use this folder when you want to inspect:
- a composed route/system sigil across multiple modules
- an operational trace rendered on top of the composed `.system.sigil`
- the interactive `--step` scrubber inside the SVG

Core artifacts already present:
- `examples/sigilo_web_system_35/routes_view.system.svg`
- `examples/sigilo_web_system_35/routes_view.system.sigil`
- `examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace`
- `examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg`
- `examples/sigilo_web_system_35/media_upload_pipeline_39_step.svg`

Regenerate the composed route/system sigil:

```bash
./cct --sigilo-only \
  --sigilo-style routes \
  --sigilo-out examples/sigilo_web_system_35/routes_view \
  examples/sigilo_web_system_35/main.cct
```

Render the animated upload pipeline:

```bash
./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --out examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg
```

Render the interactive step view:

```bash
./cct sigilo trace render \
  --step 4 \
  --trace examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --out examples/sigilo_web_system_35/media_upload_pipeline_39_step.svg
```

Open them in the browser:

```bash
open examples/sigilo_web_system_35/routes_view.system.svg
open examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg
open examples/sigilo_web_system_35/media_upload_pipeline_39_step.svg
```
