/*
 * CCT — Clavicula Turing
 * Sigilo Trace Render Definitions
 *
 * FASE 39A: Trace visual animated renderer
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_TRACE_RENDER_H
#define CCT_TRACE_RENDER_H

#include "../common/types.h"
#include <stdio.h>
#include <stddef.h>

typedef enum {
    TRACE_RENDER_SINGLE = 0,
    TRACE_RENDER_COMPARE = 1,
    TRACE_RENDER_STEP = 2,
    TRACE_RENDER_STATIC = 3
} TraceRenderMode;

typedef struct {
    char *key;
    char *value;
} TraceAttr;

typedef struct {
    char *trace_id;
    char *span_id;
    char *parent_id;
    char *name;
    char *category;
    char *attributes_json;
    char *route_id;
    char *handler;
    char *module_path;
    char *http_method;
    char *http_path;
    long long start_us;
    long long end_us;
    long long duration_us;
    int depth;
    int parent_index;
    TraceAttr *attrs;
    int attr_count;
} TraceRenderSpan;

typedef struct {
    char *input_path;
    char *trace_id;
    TraceRenderSpan *spans;
    int span_count;
    size_t span_capacity;
    long long total_us;
    size_t warnings;
    size_t errors;
} TraceRenderTrace;

typedef struct {
    TraceRenderMode mode;
    TraceRenderTrace trace_a;
    TraceRenderTrace trace_b;
    const char *sigil_path;
    int step_index;
    int animated;
    int animation_loop;
    int hide_timeline;
    const char *filter_kind;
    const char *focus_route;
} TraceRenderInput;

int trace_render_load_ctrace(const char *path, TraceRenderTrace *out);
void trace_render_free(TraceRenderTrace *trace);
void trace_render_compute_depths(TraceRenderTrace *trace);
void trace_render_emit_timeline(TraceRenderTrace *trace, FILE *out);
int trace_render_single(TraceRenderInput *input, FILE *out);
int trace_render_compare(TraceRenderInput *input, FILE *out);
int trace_render_step(TraceRenderInput *input, FILE *out);

const char *trace_render_attr_value(const TraceRenderSpan *span, const char *key);
void trace_render_svg_escape(FILE *out, const char *text);

#endif /* CCT_TRACE_RENDER_H */
