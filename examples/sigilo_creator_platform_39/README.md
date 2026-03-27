# Sigilo Creator Platform 39

Larger composed Sigilo example with a creator-platform shape: studio routes, auth, media, billing, moderation, notifications, analytics, admin, webhooks, and internal tasks.

Use this folder when you want to inspect:
- a denser system sigil with several route groups and modules
- a deeper operational trace crossing many modules
- the same trace rendered either on the composed system view or on pure routes
- the animated and step renderers over the real composed `system.svg`
- a draggable legend and a footer-expanded canvas so the timeline and scrubber stay on-screen

Core artifacts:
- `examples/sigilo_creator_platform_39/routes_view.system.svg`
- `examples/sigilo_creator_platform_39/routes_view.system.sigil`
- `examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace`
- `examples/sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg`
- `examples/sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg`
- `examples/sigilo_creator_platform_39/creator_release_pipeline_39_step.svg`

Generate the composed route/system sigil:

```bash
./cct --sigilo-only \
  --sigilo-style routes \
  --sigilo-out examples/sigilo_creator_platform_39/routes_view \
  examples/sigilo_creator_platform_39/main.cct
```

Render the animated publish pipeline:

```bash
./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace \
  --sigil examples/sigilo_creator_platform_39/routes_view.system.sigil \
  --out examples/sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg
```

Render the same trace on the pure routes view:

```bash
./cct sigilo trace render \
  --animated \
  --trace examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace \
  --sigil examples/sigilo_creator_platform_39/routes_view.system.sigil \
  --sigil-view routes \
  --out examples/sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg
```

Render the interactive step view:

```bash
./cct sigilo trace render \
  --step 6 \
  --trace examples/sigilo_creator_platform_39/creator_release_pipeline_39.ctrace \
  --sigil examples/sigilo_creator_platform_39/routes_view.system.sigil \
  --out examples/sigilo_creator_platform_39/creator_release_pipeline_39_step.svg
```

Open them:

```bash
open examples/sigilo_creator_platform_39/routes_view.system.svg
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_animated.svg
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_routes_animated.svg
open examples/sigilo_creator_platform_39/creator_release_pipeline_39_step.svg
```
