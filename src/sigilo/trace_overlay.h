/*
 * CCT — Clavicula Turing
 * Sigilo Trace Overlay Definitions
 *
 * FASE 39B: Operational overlay taxonomy
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_TRACE_OVERLAY_H
#define CCT_TRACE_OVERLAY_H

#include "trace_render.h"

#include <stdio.h>

typedef enum {
    SPAN_CAT_HANDLER = 0,
    SPAN_CAT_MIDDLEWARE = 1,
    SPAN_CAT_SQL = 2,
    SPAN_CAT_CACHE = 3,
    SPAN_CAT_STORAGE = 4,
    SPAN_CAT_TRANSCODE = 5,
    SPAN_CAT_EMAIL = 6,
    SPAN_CAT_I18N = 7,
    SPAN_CAT_TASK = 8,
    SPAN_CAT_ERROR = 9,
    SPAN_CAT_GENERIC = 10
} SpanCategory;

typedef struct {
    SpanCategory category;
    const char *css_class;
    const char *stroke_color;
    const char *fill_color;
    const char *dash_pattern;
    const char *label;
    int pulse;
} OverlayStyle;

typedef struct {
    SpanCategory category;
    const char *subcategory;
    int is_slow;
    int is_error;
} SpanOverlayMeta;

SpanCategory trace_overlay_resolve_category(const TraceRenderSpan *span);
const char *trace_overlay_resolve_subcategory(const TraceRenderSpan *span, SpanCategory cat);
const OverlayStyle *trace_overlay_style(SpanCategory cat);
void trace_overlay_compute_meta(const TraceRenderSpan *span, SpanOverlayMeta *meta);
void trace_overlay_emit_css(FILE *out);
void trace_overlay_emit_legend(const SpanCategory *active_cats, int cat_count, FILE *out);
void trace_overlay_emit_attrs(const SpanOverlayMeta *meta, FILE *out);
const char *trace_overlay_category_slug(SpanCategory cat);

#endif /* CCT_TRACE_OVERLAY_H */
