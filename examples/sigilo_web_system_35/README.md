# Sigilo Web System 35

Stable modular Sigilo example with composed system view and realistic trace animation.

Use this folder when you want to inspect:
- a composed route/system sigil across multiple modules
- an operational trace rendered on top of the composed `.system.sigil`
- the same trace rendered on the plain route sigil with no system composition
- the interactive `--step` scrubber inside the SVG
- a draggable legend and an auto-expanded footer so the timeline does not get cut off in large system views

Core artifacts already present:
- `examples/sigilo_web_system_35/routes_view.system.svg`
- `examples/sigilo_web_system_35/routes_view.system.sigil`
- `examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace`
- `examples/sigilo_web_system_35/media_upload_pipeline_39_animated.svg`
- `examples/sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg`
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

Render the same pipeline on the plain routes view:

```bash
./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_web_system_35/media_upload_pipeline_39.ctrace \
  --sigil examples/sigilo_web_system_35/routes_view.system.sigil \
  --sigil-view routes \
  --out examples/sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg
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
open examples/sigilo_web_system_35/media_upload_pipeline_39_routes_animated.svg
open examples/sigilo_web_system_35/media_upload_pipeline_39_step.svg
```

Live capture workflow:

- keep your CCT web app running with `cct/instrument` enabled
- open a capture window with `cct/trace_capture`
- use the app manually through real endpoints
- stop or snapshot the window
- render one exported `.ctrace` back on top of `routes_view.system.sigil`

Reference:

- `docs/trace_capture.md`
