/*
 * CCT — Clavicula Turing
 * Sigilo Trace Overlay Implementation
 *
 * FASE 39B: Operational overlay taxonomy
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "trace_overlay.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const OverlayStyle TRACE_OVERLAY_STYLES[] = {
    { SPAN_CAT_HANDLER,    "cat-handler",    "#4a90d9", "#e9f3fd", "none", "Handler",    0 },
    { SPAN_CAT_MIDDLEWARE, "cat-middleware", "#7bb3e8", "#edf7fd", "4,2",  "Middleware", 0 },
    { SPAN_CAT_SQL,        "cat-sql",        "#2d9f5e", "#ebf8ef", "none", "SQL",        1 },
    { SPAN_CAT_CACHE,      "cat-cache",      "#e07b39", "#fdf1e9", "none", "Cache",      1 },
    { SPAN_CAT_STORAGE,    "cat-storage",    "#8b5cf6", "#f2ebfe", "none", "Storage",    0 },
    { SPAN_CAT_TRANSCODE,  "cat-transcode",  "#d946ef", "#fde9fe", "6,2",  "Transcode",  0 },
    { SPAN_CAT_EMAIL,      "cat-email",      "#ca8a04", "#fbf3db", "none", "Email",      0 },
    { SPAN_CAT_I18N,       "cat-i18n",       "#06b6d4", "#e4fbff", "2,2",  "I18N",       0 },
    { SPAN_CAT_TASK,       "cat-task",       "#6b7280", "#eef0f3", "none", "Task",       0 },
    { SPAN_CAT_ERROR,      "cat-error",      "#dc2626", "#feecec", "none", "Error",      1 },
    { SPAN_CAT_GENERIC,    "cat-generic",    "#94a3b8", "#eef2f6", "none", "Generic",    0 }
};

static const char *trace_overlay_lookup_attr(const TraceRenderSpan *span, const char *key) {
    return trace_render_attr_value(span, key);
}

static bool trace_overlay_truthy(const char *text) {
    if (!text) return false;
    return strcmp(text, "1") == 0 || strcmp(text, "true") == 0 || strcmp(text, "yes") == 0 ||
           strcmp(text, "on") == 0 || strcmp(text, "verum") == 0;
}

static bool trace_overlay_has_prefix(const char *text, const char *const *prefixes) {
    int i = 0;
    if (!text || !prefixes) return false;
    for (i = 0; prefixes[i]; i++) {
        size_t len = strlen(prefixes[i]);
        if (strncmp(text, prefixes[i], len) == 0) return true;
    }
    return false;
}

static SpanCategory trace_overlay_from_text(const char *text) {
    if (!text || text[0] == '\0') return SPAN_CAT_GENERIC;
    if (strcmp(text, "handler") == 0) return SPAN_CAT_HANDLER;
    if (strcmp(text, "middleware") == 0) return SPAN_CAT_MIDDLEWARE;
    if (strcmp(text, "sql") == 0 || strcmp(text, "db") == 0) return SPAN_CAT_SQL;
    if (strcmp(text, "cache") == 0 || strcmp(text, "redis") == 0) return SPAN_CAT_CACHE;
    if (strcmp(text, "storage") == 0) return SPAN_CAT_STORAGE;
    if (strcmp(text, "transcode") == 0 || strcmp(text, "media") == 0) return SPAN_CAT_TRANSCODE;
    if (strcmp(text, "email") == 0 || strcmp(text, "mail") == 0) return SPAN_CAT_EMAIL;
    if (strcmp(text, "i18n") == 0 || strcmp(text, "locale") == 0 || strcmp(text, "translation") == 0) return SPAN_CAT_I18N;
    if (strcmp(text, "task") == 0 || strcmp(text, "job") == 0 || strcmp(text, "worker") == 0) return SPAN_CAT_TASK;
    if (strcmp(text, "error") == 0) return SPAN_CAT_ERROR;
    return SPAN_CAT_GENERIC;
}

const char *trace_overlay_category_slug(SpanCategory cat) {
    switch (cat) {
        case SPAN_CAT_HANDLER: return "handler";
        case SPAN_CAT_MIDDLEWARE: return "middleware";
        case SPAN_CAT_SQL: return "sql";
        case SPAN_CAT_CACHE: return "cache";
        case SPAN_CAT_STORAGE: return "storage";
        case SPAN_CAT_TRANSCODE: return "transcode";
        case SPAN_CAT_EMAIL: return "email";
        case SPAN_CAT_I18N: return "i18n";
        case SPAN_CAT_TASK: return "task";
        case SPAN_CAT_ERROR: return "error";
        case SPAN_CAT_GENERIC:
        default:
            return "generic";
    }
}

const OverlayStyle *trace_overlay_style(SpanCategory cat) {
    size_t i = 0;
    for (i = 0; i < sizeof(TRACE_OVERLAY_STYLES) / sizeof(TRACE_OVERLAY_STYLES[0]); i++) {
        if (TRACE_OVERLAY_STYLES[i].category == cat) return &TRACE_OVERLAY_STYLES[i];
    }
    return &TRACE_OVERLAY_STYLES[sizeof(TRACE_OVERLAY_STYLES) / sizeof(TRACE_OVERLAY_STYLES[0]) - 1];
}

SpanCategory trace_overlay_resolve_category(const TraceRenderSpan *span) {
    static const char *const sql_prefixes[] = { "select:", "insert:", "update:", "delete:", "query:", NULL };
    static const char *const cache_prefixes[] = { "get:", "set:", "del:", "exists:", "cache:", NULL };
    static const char *const storage_prefixes[] = { "read:", "write:", "move:", "delete_file:", "storage:", NULL };
    static const char *const task_prefixes[] = { "task:", "job:", "worker:", "enqueue:", NULL };
    static const char *const email_prefixes[] = { "mail:", "smtp:", "spool:", "webhook:", NULL };
    const char *explicit_cat = NULL;
    const char *name = NULL;
    if (!span) return SPAN_CAT_GENERIC;
    explicit_cat = trace_overlay_lookup_attr(span, "category");
    if (explicit_cat && explicit_cat[0] != '\0') return trace_overlay_from_text(explicit_cat);
    if (span->category && span->category[0] != '\0') return trace_overlay_from_text(span->category);
    name = span->name ? span->name : "";
    if (strncmp(name, "middleware.", 11) == 0) return SPAN_CAT_MIDDLEWARE;
    if (strncmp(name, "handler.", 8) == 0 || strncmp(name, "request", 7) == 0) return SPAN_CAT_HANDLER;
    if (trace_overlay_has_prefix(name, sql_prefixes) || strncmp(name, "db.", 3) == 0 || strstr(name, "query")) return SPAN_CAT_SQL;
    if (trace_overlay_has_prefix(name, cache_prefixes) || strstr(name, "cache")) return SPAN_CAT_CACHE;
    if (trace_overlay_has_prefix(name, storage_prefixes)) return SPAN_CAT_STORAGE;
    if (trace_overlay_has_prefix(name, task_prefixes)) return SPAN_CAT_TASK;
    if (trace_overlay_has_prefix(name, email_prefixes)) return SPAN_CAT_EMAIL;
    if (strncmp(name, "translate:", 10) == 0 || strncmp(name, "i18n:", 5) == 0 || strncmp(name, "locale:", 7) == 0) return SPAN_CAT_I18N;
    return SPAN_CAT_GENERIC;
}

const char *trace_overlay_resolve_subcategory(const TraceRenderSpan *span, SpanCategory cat) {
    const char *explicit_sub = NULL;
    const char *name = NULL;
    const char *sep = NULL;
    const char *cache_result = NULL;
    if (!span) return "generic";
    explicit_sub = trace_overlay_lookup_attr(span, "subcategory");
    if (explicit_sub && explicit_sub[0] != '\0') return explicit_sub;
    name = span->name ? span->name : "";
    cache_result = trace_overlay_lookup_attr(span, "cache.result");
    switch (cat) {
        case SPAN_CAT_SQL:
        case SPAN_CAT_CACHE:
        case SPAN_CAT_STORAGE:
        case SPAN_CAT_TRANSCODE:
        case SPAN_CAT_EMAIL:
        case SPAN_CAT_I18N:
        case SPAN_CAT_TASK:
            if (cat == SPAN_CAT_CACHE && cache_result && cache_result[0] != '\0') return cache_result;
            sep = strchr(name, ':');
            if (sep && sep > name) {
                static char label[32];
                size_t len = (size_t)(sep - name);
                if (len >= sizeof(label)) len = sizeof(label) - 1u;
                memcpy(label, name, len);
                label[len] = '\0';
                return label;
            }
            if (cat == SPAN_CAT_SQL) return "query";
            if (cat == SPAN_CAT_CACHE) return "cache";
            if (cat == SPAN_CAT_STORAGE) return "storage";
            if (cat == SPAN_CAT_TRANSCODE) return "transcode";
            if (cat == SPAN_CAT_EMAIL) return "send";
            if (cat == SPAN_CAT_I18N) return "translate";
            if (cat == SPAN_CAT_TASK) return "task";
            break;
        case SPAN_CAT_HANDLER:
            return "handler";
        case SPAN_CAT_MIDDLEWARE:
            return "middleware";
        case SPAN_CAT_ERROR:
            return "error";
        case SPAN_CAT_GENERIC:
        default:
            return "generic";
    }
    return "generic";
}

void trace_overlay_compute_meta(const TraceRenderSpan *span, SpanOverlayMeta *meta) {
    long long threshold = 1000;
    if (!meta) return;
    memset(meta, 0, sizeof(*meta));
    if (!span) {
        meta->category = SPAN_CAT_GENERIC;
        meta->subcategory = "generic";
        return;
    }
    meta->category = trace_overlay_resolve_category(span);
    meta->subcategory = trace_overlay_resolve_subcategory(span, meta->category);
    meta->is_error = trace_overlay_truthy(trace_overlay_lookup_attr(span, "error"));
    switch (meta->category) {
        case SPAN_CAT_SQL:
            threshold = 200;
            break;
        case SPAN_CAT_CACHE:
            threshold = 50;
            break;
        case SPAN_CAT_STORAGE:
        case SPAN_CAT_TRANSCODE:
            threshold = 500;
            break;
        default:
            threshold = 1000;
            break;
    }
    meta->is_slow = span->duration_us > threshold ? 1 : 0;
}

void trace_overlay_emit_css(FILE *out) {
    size_t i = 0;
    if (!out) return;
    fprintf(out,
            ".legend-box{fill:#fffaf2;stroke:#d6cabc;stroke-width:.9}"
            ".legend-entry text{fill:#40352c;font:10px monospace}"
            ".legend-swatch{stroke-width:1}"
            ".slow{filter:drop-shadow(0 0 4px rgba(28,56,84,.18))}"
            ".error{stroke:#dc2626 !important;fill:#feecec !important}"
            ".pulse{filter:drop-shadow(0 0 3px rgba(28,56,84,.16))}"
            ".trace-packet{opacity:0;stroke:#fffaf2;stroke-width:1.1;filter:drop-shadow(0 0 3px rgba(28,56,84,.18))}"
            ".overlay-meta{font:10px monospace;fill:#5a5046}"
            ".cat-generic{stroke:#94a3b8}"
            ".timeline-bar.cat-generic{fill:#c4cfdb}"
            ".legend-swatch.cat-generic{fill:#eef2f6;stroke:#94a3b8}"
            ".trace-packet.cat-generic{fill:#94a3b8}");
    for (i = 0; i < sizeof(TRACE_OVERLAY_STYLES) / sizeof(TRACE_OVERLAY_STYLES[0]); i++) {
        const OverlayStyle *style = &TRACE_OVERLAY_STYLES[i];
        fprintf(out,
                ".%s{stroke:%s}%s"
                ".span-node.%s,.span-core.%s{stroke:%s;fill:%s}"
                ".span-link.%s,.route-flow.%s{stroke:%s}%s"
                ".trace-packet.%s{fill:%s}"
                ".timeline-bar.%s,.legend-swatch.%s{fill:%s;stroke:%s}",
                style->css_class, style->stroke_color,
                strcmp(style->dash_pattern, "none") == 0 ? "" : "",
                style->css_class, style->css_class, style->stroke_color, style->fill_color,
                style->css_class, style->css_class, style->stroke_color,
                strcmp(style->dash_pattern, "none") == 0 ? "" : "",
                style->css_class, style->stroke_color,
                style->css_class, style->css_class, style->stroke_color, style->stroke_color);
        if (strcmp(style->dash_pattern, "none") != 0) {
            fprintf(out,
                    ".span-link.%s,.route-flow.%s,.span-node.%s,.legend-swatch.%s{stroke-dasharray:%s}",
                    style->css_class, style->css_class, style->css_class, style->css_class, style->dash_pattern);
        }
        if (style->pulse) fprintf(out, ".%s{vector-effect:non-scaling-stroke}", style->css_class);
    }
}

void trace_overlay_emit_legend(const SpanCategory *active_cats, int cat_count, FILE *out) {
    int i = 0;
    double height = 16.0 + (double)cat_count * 16.0;
    if (!out || !active_cats || cat_count <= 0) return;
    fprintf(out, "<g id=\"legend\"><rect class=\"legend-box\" x=\"0\" y=\"0\" width=\"126\" height=\"%.2f\" rx=\"10\"/>", height);
    fprintf(out, "<text class=\"legend\" x=\"10\" y=\"13\">legend</text>");
    for (i = 0; i < cat_count; i++) {
        const OverlayStyle *style = trace_overlay_style(active_cats[i]);
        double y = 24.0 + (double)i * 15.0;
        fprintf(out, "<g class=\"legend-entry\" data-category=\"%s\">", trace_overlay_category_slug(active_cats[i]));
        fprintf(out, "<rect class=\"legend-swatch %s\" x=\"9\" y=\"%.2f\" width=\"12\" height=\"8\" rx=\"2\"/>", style->css_class, y - 7.0);
        fprintf(out, "<text x=\"28\" y=\"%.2f\">%s</text></g>", y, style->label);
    }
    fprintf(out, "</g>");
}

void trace_overlay_emit_attrs(const SpanOverlayMeta *meta, FILE *out) {
    if (!meta || !out) return;
    fprintf(out, " data-category=\"%s\"", trace_overlay_category_slug(meta->category));
    fprintf(out, " data-subcategory=\"%s\"", meta->subcategory ? meta->subcategory : "generic");
    if (meta->is_slow) fprintf(out, " data-slow=\"1\"");
    if (meta->is_error) fprintf(out, " data-error=\"1\"");
}
