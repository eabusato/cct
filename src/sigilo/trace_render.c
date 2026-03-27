/*
 * CCT — Clavicula Turing
 * Sigilo Trace Render Implementation
 *
 * FASE 39A: Trace visual animated renderer
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "trace_render.h"

#include "sigil_parse.h"
#include "trace_overlay.h"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_RENDER_PI 3.14159265358979323846

typedef struct {
    double panel_x;
    double panel_y;
    double panel_w;
    double panel_h;
    double cx;
    double cy;
    double core_r;
    double timeline_x;
    double timeline_y;
    double timeline_w;
    double timeline_h;
} trace_render_panel_t;

typedef struct {
    double x;
    double y;
    double r;
    int active;
    int mapped_route;
    int visible;
} trace_render_node_t;

typedef struct {
    const cct_sigil_web_route_t *route;
    double x;
    double y;
} trace_render_route_hit_t;

typedef struct {
    long long min_start;
    long long max_end;
    int max_depth;
} trace_render_bounds_t;

typedef struct {
    char *route_id;
    char *group;
    char *module_path;
    char *handler;
    double x;
    double y;
} trace_render_svg_route_anchor_t;

typedef struct {
    char *module_path;
    double cx;
    double cy;
    double r;
    double scale;
} trace_render_svg_module_anchor_t;

typedef struct {
    char *group;
    double x;
    double y;
    int count;
} trace_render_svg_group_anchor_t;

typedef struct {
    char *module_path;
    char *ritual;
    double x;
    double y;
} trace_render_svg_ritual_anchor_t;

typedef struct {
    char *svg_prefix;
    double min_x;
    double min_y;
    double width;
    double height;
    int ready;
    trace_render_svg_route_anchor_t *routes;
    int route_count;
    int route_capacity;
    trace_render_svg_module_anchor_t *modules;
    int module_count;
    int module_capacity;
    trace_render_svg_group_anchor_t *groups;
    int group_count;
    int group_capacity;
    trace_render_svg_ritual_anchor_t *rituals;
    int ritual_count;
    int ritual_capacity;
} trace_render_svg_backdrop_t;

static bool trace_render_matches_filter(const TraceRenderInput *input, const TraceRenderSpan *span);
static const cct_sigil_web_route_t *trace_render_find_route_by_id(const cct_sigil_document_t *doc, const char *route_id);

static char *trace_render_read_file(const char *path) {
    FILE *file = NULL;
    long size = 0;
    size_t bytes_read = 0;
    char *buffer = NULL;

    if (!path) return NULL;
    file = fopen(path, "rb");
    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    rewind(file);
    buffer = (char*)malloc((size_t)size + 1u);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    bytes_read = fread(buffer, 1u, (size_t)size, file);
    fclose(file);
    if (bytes_read != (size_t)size) {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    return buffer;
}

static void trace_render_svg_backdrop_dispose(trace_render_svg_backdrop_t *backdrop) {
    int i = 0;
    if (!backdrop) return;
    free(backdrop->svg_prefix);
    for (i = 0; i < backdrop->route_count; i++) {
        free(backdrop->routes[i].route_id);
        free(backdrop->routes[i].group);
        free(backdrop->routes[i].module_path);
        free(backdrop->routes[i].handler);
    }
    free(backdrop->routes);
    for (i = 0; i < backdrop->module_count; i++) free(backdrop->modules[i].module_path);
    free(backdrop->modules);
    for (i = 0; i < backdrop->group_count; i++) free(backdrop->groups[i].group);
    free(backdrop->groups);
    for (i = 0; i < backdrop->ritual_count; i++) {
        free(backdrop->rituals[i].module_path);
        free(backdrop->rituals[i].ritual);
    }
    free(backdrop->rituals);
    memset(backdrop, 0, sizeof(*backdrop));
}

static char *trace_render_strdup_range(const char *start, const char *end) {
    size_t len = 0;
    char *out = NULL;
    if (!start || !end || end < start) return NULL;
    len = (size_t)(end - start);
    out = (char*)malloc(len + 1u);
    if (!out) return NULL;
    memcpy(out, start, len);
    out[len] = '\0';
    return out;
}

static char *trace_render_strdup_trimmed(const char *start, const char *end) {
    while (start && end && start < end && isspace((unsigned char)*start)) start++;
    while (start && end && end > start && isspace((unsigned char)end[-1])) end--;
    return trace_render_strdup_range(start, end);
}

static char *trace_render_replace_suffix(const char *path, const char *old_suffix, const char *new_suffix) {
    size_t path_len = 0;
    size_t old_len = 0;
    size_t new_len = 0;
    char *out = NULL;
    if (!path || !old_suffix || !new_suffix) return NULL;
    path_len = strlen(path);
    old_len = strlen(old_suffix);
    new_len = strlen(new_suffix);
    if (path_len < old_len || strcmp(path + path_len - old_len, old_suffix) != 0) return NULL;
    out = (char*)malloc(path_len - old_len + new_len + 1u);
    if (!out) return NULL;
    memcpy(out, path, path_len - old_len);
    memcpy(out + path_len - old_len, new_suffix, new_len);
    out[path_len - old_len + new_len] = '\0';
    return out;
}

static char *trace_render_extract_attr_range(const char *start, const char *end, const char *attr) {
    char pattern[96];
    const char *p = NULL;
    size_t n = 0;
    if (!start || !end || !attr || end <= start) return NULL;
    snprintf(pattern, sizeof(pattern), "%s=\"", attr);
    p = start;
    n = strlen(pattern);
    while (p && p < end) {
        p = strstr(p, pattern);
        if (!p || p >= end) return NULL;
        if (p + (ptrdiff_t)n >= end) return NULL;
        p += n;
        {
            const char *q = strchr(p, '"');
            if (!q || q > end) return NULL;
            return trace_render_strdup_range(p, q);
        }
    }
    return NULL;
}

static bool trace_render_push_group_anchor(trace_render_svg_backdrop_t *backdrop,
                                           const char *group,
                                           double x,
                                           double y) {
    trace_render_svg_group_anchor_t *grown = NULL;
    int next = 0;
    int i = 0;
    if (!backdrop || !group || group[0] == '\0') return false;
    for (i = 0; i < backdrop->group_count; i++) {
        if (strcmp(backdrop->groups[i].group, group) == 0) {
            double count = (double)backdrop->groups[i].count;
            backdrop->groups[i].x = ((backdrop->groups[i].x * count) + x) / (count + 1.0);
            backdrop->groups[i].y = ((backdrop->groups[i].y * count) + y) / (count + 1.0);
            backdrop->groups[i].count += 1;
            return true;
        }
    }
    if (backdrop->group_count >= backdrop->group_capacity) {
        next = backdrop->group_capacity == 0 ? 8 : backdrop->group_capacity * 2;
        grown = (trace_render_svg_group_anchor_t*)realloc(backdrop->groups, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        backdrop->groups = grown;
        backdrop->group_capacity = next;
    }
    backdrop->groups[backdrop->group_count].group = strdup(group);
    if (!backdrop->groups[backdrop->group_count].group) return false;
    backdrop->groups[backdrop->group_count].x = x;
    backdrop->groups[backdrop->group_count].y = y;
    backdrop->groups[backdrop->group_count].count = 1;
    backdrop->group_count++;
    return true;
}

static bool trace_render_push_route_anchor(trace_render_svg_backdrop_t *backdrop,
                                           const char *route_id,
                                           const char *group,
                                           const char *module_path,
                                           const char *handler,
                                           double x,
                                           double y) {
    trace_render_svg_route_anchor_t *grown = NULL;
    int next = 0;
    if (!backdrop || !route_id) return false;
    if (backdrop->route_count >= backdrop->route_capacity) {
        next = backdrop->route_capacity == 0 ? 8 : backdrop->route_capacity * 2;
        grown = (trace_render_svg_route_anchor_t*)realloc(backdrop->routes, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        backdrop->routes = grown;
        backdrop->route_capacity = next;
    }
    backdrop->routes[backdrop->route_count].route_id = strdup(route_id);
    if (!backdrop->routes[backdrop->route_count].route_id) return false;
    backdrop->routes[backdrop->route_count].group = group ? strdup(group) : NULL;
    backdrop->routes[backdrop->route_count].module_path = module_path ? strdup(module_path) : NULL;
    backdrop->routes[backdrop->route_count].handler = handler ? strdup(handler) : NULL;
    backdrop->routes[backdrop->route_count].x = x;
    backdrop->routes[backdrop->route_count].y = y;
    backdrop->route_count++;
    if (group && group[0] != '\0') (void)trace_render_push_group_anchor(backdrop, group, x, y);
    return true;
}

static bool trace_render_push_module_anchor(trace_render_svg_backdrop_t *backdrop,
                                            const char *module_path,
                                            double cx,
                                            double cy,
                                            double r,
                                            double scale) {
    trace_render_svg_module_anchor_t *grown = NULL;
    int next = 0;
    if (!backdrop || !module_path) return false;
    if (backdrop->module_count >= backdrop->module_capacity) {
        next = backdrop->module_capacity == 0 ? 8 : backdrop->module_capacity * 2;
        grown = (trace_render_svg_module_anchor_t*)realloc(backdrop->modules, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        backdrop->modules = grown;
        backdrop->module_capacity = next;
    }
    backdrop->modules[backdrop->module_count].module_path = strdup(module_path);
    if (!backdrop->modules[backdrop->module_count].module_path) return false;
    backdrop->modules[backdrop->module_count].cx = cx;
    backdrop->modules[backdrop->module_count].cy = cy;
    backdrop->modules[backdrop->module_count].r = r;
    backdrop->modules[backdrop->module_count].scale = scale;
    backdrop->module_count++;
    return true;
}

static const trace_render_svg_route_anchor_t *trace_render_backdrop_route(const trace_render_svg_backdrop_t *backdrop,
                                                                          const char *route_id) {
    int i = 0;
    if (!backdrop || !route_id) return NULL;
    for (i = 0; i < backdrop->route_count; i++) {
        if (strcmp(backdrop->routes[i].route_id, route_id) == 0) return &backdrop->routes[i];
    }
    return NULL;
}

static const trace_render_svg_module_anchor_t *trace_render_backdrop_module(const trace_render_svg_backdrop_t *backdrop,
                                                                            const char *module_path) {
    int i = 0;
    if (!backdrop || !module_path) return NULL;
    for (i = 0; i < backdrop->module_count; i++) {
        if (strcmp(backdrop->modules[i].module_path, module_path) == 0) return &backdrop->modules[i];
    }
    return NULL;
}

static const trace_render_svg_group_anchor_t *trace_render_backdrop_group(const trace_render_svg_backdrop_t *backdrop,
                                                                          const char *group) {
    int i = 0;
    if (!backdrop || !group) return NULL;
    for (i = 0; i < backdrop->group_count; i++) {
        if (strcmp(backdrop->groups[i].group, group) == 0) return &backdrop->groups[i];
    }
    return NULL;
}

static bool trace_render_backdrop_group_centroid(const trace_render_svg_backdrop_t *backdrop,
                                                 const char *group,
                                                 double *out_x,
                                                 double *out_y) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    int count = 0;
    int i = 0;
    if (!backdrop || !group || !out_x || !out_y) return false;
    for (i = 0; i < backdrop->route_count; i++) {
        if (!backdrop->routes[i].group || strcmp(backdrop->routes[i].group, group) != 0) continue;
        sum_x += backdrop->routes[i].x;
        sum_y += backdrop->routes[i].y;
        count++;
    }
    if (count == 0) return false;
    *out_x = sum_x / (double)count;
    *out_y = sum_y / (double)count;
    return true;
}

static bool trace_render_group_centroid_for_doc(const trace_render_svg_backdrop_t *backdrop,
                                                const cct_sigil_document_t *doc,
                                                const char *group,
                                                double *out_x,
                                                double *out_y) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    size_t count = 0;
    size_t i = 0;
    if (!backdrop || !doc || !group || !out_x || !out_y) return false;
    for (i = 0; i < doc->web_routes_count; i++) {
        const cct_sigil_web_route_t *route = &doc->web_routes[i];
        const trace_render_svg_route_anchor_t *route_anchor = NULL;
        if (!route->group || strcmp(route->group, group) != 0) continue;
        route_anchor = trace_render_backdrop_route(backdrop, route->route_id);
        if (!route_anchor) continue;
        sum_x += route_anchor->x;
        sum_y += route_anchor->y;
        count++;
    }
    if (count == 0) return false;
    *out_x = sum_x / (double)count;
    *out_y = sum_y / (double)count;
    return true;
}

static bool trace_render_route_prefix_centroid(const trace_render_svg_backdrop_t *backdrop,
                                               const char *prefix,
                                               double *out_x,
                                               double *out_y) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    int count = 0;
    int i = 0;
    size_t prefix_len = 0;
    if (!backdrop || !prefix || !out_x || !out_y) return false;
    prefix_len = strlen(prefix);
    for (i = 0; i < backdrop->route_count; i++) {
        if (!backdrop->routes[i].route_id) continue;
        if (strncmp(backdrop->routes[i].route_id, prefix, prefix_len) != 0) continue;
        sum_x += backdrop->routes[i].x;
        sum_y += backdrop->routes[i].y;
        count++;
    }
    if (count == 0) return false;
    *out_x = sum_x / (double)count;
    *out_y = sum_y / (double)count;
    return true;
}

static bool trace_render_push_ritual_anchor(trace_render_svg_backdrop_t *backdrop,
                                            const char *module_path,
                                            const char *ritual,
                                            double x,
                                            double y) {
    trace_render_svg_ritual_anchor_t *grown = NULL;
    int next = 0;
    if (!backdrop || !module_path || !ritual) return false;
    if (backdrop->ritual_count >= backdrop->ritual_capacity) {
        next = backdrop->ritual_capacity == 0 ? 16 : backdrop->ritual_capacity * 2;
        grown = (trace_render_svg_ritual_anchor_t*)realloc(backdrop->rituals, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        backdrop->rituals = grown;
        backdrop->ritual_capacity = next;
    }
    backdrop->rituals[backdrop->ritual_count].module_path = strdup(module_path);
    if (!backdrop->rituals[backdrop->ritual_count].module_path) return false;
    backdrop->rituals[backdrop->ritual_count].ritual = strdup(ritual);
    if (!backdrop->rituals[backdrop->ritual_count].ritual) return false;
    backdrop->rituals[backdrop->ritual_count].x = x;
    backdrop->rituals[backdrop->ritual_count].y = y;
    backdrop->ritual_count++;
    return true;
}

static const trace_render_svg_ritual_anchor_t *trace_render_backdrop_ritual(const trace_render_svg_backdrop_t *backdrop,
                                                                             const char *module_path,
                                                                             const char *ritual) {
    int i = 0;
    if (!backdrop || !module_path || !ritual) return NULL;
    for (i = 0; i < backdrop->ritual_count; i++) {
        if (strcmp(backdrop->rituals[i].module_path, module_path) == 0 &&
            strcmp(backdrop->rituals[i].ritual, ritual) == 0) return &backdrop->rituals[i];
    }
    return NULL;
}

static const char *trace_render_sigil_entry(const cct_sigil_document_t *doc, const char *section, const char *key) {
    size_t i = 0;
    if (!doc || !key) return NULL;
    for (i = 0; i < doc->entry_count; i++) {
        const cct_sigil_kv_t *kv = &doc->entries[i];
        const char *kv_section = kv->section ? kv->section : "";
        if (strcmp(kv_section, section ? section : "") == 0 && strcmp(kv->key ? kv->key : "", key) == 0) return kv->value;
    }
    return NULL;
}

static bool trace_render_parse_viewbox(const char *svg,
                                       double *out_min_x,
                                       double *out_min_y,
                                       double *out_w,
                                       double *out_h) {
    const char *root = NULL;
    const char *root_end = NULL;
    char *viewbox = NULL;
    double min_x = 0.0;
    double min_y = 0.0;
    double w = 0.0;
    double h = 0.0;
    if (!svg || !out_min_x || !out_min_y || !out_w || !out_h) return false;
    root = strstr(svg, "<svg");
    if (!root) return false;
    root_end = strchr(root, '>');
    if (!root_end) return false;
    viewbox = trace_render_extract_attr_range(root, root_end, "viewBox");
    if (!viewbox) return false;
    if (sscanf(viewbox, "%lf %lf %lf %lf", &min_x, &min_y, &w, &h) != 4) {
        free(viewbox);
        return false;
    }
    free(viewbox);
    *out_min_x = min_x;
    *out_min_y = min_y;
    *out_w = w;
    *out_h = h;
    return true;
}

static bool trace_render_capture_svg_prefix(const char *svg, trace_render_svg_backdrop_t *backdrop) {
    const char *close = NULL;
    if (!svg || !backdrop) return false;
    close = strstr(svg, "</svg>");
    if (close) {
        const char *next = close;
        while ((next = strstr(next + 1, "</svg>")) != NULL) close = next;
    }
    if (!close) return false;
    backdrop->svg_prefix = trace_render_strdup_range(svg, close);
    return backdrop->svg_prefix != NULL;
}

static bool trace_render_parse_route_anchors(const char *svg,
                                             const cct_sigil_document_t *doc,
                                             trace_render_svg_backdrop_t *backdrop) {
    const char *p = svg;
    if (!svg || !backdrop) return false;
    while ((p = strstr(p, "<g id=\"system_route_")) != NULL) {
        const char *tag_end = strchr(p, '>');
        const char *group_end = tag_end ? strstr(tag_end, "</g>") : NULL;
        char *route_id = NULL;
        const char *anchor = NULL;
        char *cx_text = NULL;
        char *cy_text = NULL;
        double x = 0.0;
        double y = 0.0;
        if (!tag_end || !group_end) break;
        route_id = trace_render_extract_attr_range(p, tag_end, "data-route-id");
        if (!route_id) {
            p = group_end + 4;
            continue;
        }
        anchor = strstr(tag_end, "class=\"route-core\"");
        if (!anchor || anchor >= group_end) anchor = strstr(tag_end, "class=\"route-shell\"");
        if (anchor && anchor < group_end) {
            const char *anchor_end = strchr(anchor, '>');
            if (anchor_end && anchor_end < group_end) {
                cx_text = trace_render_extract_attr_range(anchor, anchor_end, "cx");
                cy_text = trace_render_extract_attr_range(anchor, anchor_end, "cy");
            }
        }
        if (cx_text && cy_text) {
            const cct_sigil_web_route_t *route = doc ? trace_render_find_route_by_id(doc, route_id) : NULL;
            x = atof(cx_text);
            y = atof(cy_text);
            (void)trace_render_push_route_anchor(backdrop,
                                                 route_id,
                                                 route ? route->group : NULL,
                                                 route ? route->module : NULL,
                                                 route ? route->handler : NULL,
                                                 x,
                                                 y);
        }
        free(route_id);
        free(cx_text);
        free(cy_text);
        p = group_end + 4;
    }
    return backdrop->route_count > 0;
}

static bool trace_render_parse_module_anchors(const char *svg,
                                              const cct_sigil_document_t *doc,
                                              trace_render_svg_backdrop_t *backdrop) {
    const char *p = svg;
    if (!svg || !doc || !backdrop) return false;
    while ((p = strstr(p, "<g id=\"module_sigil_")) != NULL) {
        const char *tag_end = strchr(p, '>');
        char *id_text = NULL;
        char *transform = NULL;
        const char *module_path = NULL;
        int index = 0;
        double tx = 0.0;
        double ty = 0.0;
        double scale = 1.0;
        if (!tag_end) break;
        id_text = trace_render_extract_attr_range(p, tag_end, "id");
        transform = trace_render_extract_attr_range(p, tag_end, "transform");
        if (!id_text || !transform) {
            free(id_text);
            free(transform);
            p = tag_end + 1;
            continue;
        }
        if (sscanf(id_text, "module_sigil_%d", &index) != 1 ||
            sscanf(transform, "translate(%lf %lf) scale(%lf)", &tx, &ty, &scale) != 3) {
            free(id_text);
            free(transform);
            p = tag_end + 1;
            continue;
        }
        {
            char key[32];
            snprintf(key, sizeof(key), "%d", index);
            module_path = trace_render_sigil_entry(doc, "modules", key);
            if (module_path) {
                double cx = tx + scale * 256.0;
                double cy = ty + scale * 256.0;
                double r = scale * 256.0;
                const char *region = tag_end + 1;
                const char *region_end = strstr(region, "<g class=\"system-node-wrap\">");
                const char *next_module = strstr(region, "<g id=\"module_sigil_");
                (void)trace_render_push_module_anchor(backdrop, module_path, cx, cy, r, scale);
                if (next_module && (!region_end || next_module < region_end)) region_end = next_module;
                if (!region_end) region_end = svg + strlen(svg);
                while (region && region < region_end) {
                    const char *circle = strstr(region, "<circle");
                    const char *circle_end = NULL;
                    char *ritual = NULL;
                    char *kind = NULL;
                    char *cx_text = NULL;
                    char *cy_text = NULL;
                    if (!circle || circle >= region_end) break;
                    circle_end = strchr(circle, '>');
                    if (!circle_end || circle_end >= region_end) break;
                    ritual = trace_render_extract_attr_range(circle, circle_end, "data-ritual");
                    kind = trace_render_extract_attr_range(circle, circle_end, "data-kind");
                    if (ritual && kind && strcmp(kind, "rituale") == 0) {
                        cx_text = trace_render_extract_attr_range(circle, circle_end, "cx");
                        cy_text = trace_render_extract_attr_range(circle, circle_end, "cy");
                        if (cx_text && cy_text) {
                            double ritual_x = tx + scale * atof(cx_text);
                            double ritual_y = ty + scale * atof(cy_text);
                            (void)trace_render_push_ritual_anchor(backdrop, module_path, ritual, ritual_x, ritual_y);
                        }
                    }
                    free(ritual);
                    free(kind);
                    free(cx_text);
                    free(cy_text);
                    region = circle_end + 1;
                }
            }
        }
        free(id_text);
        free(transform);
        p = tag_end + 1;
    }
    return backdrop->module_count > 0;
}

static bool trace_render_load_svg_backdrop(const char *sigil_path,
                                           const cct_sigil_document_t *doc,
                                           trace_render_svg_backdrop_t *backdrop) {
    char *svg_path = NULL;
    char *svg_text = NULL;
    if (!sigil_path || !doc || !backdrop) return false;
    memset(backdrop, 0, sizeof(*backdrop));
    svg_path = trace_render_replace_suffix(sigil_path, ".sigil", ".svg");
    if (!svg_path) return false;
    svg_text = trace_render_read_file(svg_path);
    free(svg_path);
    if (!svg_text) return false;
    if (!trace_render_parse_viewbox(svg_text, &backdrop->min_x, &backdrop->min_y, &backdrop->width, &backdrop->height) ||
        !trace_render_capture_svg_prefix(svg_text, backdrop)) {
        free(svg_text);
        trace_render_svg_backdrop_dispose(backdrop);
        return false;
    }
    (void)trace_render_parse_route_anchors(svg_text, doc, backdrop);
    (void)trace_render_parse_module_anchors(svg_text, doc, backdrop);
    backdrop->ready = backdrop->route_count > 0 || backdrop->module_count > 0;
    free(svg_text);
    if (!backdrop->ready) {
        trace_render_svg_backdrop_dispose(backdrop);
        return false;
    }
    return true;
}

static const char *trace_render_find_key(const char *json, const char *key) {
    static char pattern[160];
    int written = 0;
    if (!json || !key) return NULL;
    written = snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    if (written <= 0 || (size_t)written >= sizeof(pattern)) return NULL;
    json = strstr(json, pattern);
    return json ? json + strlen(pattern) : NULL;
}

static char *trace_render_extract_json_string(const char *json, const char *key) {
    const char *p = trace_render_find_key(json, key);
    const char *start = NULL;
    if (!p || *p != '"') return NULL;
    p++;
    start = p;
    while (*p) {
        if (*p == '\\' && p[1] != '\0') {
            p += 2;
            continue;
        }
        if (*p == '"') break;
        p++;
    }
    if (*p != '"') return NULL;
    return trace_render_strdup_range(start, p);
}

static bool trace_render_extract_json_int(const char *json, const char *key, long long *out) {
    const char *p = trace_render_find_key(json, key);
    char *end = NULL;
    long long value = 0;
    if (!p || !out) return false;
    value = strtoll(p, &end, 10);
    if (end == p) return false;
    *out = value;
    return true;
}

static char *trace_render_extract_json_object_text(const char *json, const char *key) {
    const char *p = trace_render_find_key(json, key);
    const char *start = NULL;
    int depth = 0;
    bool in_string = false;
    if (!p || *p != '{') return NULL;
    start = p;
    while (*p) {
        if (*p == '"' && (p == start || p[-1] != '\\')) {
            in_string = !in_string;
        } else if (!in_string) {
            if (*p == '{') {
                depth++;
            } else if (*p == '}') {
                depth--;
                if (depth == 0) {
                    p++;
                    return trace_render_strdup_range(start, p);
                }
            }
        }
        p++;
    }
    return NULL;
}

static void trace_render_attr_dispose(TraceAttr *attr) {
    if (!attr) return;
    free(attr->key);
    free(attr->value);
    attr->key = NULL;
    attr->value = NULL;
}

static void trace_render_span_dispose(TraceRenderSpan *span) {
    int i = 0;
    if (!span) return;
    free(span->trace_id);
    free(span->span_id);
    free(span->parent_id);
    free(span->name);
    free(span->category);
    free(span->attributes_json);
    free(span->route_id);
    free(span->handler);
    free(span->module_path);
    free(span->http_method);
    free(span->http_path);
    for (i = 0; i < span->attr_count; i++) trace_render_attr_dispose(&span->attrs[i]);
    free(span->attrs);
    memset(span, 0, sizeof(*span));
    span->parent_index = -1;
}

static bool trace_render_push_attr(TraceAttr **attrs, int *count, int *capacity, TraceAttr *attr) {
    TraceAttr *grown = NULL;
    int next = 0;
    if (!attrs || !count || !capacity || !attr) return false;
    if (*count >= *capacity) {
        next = (*capacity == 0) ? 8 : (*capacity * 2);
        grown = (TraceAttr*)realloc(*attrs, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        *attrs = grown;
        *capacity = next;
    }
    (*attrs)[*count] = *attr;
    (*count)++;
    return true;
}

static bool trace_render_parse_attrs(const char *json, TraceAttr **out_attrs, int *out_count) {
    const char *p = json;
    int capacity = 0;
    TraceAttr *attrs = NULL;
    int count = 0;
    if (!out_attrs || !out_count) return false;
    *out_attrs = NULL;
    *out_count = 0;
    if (!json || strcmp(json, "{}") == 0) return true;
    while (p && isspace((unsigned char)*p)) p++;
    if (!p || *p != '{') return false;
    p++;
    while (*p) {
        TraceAttr attr;
        const char *key_start = NULL;
        const char *value_start = NULL;
        memset(&attr, 0, sizeof(attr));
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '}') break;
        if (*p != '"') goto fail;
        p++;
        key_start = p;
        while (*p) {
            if (*p == '\\' && p[1] != '\0') {
                p += 2;
                continue;
            }
            if (*p == '"') break;
            p++;
        }
        if (*p != '"') goto fail;
        attr.key = trace_render_strdup_range(key_start, p);
        if (!attr.key) goto fail;
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p != ':') goto fail;
        p++;
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '"') {
            p++;
            value_start = p;
            while (*p) {
                if (*p == '\\' && p[1] != '\0') {
                    p += 2;
                    continue;
                }
                if (*p == '"') break;
                p++;
            }
            if (*p != '"') goto fail;
            attr.value = trace_render_strdup_range(value_start, p);
            if (!attr.value) goto fail;
            p++;
        } else {
            value_start = p;
            while (*p && *p != ',' && *p != '}') p++;
            attr.value = trace_render_strdup_trimmed(value_start, p);
            if (!attr.value) goto fail;
        }
        if (!trace_render_push_attr(&attrs, &count, &capacity, &attr)) {
            trace_render_attr_dispose(&attr);
            goto fail;
        }
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == ',') {
            p++;
            continue;
        }
        if (*p == '}') break;
        if (*p != '\0') goto fail;
    }
    *out_attrs = attrs;
    *out_count = count;
    return true;

fail:
    if (attrs) {
        int i = 0;
        for (i = 0; i < count; i++) trace_render_attr_dispose(&attrs[i]);
    }
    free(attrs);
    return false;
}

const char *trace_render_attr_value(const TraceRenderSpan *span, const char *key) {
    int i = 0;
    if (!span || !key) return NULL;
    for (i = 0; i < span->attr_count; i++) {
        if (span->attrs[i].key && strcmp(span->attrs[i].key, key) == 0) return span->attrs[i].value;
    }
    return NULL;
}

static bool trace_render_parse_line(const char *line, TraceRenderSpan *out) {
    if (!line || !out || line[0] == '\0') return false;
    memset(out, 0, sizeof(*out));
    out->parent_index = -1;
    out->trace_id = trace_render_extract_json_string(line, "trace_id");
    out->span_id = trace_render_extract_json_string(line, "span_id");
    out->parent_id = trace_render_extract_json_string(line, "parent_id");
    out->name = trace_render_extract_json_string(line, "name");
    out->attributes_json = trace_render_extract_json_object_text(line, "attributes");
    if (!out->trace_id || !out->span_id || !out->name || !out->parent_id) {
        trace_render_span_dispose(out);
        return false;
    }
    if (!trace_render_extract_json_int(line, "start_time_ms", &out->start_us) ||
        !trace_render_extract_json_int(line, "end_time_ms", &out->end_us)) {
        trace_render_span_dispose(out);
        return false;
    }
    if (!trace_render_extract_json_int(line, "duration_ms", &out->duration_us)) {
        out->duration_us = out->end_us - out->start_us;
    }
    if (!trace_render_parse_attrs(out->attributes_json, &out->attrs, &out->attr_count)) {
        trace_render_span_dispose(out);
        return false;
    }
    {
        const char *value = trace_render_attr_value(out, "route_id");
        if (value) out->route_id = strdup(value);
        value = trace_render_attr_value(out, "handler");
        if (value) out->handler = strdup(value);
        value = trace_render_attr_value(out, "module");
        if (value) out->module_path = strdup(value);
        value = trace_render_attr_value(out, "http.method");
        if (value) out->http_method = strdup(value);
        value = trace_render_attr_value(out, "http.path");
        if (value) out->http_path = strdup(value);
        value = trace_render_attr_value(out, "category");
        if (value) out->category = strdup(value);
    }
    return true;
}

static bool trace_render_push_span(TraceRenderTrace *trace, TraceRenderSpan *span) {
    TraceRenderSpan *grown = NULL;
    int next = 0;
    if (!trace || !span) return false;
    if (trace->span_capacity == 0) {
        trace->spans = (TraceRenderSpan*)malloc(8u * sizeof(*trace->spans));
        if (!trace->spans) return false;
        memset(trace->spans, 0, 8u * sizeof(*trace->spans));
        trace->span_capacity = 8u;
    }
    if ((size_t)trace->span_count >= trace->span_capacity) {
        next = (int)(trace->span_capacity == 0 ? 8u : trace->span_capacity * 2u);
        grown = (TraceRenderSpan*)realloc(trace->spans, (size_t)next * sizeof(*grown));
        if (!grown) return false;
        memset(grown + trace->span_capacity, 0, ((size_t)next - trace->span_capacity) * sizeof(*grown));
        trace->spans = grown;
        trace->span_capacity = (size_t)next;
    }
    trace->spans[trace->span_count++] = *span;
    return true;
}

void trace_render_compute_depths(TraceRenderTrace *trace) {
    int i = 0;
    long long min_start = 0;
    long long max_end = 0;
    if (!trace) return;
    for (i = 0; i < trace->span_count; i++) {
        int j = 0;
        trace->spans[i].parent_index = -1;
        for (j = 0; j < trace->span_count; j++) {
            if (i == j) continue;
            if (trace->spans[i].parent_id &&
                trace->spans[j].span_id &&
                strcmp(trace->spans[i].parent_id, trace->spans[j].span_id) == 0) {
                trace->spans[i].parent_index = j;
                break;
            }
        }
    }
    for (i = 0; i < trace->span_count; i++) {
        int depth = 0;
        int parent = trace->spans[i].parent_index;
        while (parent >= 0) {
            depth++;
            parent = trace->spans[parent].parent_index;
        }
        trace->spans[i].depth = depth;
        if (i == 0 || trace->spans[i].start_us < min_start) min_start = trace->spans[i].start_us;
        if (i == 0 || trace->spans[i].end_us > max_end) max_end = trace->spans[i].end_us;
    }
    trace->total_us = (trace->span_count == 0 || max_end <= min_start) ? 0 : (max_end - min_start);
}

int trace_render_load_ctrace(const char *path, TraceRenderTrace *out) {
    char *text = NULL;
    char *save = NULL;
    char *line = NULL;
    if (!path || !out) return -1;
    memset(out, 0, sizeof(*out));
    out->input_path = strdup(path);
    if (!out->input_path) return -1;
    text = trace_render_read_file(path);
    if (!text) {
        fprintf(stderr, "trace render: unable to read %s\n", path);
        trace_render_free(out);
        return -1;
    }
    line = strtok_r(text, "\n", &save);
    while (line) {
        while (*line == ' ' || *line == '\t' || *line == '\r') line++;
        if (*line != '\0') {
            TraceRenderSpan span;
            if (!trace_render_parse_line(line, &span)) {
                out->errors++;
            } else {
                if (!out->trace_id) out->trace_id = strdup(span.trace_id);
                if (out->trace_id && strcmp(out->trace_id, span.trace_id) != 0) {
                    out->errors++;
                    trace_render_span_dispose(&span);
                } else if (!trace_render_push_span(out, &span)) {
                    trace_render_span_dispose(&span);
                    free(text);
                    trace_render_free(out);
                    return -1;
                }
            }
        }
        line = strtok_r(NULL, "\n", &save);
    }
    free(text);
    trace_render_compute_depths(out);
    if (out->span_count == 0) {
        out->errors++;
        fprintf(stderr, "trace render: empty trace %s\n", path);
        return -1;
    }
    return 0;
}

void trace_render_free(TraceRenderTrace *trace) {
    int i = 0;
    if (!trace) return;
    for (i = 0; i < trace->span_count; i++) trace_render_span_dispose(&trace->spans[i]);
    free(trace->spans);
    free(trace->input_path);
    free(trace->trace_id);
    memset(trace, 0, sizeof(*trace));
}

void trace_render_svg_escape(FILE *out, const char *text) {
    const char *p = text ? text : "";
    if (!out) return;
    while (*p) {
        if (*p == '&') fputs("&amp;", out);
        else if (*p == '<') fputs("&lt;", out);
        else if (*p == '>') fputs("&gt;", out);
        else if (*p == '"') fputs("&quot;", out);
        else fputc(*p, out);
        p++;
    }
}

static void trace_render_hash_u8(u64 *hash, u8 value) {
    *hash ^= (u64)value;
    *hash *= 1099511628211ULL;
}

static void trace_render_hash_str(u64 *hash, const char *text) {
    const char *p = text ? text : "";
    while (*p) {
        trace_render_hash_u8(hash, (u8)*p);
        p++;
    }
    trace_render_hash_u8(hash, 0);
}

static u64 trace_render_compute_hash(const TraceRenderTrace *trace) {
    int i = 0;
    u64 hash = 1469598103934665603ULL;
    if (!trace) return hash;
    trace_render_hash_str(&hash, trace->trace_id);
    for (i = 0; i < trace->span_count; i++) {
        int b = 0;
        trace_render_hash_str(&hash, trace->spans[i].span_id);
        trace_render_hash_str(&hash, trace->spans[i].parent_id);
        trace_render_hash_str(&hash, trace->spans[i].name);
        trace_render_hash_str(&hash, trace->spans[i].route_id);
        trace_render_hash_str(&hash, trace->spans[i].module_path);
        trace_render_hash_str(&hash, trace->spans[i].category);
        for (b = 0; b < 8; b++) trace_render_hash_u8(&hash, (u8)((trace->spans[i].duration_us >> (b * 8)) & 0xff));
    }
    return hash;
}

static double trace_render_hash_unit(u64 seed, u32 salt) {
    u64 x = seed ^ (0x9e3779b97f4a7c15ULL * (u64)(salt + 1u));
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    return (double)(x & 0xFFFFFFULL) / (double)0x1000000ULL;
}

static const cct_sigil_web_route_t *trace_render_find_route_by_id(const cct_sigil_document_t *doc, const char *route_id) {
    size_t i = 0;
    if (!doc || !route_id) return NULL;
    for (i = 0; i < doc->web_routes_count; i++) {
        if (doc->web_routes[i].route_id && strcmp(doc->web_routes[i].route_id, route_id) == 0) return &doc->web_routes[i];
    }
    return NULL;
}

static u64 trace_render_route_seed(const cct_sigil_web_route_t *route) {
    u64 seed = 1469598103934665603ULL;
    const char *hex = route ? route->route_hash : NULL;
    if (hex && hex[0] != '\0') {
        char *end = NULL;
        unsigned long long parsed = strtoull(hex, &end, 16);
        if (end && *end == '\0') return (u64)parsed;
    }
    trace_render_hash_str(&seed, route ? route->route_id : NULL);
    trace_render_hash_str(&seed, route ? route->path : NULL);
    return seed;
}

static void trace_render_svg_arc(FILE *out, const char *cls, double x, double y, double r, double a0, double a1) {
    double sx = x + cos(a0) * r;
    double sy = y + sin(a0) * r;
    double ex = x + cos(a1) * r;
    double ey = y + sin(a1) * r;
    int large_arc = fabs(a1 - a0) > TRACE_RENDER_PI ? 1 : 0;
    int sweep = a1 >= a0 ? 1 : 0;
    fprintf(out,
            "<path class=\"%s\" d=\"M %.2f %.2f A %.2f %.2f 0 %d %d %.2f %.2f\"/>",
            cls, sx, sy, r, r, large_arc, sweep, ex, ey);
}

static void trace_render_emit_route_hit(FILE *out, const cct_sigil_web_route_t *route, double x, double y, double base_r) {
    u64 seed = trace_render_route_seed(route);
    double shell_r = base_r * (1.22 + trace_render_hash_unit(seed, 1) * 0.24);
    double mid_r = base_r * (0.82 + trace_render_hash_unit(seed, 2) * 0.18);
    double core_r = base_r * (0.42 + trace_render_hash_unit(seed, 3) * 0.12);
    double tilt = trace_render_hash_unit(seed, 4) * (2.0 * TRACE_RENDER_PI);
    u32 spoke_count = 3u + (u32)(trace_render_hash_unit(seed, 5) * 5.0);
    u32 i = 0;

    fprintf(out, "<circle class=\"route-shell\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, shell_r);
    fprintf(out, "<circle class=\"route-mid\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, mid_r);
    trace_render_svg_arc(out, "route-arc", x, y, shell_r * 0.92, tilt - 0.68, tilt + 0.84);
    trace_render_svg_arc(out, "route-arc", x, y, shell_r * 0.74, tilt + 1.52, tilt + 2.66);
    for (i = 0; i < spoke_count; i++) {
        double a = tilt + (2.0 * TRACE_RENDER_PI) * (double)i / (double)spoke_count;
        double ex = x + cos(a) * (shell_r - 1.8);
        double ey = y + sin(a) * (shell_r - 1.8);
        fprintf(out,
                "<path class=\"route-spoke\" d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\"/>",
                x, y,
                x + cos(a + TRACE_RENDER_PI * 0.5) * (base_r * 0.12),
                y + sin(a + TRACE_RENDER_PI * 0.5) * (base_r * 0.12),
                ex, ey);
    }
    fprintf(out, "<circle class=\"route-hit\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>", x, y, core_r);
}

static void trace_render_build_overlay_classes(char *buf, size_t cap, const char *base, const SpanOverlayMeta *meta) {
    const OverlayStyle *style = trace_overlay_style(meta ? meta->category : SPAN_CAT_GENERIC);
    int written = 0;
    if (!buf || cap == 0) return;
    written = snprintf(buf, cap, "%s %s%s%s%s",
                       base ? base : "",
                       style->css_class,
                       (meta && meta->is_slow) ? " slow" : "",
                       (meta && meta->is_error) ? " error" : "",
                       style->pulse ? " pulse" : "");
    if (written < 0 || (size_t)written >= cap) buf[cap - 1] = '\0';
}

static int trace_render_collect_active_categories(const TraceRenderTrace *trace,
                                                  const TraceRenderInput *input,
                                                  SpanCategory *cats,
                                                  int cat_cap) {
    bool seen[SPAN_CAT_GENERIC + 1];
    int count = 0;
    int i = 0;
    memset(seen, 0, sizeof(seen));
    if (!trace || !cats || cat_cap <= 0) return 0;
    for (i = 0; i < trace->span_count; i++) {
        SpanOverlayMeta meta;
        if (!trace_render_matches_filter(input, &trace->spans[i])) continue;
        trace_overlay_compute_meta(&trace->spans[i], &meta);
        if (!seen[meta.category] && count < cat_cap) {
            cats[count++] = meta.category;
            seen[meta.category] = true;
        }
        if (meta.is_error && !seen[SPAN_CAT_ERROR] && count < cat_cap) {
            cats[count++] = SPAN_CAT_ERROR;
            seen[SPAN_CAT_ERROR] = true;
        }
    }
    return count;
}

static bool trace_render_matches_filter(const TraceRenderInput *input, const TraceRenderSpan *span) {
    const char *filter = input ? input->filter_kind : NULL;
    if (!span) return false;
    if (filter && filter[0] != '\0') {
        const char *category = trace_overlay_category_slug(trace_overlay_resolve_category(span));
        if (!category || strcmp(category, filter) != 0) return false;
    }
    return true;
}

static trace_render_bounds_t trace_render_collect_bounds(const TraceRenderTrace *trace, const TraceRenderInput *input) {
    trace_render_bounds_t bounds;
    int i = 0;
    memset(&bounds, 0, sizeof(bounds));
    bounds.min_start = 0;
    bounds.max_end = 0;
    bounds.max_depth = 0;
    for (i = 0; i < trace->span_count; i++) {
        if (!trace_render_matches_filter(input, &trace->spans[i])) continue;
        if (bounds.min_start == 0 || trace->spans[i].start_us < bounds.min_start) bounds.min_start = trace->spans[i].start_us;
        if (trace->spans[i].end_us > bounds.max_end) bounds.max_end = trace->spans[i].end_us;
        if (trace->spans[i].depth > bounds.max_depth) bounds.max_depth = trace->spans[i].depth;
    }
    if (bounds.max_end <= bounds.min_start && trace->span_count > 0) {
        bounds.min_start = trace->spans[0].start_us;
        bounds.max_end = trace->spans[0].end_us;
    }
    return bounds;
}

static int trace_render_sorted_indices(const TraceRenderTrace *trace, int **out_indices, const TraceRenderInput *input) {
    int *indices = NULL;
    int count = 0;
    int i = 0;
    int j = 0;
    if (!trace || !out_indices) return -1;
    indices = (int*)malloc((size_t)(trace->span_count > 0 ? trace->span_count : 1) * sizeof(*indices));
    if (!indices) return -1;
    for (i = 0; i < trace->span_count; i++) {
        if (!trace_render_matches_filter(input, &trace->spans[i])) continue;
        indices[count++] = i;
    }
    for (i = 0; i < count; i++) {
        for (j = i + 1; j < count; j++) {
            const TraceRenderSpan *a = &trace->spans[indices[i]];
            const TraceRenderSpan *b = &trace->spans[indices[j]];
            if (a->start_us > b->start_us ||
                (a->start_us == b->start_us && a->duration_us > b->duration_us)) {
                int tmp = indices[i];
                indices[i] = indices[j];
                indices[j] = tmp;
            }
        }
    }
    *out_indices = indices;
    return count;
}

static int trace_render_active_count(const TraceRenderTrace *trace, const TraceRenderInput *input) {
    int *indices = NULL;
    int count = trace_render_sorted_indices(trace, &indices, input);
    int active = count;
    if (count < 0) return -1;
    if (input && input->mode == TRACE_RENDER_STEP) {
        if (input->step_index < 0) active = 0;
        else if (input->step_index >= count) active = count + 1;
        else active = input->step_index + 1;
    }
    free(indices);
    return active;
}

static void trace_render_panel_defaults(trace_render_panel_t *panel, double x, double y, double w, double h) {
    if (!panel) return;
    panel->panel_x = x;
    panel->panel_y = y;
    panel->panel_w = w;
    panel->panel_h = h;
    panel->cx = x + w * 0.5;
    panel->cy = y + h * 0.36;
    panel->core_r = w * 0.075;
    panel->timeline_x = x + 26.0;
    panel->timeline_y = y + h - 124.0;
    panel->timeline_w = w - 52.0;
    panel->timeline_h = 92.0;
}

static int trace_render_collect_routes(const TraceRenderTrace *trace,
                                       const TraceRenderInput *input,
                                       const cct_sigil_document_t *sigil_doc,
                                       trace_render_route_hit_t **out_hits) {
    trace_render_route_hit_t *hits = NULL;
    int count = 0;
    int i = 0;
    if (!out_hits) return -1;
    *out_hits = NULL;
    if (!sigil_doc || !trace) return 0;
    hits = (trace_render_route_hit_t*)calloc((size_t)(trace->span_count > 0 ? trace->span_count : 1), sizeof(*hits));
    if (!hits) return -1;
    for (i = 0; i < trace->span_count; i++) {
        const cct_sigil_web_route_t *route = NULL;
        int r = 0;
        if (!trace_render_matches_filter(input, &trace->spans[i])) continue;
        if (!trace->spans[i].route_id) continue;
        route = trace_render_find_route_by_id(sigil_doc, trace->spans[i].route_id);
        if (!route) continue;
        for (r = 0; r < count; r++) {
            if (hits[r].route == route) {
                route = NULL;
                break;
            }
        }
        if (route) hits[count++].route = route;
    }
    *out_hits = hits;
    return count;
}

static int trace_render_route_index(const trace_render_route_hit_t *hits, int hit_count, const char *route_id) {
    int i = 0;
    if (!hits || !route_id) return -1;
    for (i = 0; i < hit_count; i++) {
        if (hits[i].route && hits[i].route->route_id && strcmp(hits[i].route->route_id, route_id) == 0) return i;
    }
    return -1;
}

static int trace_render_first_route_span(const TraceRenderTrace *trace, const TraceRenderInput *input, const char *route_id) {
    int i = 0;
    int best = -1;
    if (!trace || !route_id) return -1;
    for (i = 0; i < trace->span_count; i++) {
        if (!trace_render_matches_filter(input, &trace->spans[i])) continue;
        if (!trace->spans[i].route_id || strcmp(trace->spans[i].route_id, route_id) != 0) continue;
        if (best < 0 ||
            trace->spans[i].start_us < trace->spans[best].start_us ||
            (trace->spans[i].start_us == trace->spans[best].start_us && trace->spans[i].depth < trace->spans[best].depth)) {
            best = i;
        }
    }
    return best;
}

static int trace_render_unique_slot(const char *text, char **keys, int *count, int limit) {
    int i = 0;
    if (!text || !keys || !count || limit <= 0) return 0;
    for (i = 0; i < *count; i++) {
        if (keys[i] && strcmp(keys[i], text) == 0) return i;
    }
    if (*count >= limit) return *count - 1;
    keys[*count] = strdup(text);
    if (!keys[*count]) return 0;
    (*count)++;
    return *count - 1;
}

static void trace_render_free_slots(char **keys, int count) {
    int i = 0;
    if (!keys) return;
    for (i = 0; i < count; i++) free(keys[i]);
}

static const char *trace_render_span_middleware_name(const TraceRenderSpan *span) {
    const char *value = NULL;
    if (!span) return NULL;
    value = trace_render_attr_value(span, "middleware");
    if (value && value[0] != '\0') return value;
    if (span->name && strncmp(span->name, "middleware.", 11) == 0 && span->name[11] != '\0') return span->name + 11;
    return NULL;
}

static const char *trace_render_span_handler_name(const TraceRenderSpan *span,
                                                  const cct_sigil_web_route_t *route) {
    if (span && span->handler && span->handler[0] != '\0') return span->handler;
    if (route && route->handler && route->handler[0] != '\0') return route->handler;
    if (span && span->name && strncmp(span->name, "handler.", 8) == 0 && span->name[8] != '\0') return span->name + 8;
    return NULL;
}

static bool trace_render_path_matches_module_name(const char *module_path, const char *name) {
    char suffix[96];
    size_t path_len = 0;
    size_t suffix_len = 0;
    if (!module_path || !name || name[0] == '\0') return false;
    snprintf(suffix, sizeof(suffix), "/%s.cct", name);
    path_len = strlen(module_path);
    suffix_len = strlen(suffix);
    return path_len >= suffix_len && strcmp(module_path + path_len - suffix_len, suffix) == 0;
}

static const trace_render_svg_module_anchor_t *trace_render_backdrop_module_by_name(const trace_render_svg_backdrop_t *backdrop,
                                                                                     const char *name) {
    int i = 0;
    if (!backdrop || !name) return NULL;
    for (i = 0; i < backdrop->module_count; i++) {
        if (trace_render_path_matches_module_name(backdrop->modules[i].module_path, name)) return &backdrop->modules[i];
    }
    return NULL;
}

static bool trace_render_backdrop_ritual_prefix_centroid(const trace_render_svg_backdrop_t *backdrop,
                                                         const char *module_path,
                                                         const char *prefix,
                                                         double *out_x,
                                                         double *out_y) {
    double sum_x = 0.0;
    double sum_y = 0.0;
    int count = 0;
    int i = 0;
    size_t prefix_len = 0;
    if (!backdrop || !module_path || !prefix || !out_x || !out_y) return false;
    prefix_len = strlen(prefix);
    for (i = 0; i < backdrop->ritual_count; i++) {
        if (strcmp(backdrop->rituals[i].module_path, module_path) != 0) continue;
        if (strncmp(backdrop->rituals[i].ritual, prefix, prefix_len) != 0) continue;
        sum_x += backdrop->rituals[i].x;
        sum_y += backdrop->rituals[i].y;
        count++;
    }
    if (count == 0) return false;
    *out_x = sum_x / (double)count;
    *out_y = sum_y / (double)count;
    return true;
}

static void trace_render_assign_positions(const TraceRenderTrace *trace,
                                          const TraceRenderInput *input,
                                          const trace_render_panel_t *panel,
                                          const cct_sigil_document_t *sigil_doc,
                                          const trace_render_svg_backdrop_t *backdrop,
                                          trace_render_route_hit_t *hits,
                                          int hit_count,
                                          trace_render_node_t *nodes) {
    u64 seed = trace_render_compute_hash(trace);
    char *module_slots[24];
    char *category_slots[24];
    int module_count = 0;
    int category_count = 0;
    int i = 0;
    int j = 0;
    memset(module_slots, 0, sizeof(module_slots));
    memset(category_slots, 0, sizeof(category_slots));
    for (i = 0; i < hit_count; i++) {
        const trace_render_svg_route_anchor_t *route_anchor =
            (backdrop && hits[i].route) ? trace_render_backdrop_route(backdrop, hits[i].route->route_id) : NULL;
        if (route_anchor) {
            hits[i].x = route_anchor->x;
            hits[i].y = route_anchor->y;
        } else {
            double x0 = panel->panel_x + 150.0;
            double x1 = panel->panel_x + panel->panel_w - 150.0;
            double t = hit_count == 1 ? 0.5 : (double)i / (double)(hit_count - 1);
            hits[i].x = x0 + (x1 - x0) * t;
            hits[i].y = panel->panel_y + 110.0 + (trace_render_hash_unit(seed, 740u + (u32)i) - 0.5) * 10.0;
        }
    }
    for (i = 0; i < trace->span_count; i++) {
        const TraceRenderSpan *span = &trace->spans[i];
        const cct_sigil_web_route_t *route_meta =
            (sigil_doc && span->route_id) ? trace_render_find_route_by_id(sigil_doc, span->route_id) : NULL;
        const trace_render_svg_module_anchor_t *module_anchor =
            (backdrop && span->module_path) ? trace_render_backdrop_module(backdrop, span->module_path) : NULL;
        const trace_render_svg_route_anchor_t *route_anchor =
            (backdrop && span->route_id) ? trace_render_backdrop_route(backdrop, span->route_id) : NULL;
        const trace_render_svg_group_anchor_t *group_anchor = NULL;
        const trace_render_svg_ritual_anchor_t *ritual_anchor = NULL;
        const char *middleware_name = trace_render_span_middleware_name(span);
        const char *handler_name = trace_render_span_handler_name(span, route_meta);
        double group_anchor_x = 0.0;
        double group_anchor_y = 0.0;
        int have_group_anchor = 0;
        double target_x = panel->cx;
        double target_y = panel->cy;
        double x = 0.0;
        double y = 0.0;
        double t = 0.0;
        int route_idx = -1;
        int module_slot = -1;
        int category_slot = -1;
        int sibling_count = 0;
        int sibling_slot = 0;
        const char *category = trace_overlay_category_slug(trace_overlay_resolve_category(span));
        nodes[i].visible = trace_render_matches_filter(input, span) ? 1 : 0;
        if (!nodes[i].visible) continue;
        nodes[i].active = 1;
        if (!module_anchor && route_meta && route_meta->module && backdrop) {
            module_anchor = trace_render_backdrop_module(backdrop, route_meta->module);
        }
        if (backdrop && module_anchor && handler_name) {
            ritual_anchor = trace_render_backdrop_ritual(backdrop, module_anchor->module_path, handler_name);
        }
        if (backdrop && middleware_name && middleware_name[0] != '\0') {
            char middleware_group[128];
            char middleware_prefix[128];
            snprintf(middleware_prefix, sizeof(middleware_prefix), "%s.", middleware_name);
            if (trace_render_route_prefix_centroid(backdrop, middleware_prefix, &group_anchor_x, &group_anchor_y)) {
                have_group_anchor = 1;
            }
            if (middleware_name[0] == '/') {
                group_anchor = trace_render_backdrop_group(backdrop, middleware_name);
                if (!group_anchor &&
                    (trace_render_group_centroid_for_doc(backdrop, sigil_doc, middleware_name, &group_anchor_x, &group_anchor_y) ||
                     trace_render_backdrop_group_centroid(backdrop, middleware_name, &group_anchor_x, &group_anchor_y))) {
                    have_group_anchor = 1;
                }
            } else {
                snprintf(middleware_group, sizeof(middleware_group), "/%s", middleware_name);
                group_anchor = trace_render_backdrop_group(backdrop, middleware_group);
                if (!group_anchor &&
                    (trace_render_group_centroid_for_doc(backdrop, sigil_doc, middleware_group, &group_anchor_x, &group_anchor_y) ||
                     trace_render_backdrop_group_centroid(backdrop, middleware_group, &group_anchor_x, &group_anchor_y))) {
                    have_group_anchor = 1;
                }
            }
            if (!group_anchor && route_meta && route_meta->group) {
                group_anchor = trace_render_backdrop_group(backdrop, route_meta->group);
                if (!group_anchor &&
                    (trace_render_group_centroid_for_doc(backdrop, sigil_doc, route_meta->group, &group_anchor_x, &group_anchor_y) ||
                     trace_render_backdrop_group_centroid(backdrop, route_meta->group, &group_anchor_x, &group_anchor_y))) {
                    have_group_anchor = 1;
                }
            }
            if (!group_anchor && backdrop) {
                const trace_render_svg_module_anchor_t *middleware_module =
                    trace_render_backdrop_module_by_name(backdrop, middleware_name);
                if (middleware_module) {
                    if (trace_render_backdrop_ritual_prefix_centroid(backdrop,
                                                                     middleware_module->module_path,
                                                                     middleware_name,
                                                                     &group_anchor_x,
                                                                     &group_anchor_y)) {
                        have_group_anchor = 1;
                    } else {
                        target_x = middleware_module->cx;
                        target_y = middleware_module->cy;
                    }
                }
            }
        }
        if (span->depth == 0) {
            nodes[i].x = route_anchor ? route_anchor->x : panel->cx;
            nodes[i].y = route_anchor ? route_anchor->y : panel->cy;
            nodes[i].r = route_anchor ? 8.8 : 16.0;
            if (route_anchor) nodes[i].mapped_route = 1;
            continue;
        }
        route_idx = trace_render_route_index(hits, hit_count, span->route_id);
        if (ritual_anchor) {
            target_x = ritual_anchor->x;
            target_y = ritual_anchor->y;
        } else if (group_anchor || have_group_anchor) {
            target_x = group_anchor ? group_anchor->x : group_anchor_x;
            target_y = group_anchor ? group_anchor->y : group_anchor_y;
        } else if (module_anchor) {
            target_x = module_anchor->cx;
            target_y = module_anchor->cy;
        } else if (route_idx >= 0) {
            target_x = hits[route_idx].x;
            target_y = hits[route_idx].y;
            nodes[i].mapped_route = 1;
        } else if (span->module_path && span->module_path[0] != '\0') {
            module_slot = trace_render_unique_slot(span->module_path, module_slots, &module_count, 24);
            target_x = panel->panel_x + panel->panel_w - 90.0;
            target_y = panel->panel_y + 128.0 + (double)module_slot * 58.0;
        } else if (category && strcmp(category, "generic") != 0) {
            category_slot = trace_render_unique_slot(category, category_slots, &category_count, 24);
            target_x = panel->panel_x + 130.0;
            target_y = panel->panel_y + 170.0 + (double)category_slot * 42.0;
        } else {
            target_x = panel->panel_x + 96.0;
            target_y = panel->panel_y + 112.0 + (double)(i % 6) * 54.0;
        }
        for (j = 0; j < trace->span_count; j++) {
            if (!nodes[j].visible) continue;
            if (trace->spans[j].parent_index == span->parent_index) {
                if (j == i) sibling_slot = sibling_count;
                sibling_count++;
            }
        }
        if (ritual_anchor) {
            x = ritual_anchor->x;
            y = ritual_anchor->y;
        } else if ((group_anchor || have_group_anchor) && category && strcmp(category, "middleware") == 0) {
            x = group_anchor ? group_anchor->x : group_anchor_x;
            y = group_anchor ? group_anchor->y : group_anchor_y;
        } else if (module_anchor && (span->depth == 1 || (category && strcmp(category, "handler") == 0))) {
            x = module_anchor->cx;
            y = module_anchor->cy;
        } else if (module_anchor && span->depth >= 2) {
            double orbit = module_anchor->r * 0.42;
            double orbit_center_x = module_anchor->cx;
            double orbit_center_y = module_anchor->cy;
            double slot = sibling_count <= 1 ? 0.0 : ((double)sibling_slot - ((double)(sibling_count - 1) * 0.5));
            double angle = -TRACE_RENDER_PI * 0.35 + slot * 0.55 + trace_render_hash_unit(seed, 980u + (u32)i) * 0.35;
            if (orbit > 86.0) orbit = 86.0;
            if (span->parent_index >= 0 &&
                nodes[span->parent_index].visible &&
                trace->spans[span->parent_index].module_path &&
                span->module_path &&
                strcmp(trace->spans[span->parent_index].module_path, span->module_path) == 0) {
                orbit_center_x = nodes[span->parent_index].x;
                orbit_center_y = nodes[span->parent_index].y;
                orbit = 30.0 + (double)(sibling_count > 0 ? sibling_count : 1) * 4.0;
                if (orbit > 54.0) orbit = 54.0;
            }
            x = orbit_center_x + cos(angle) * orbit;
            y = orbit_center_y + sin(angle) * orbit * 0.78;
        } else if (span->parent_index >= 0 && nodes[span->parent_index].visible) {
            double parent_x = nodes[span->parent_index].x;
            double parent_y = nodes[span->parent_index].y;
            double spread = sibling_count <= 1 ? 0.0 : ((double)sibling_slot - ((double)(sibling_count - 1) * 0.5));
            double normal_x = -(target_y - parent_y);
            double normal_y = target_x - parent_x;
            double normal_len = sqrt(normal_x * normal_x + normal_y * normal_y);
            if (normal_len < 0.001) {
                normal_x = 0.0;
                normal_y = 1.0;
                normal_len = 1.0;
            }
            normal_x /= normal_len;
            normal_y /= normal_len;
            t = 0.48 + (double)span->depth * 0.08;
            if (t > 0.72) t = 0.72;
            x = parent_x + (target_x - parent_x) * t + normal_x * spread * 38.0;
            y = parent_y + (target_y - parent_y) * t + normal_y * spread * 24.0;
        } else {
            t = 0.44 + (double)span->depth * 0.12;
            if (t > 0.86) t = 0.86;
            x = panel->cx + (target_x - panel->cx) * t;
            y = panel->cy + (target_y - panel->cy) * t;
        }
        if (!ritual_anchor && !((group_anchor || have_group_anchor) && category && strcmp(category, "middleware") == 0)) {
            x += (trace_render_hash_unit(seed, 900u + (u32)i) - 0.5) * 6.0;
            y += (trace_render_hash_unit(seed, 960u + (u32)i) - 0.5) * 6.0;
        }
        if (x < panel->panel_x + 72.0) x = panel->panel_x + 72.0;
        if (x > panel->panel_x + panel->panel_w - 72.0) x = panel->panel_x + panel->panel_w - 72.0;
        if (y < panel->panel_y + 92.0) y = panel->panel_y + 92.0;
        if (y > panel->timeline_y - 54.0) y = panel->timeline_y - 54.0;
        if (!isfinite(x) || !isfinite(y)) {
            x = panel->cx;
            y = panel->cy + 40.0 + (double)i * 24.0;
        }
        nodes[i].x = x;
        nodes[i].y = y;
        nodes[i].r = module_anchor ? 7.2 + trace_render_hash_unit(seed, 1024u + (u32)i) * 3.4
                                   : 10.0 + trace_render_hash_unit(seed, 1024u + (u32)i) * 4.0;
        if (sigil_doc && span->route_id && trace_render_find_route_by_id(sigil_doc, span->route_id)) nodes[i].mapped_route = 1;
    }
    trace_render_free_slots(module_slots, module_count);
    trace_render_free_slots(category_slots, category_count);
}

static void trace_render_link_curve(double x0, double y0, double x1, double y1, double *mx, double *my) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double lift = 10.0 + fabs(dy) * 0.10 + fabs(dx) * 0.04;
    if (!mx || !my) return;
    if (lift > 36.0) lift = 36.0;
    *mx = (x0 + x1) * 0.5;
    *my = (y0 + y1) * 0.5 - lift;
}

static void trace_render_emit_curve_path(FILE *out, double x0, double y0, double x1, double y1) {
    double mx = 0.0;
    double my = 0.0;
    trace_render_link_curve(x0, y0, x1, y1, &mx, &my);
    fprintf(out, "M %.2f %.2f Q %.2f %.2f %.2f %.2f", x0, y0, mx, my, x1, y1);
}

static double trace_render_anim_span_start(const TraceRenderTrace *trace,
                                           const trace_render_bounds_t *bounds,
                                           const TraceRenderSpan *span) {
    long long total = 0;
    if (!trace || !bounds || !span) return 0.0;
    total = bounds->max_end - bounds->min_start;
    if (total <= 0) total = trace->total_us;
    if (total <= 0) return 0.0;
    return ((double)(span->start_us - bounds->min_start) / (double)total) * 5.0;
}

static double trace_render_anim_link_duration(double start_delay, double arrival_delay) {
    double duration = arrival_delay - start_delay;
    if (duration < 0.24) duration = 0.24;
    if (duration > 1.20) duration = 1.20;
    return duration;
}

static double trace_render_anim_link_start(const TraceRenderTrace *trace,
                                           const trace_render_bounds_t *bounds,
                                           const TraceRenderSpan *span) {
    double child_delay = trace_render_anim_span_start(trace, bounds, span);
    double parent_delay = 0.0;
    if (!trace || !bounds || !span) return 0.0;
    if (span->parent_index >= 0 && span->parent_index < trace->span_count) {
        parent_delay = trace_render_anim_span_start(trace, bounds, &trace->spans[span->parent_index]);
        if (child_delay - parent_delay < 0.24) {
            parent_delay = child_delay - 0.24;
            if (parent_delay < 0.0) parent_delay = 0.0;
        }
        if (child_delay - parent_delay > 1.20) parent_delay = child_delay - 1.20;
        if (parent_delay < 0.0) parent_delay = 0.0;
        return parent_delay;
    }
    if (child_delay <= 0.24) return 0.0;
    return child_delay - 0.24;
}

static double trace_render_path_length_hint(double x0, double y0, double x1, double y1) {
    double dx = x1 - x0;
    double dy = y1 - y0;
    double dist = sqrt(dx * dx + dy * dy);
    return dist > 24.0 ? dist : 24.0;
}

static void trace_render_format_anim_begin(char *buf,
                                           size_t buf_size,
                                           double delay) {
    if (!buf || buf_size == 0u) return;
    snprintf(buf, buf_size, "%.2fs", delay);
}

static double trace_render_anim_cycle_duration(void) {
    return 5.35;
}

static void trace_render_emit_motion_packet(FILE *out,
                                            const char *class_buf,
                                            double radius,
                                            double delay,
                                            double duration,
                                            double x0,
                                            double y0,
                                            double x1,
                                            double y1) {
    char begin_buf[64];
    if (!out || !class_buf) return;
    trace_render_format_anim_begin(begin_buf, sizeof(begin_buf), delay);
    fprintf(out, "<circle class=\"trace-packet %s\" r=\"%.2f\">", class_buf, radius);
    fprintf(out,
            "<animate attributeName=\"opacity\" values=\"0;1;1;0\" begin=\"%s\" dur=\"%.2fs\" fill=\"freeze\"/>",
            begin_buf, duration);
    fprintf(out,
            "<animateMotion begin=\"%s\" dur=\"%.2fs\" fill=\"freeze\" path=\"",
            begin_buf, duration);
    trace_render_emit_curve_path(out, x0, y0, x1, y1);
    fprintf(out, "\"/></circle>\n");
}

static void trace_render_emit_step_state_attrs(FILE *out,
                                               int rank,
                                               const char *active_class,
                                               const char *pending_class) {
    if (!out || !active_class || !pending_class || rank < 0) return;
    fprintf(out,
            " data-step-rank=\"%d\" data-step-active-class=\"%s\" data-step-pending-class=\"%s\"",
            rank, active_class, pending_class);
}

static void trace_render_emit_style(FILE *out, int animated) {
    fprintf(out,
            "<style><![CDATA["
            ".bg{fill:#f6f0e6}"
            ".panel-bg{fill:#fbf8f2;stroke:#d4c9b9;stroke-width:1}"
            ".halo{fill:none;stroke:#dccfbf;stroke-width:1.1;opacity:.92}"
            ".halo-soft{fill:none;stroke:#ece2d5;stroke-width:.8;opacity:.78}"
            ".title{fill:#211812;font:600 18px monospace}"
            ".subtitle{fill:#5a5046;font:10px monospace}"
            ".hash{fill:#2a221c;font:10px monospace}"
            ".core{fill:#fffaf2;stroke:#786856;stroke-width:1.6}"
            ".core-mark{fill:none;stroke:#a39383;stroke-width:1.0;opacity:.95}"
            ".route-shell{fill:rgba(255,252,246,0.74);stroke:#5a4b3f;stroke-width:1.0}"
            ".route-mid{fill:none;stroke:#b9aa96;stroke-width:.9;opacity:.8}"
            ".route-arc{fill:none;stroke:#736252;stroke-width:.92;opacity:.86;stroke-linecap:round}"
            ".route-spoke{fill:none;stroke:#968471;stroke-width:.86;opacity:.8;stroke-linecap:round}"
            ".route-hit{fill:#5e4d3d}"
            ".span-link{fill:none;stroke:#776757;stroke-width:1.0;opacity:.72}"
            ".span-node{fill:#fbfaf6;stroke:#6c6256;stroke-width:1.0}"
            ".span-node-active{stroke:#3f6f9a;stroke-width:1.8}"
            ".span-node-pending{fill:#f2ede4;stroke:#b7a99a;opacity:.56}"
            ".span-node-hit{stroke:#5e4d3d;stroke-width:1.55}"
            ".span-core{fill:#efe5d6;stroke:#5e5143;stroke-width:1.1}"
            ".trace-orn{fill:none;stroke:#b9a895;stroke-width:.9;opacity:.62}"
            ".timeline-axis{fill:none;stroke:#8f8173;stroke-width:.8}"
            ".timeline-lane{fill:#f0e7da;stroke:#d5c8b5;stroke-width:.6}"
            ".timeline-row{fill:#f8f1e6;stroke:#dccfbe;stroke-width:.55}"
            ".timeline-bar{fill:#5f87af;opacity:.74}"
            ".timeline-bar-pending{fill:#d7ccbe;opacity:.52}"
            ".mapping-label{fill:#5a5046;font:10px monospace}"
            ".legend{fill:#40352c;font:10px monospace}"
            ".compare-divider{stroke:#d7cabb;stroke-width:1}"
            ".node-wrap,.edge-wrap,.timeline-entry{cursor:help}"
            ".timeline-entry:hover .timeline-bar,.timeline-entry:hover .timeline-bar-pending{opacity:.94;stroke:#2f261f;stroke-width:1.1}"
            ".node-wrap:hover .span-node,.node-wrap:hover .span-core{stroke:#2f261f;stroke-width:2.1}"
            ".route-flow{fill:none;stroke:#69584b;stroke-width:.95;opacity:.58}"
            ".step-marker{fill:#1e4f78}"
            ".step-rail{fill:none;stroke:#8f8173;stroke-width:1.2}"
            ".step-dot{fill:#efe4d4;stroke:#756657;stroke-width:1;cursor:pointer}"
            ".step-dot.active{fill:#1e4f78;stroke:#1e4f78}"
            ".step-knob{fill:#1e4f78;stroke:#fffaf2;stroke-width:1.2;cursor:grab}"
            ".step-knob.dragging{cursor:grabbing}"
            ".step-label{fill:#5a5046;font:10px monospace}"
            ".trace-caption-box{fill:rgba(255,250,242,0.92);stroke:#d6cabc;stroke-width:.9}"
            ".trace-caption-title{fill:#211812;font:600 14px monospace}"
            ".trace-caption-subtitle{fill:#5a5046;font:10px monospace}"
            ".trace-summary-box{fill:rgba(255,250,242,0.94);stroke:#d6cabc;stroke-width:.9}"
            ".trace-summary-title{fill:#211812;font:600 11px monospace}"
            ".trace-summary-line{fill:#5a5046;font:10px monospace}"
            ".focus-ring{fill:none;stroke:#35648c;stroke-width:1.2;stroke-dasharray:4 2}"
            ".trace-legend-wrap{cursor:grab}"
            ".trace-legend-wrap.dragging{cursor:grabbing}"
            ".trace-caption-wrap{cursor:grab}"
            ".trace-caption-wrap.dragging{cursor:grabbing}"
            ".trace-summary-wrap{cursor:grab}"
            ".trace-summary-wrap.dragging{cursor:grabbing}"
            ".anim-control-wrap{cursor:pointer}"
            ".anim-button{fill:rgba(255,250,242,0.96);stroke:#7b6b5c;stroke-width:.9}"
            ".anim-button:hover{fill:#fffdf8;stroke:#3f6f9a}"
            ".anim-button-text{fill:#2d241d;font:600 11px monospace}"
            ".anim-button-subtitle{fill:#5a5046;font:10px monospace}"
            "]]></style>\n");
    fprintf(out, "<style><![CDATA[");
    trace_overlay_emit_css(out);
    fprintf(out, "]]></style>\n");
    (void)animated;
}

static int trace_render_should_list_summary_span(const TraceRenderSpan *span) {
    if (!span || !span->name || span->name[0] == '\0') return 0;
    if (span->route_id && span->route_id[0] != '\0') return 1;
    if (span->depth <= 1 && span->category &&
        (strcmp(span->category, "handler") == 0 || strcmp(span->category, "middleware") == 0)) return 1;
    return 0;
}

static int trace_render_can_place_footer_boxes(const trace_render_panel_t *panel) {
    double footer_x = 0.0;
    double footer_w = 0.0;
    if (!panel) return 0;
    footer_x = panel->timeline_x + panel->timeline_w + 18.0;
    footer_w = (panel->panel_x + panel->panel_w) - footer_x - 18.0;
    return footer_w >= (352.0 + 14.0 + 338.0);
}

static void trace_render_emit_backdrop_root(FILE *out,
                                            const trace_render_svg_backdrop_t *backdrop,
                                            double canvas_height) {
    const char *prefix = NULL;
    const char *root = NULL;
    const char *root_end = NULL;
    const char *body = NULL;
    double footer_y = 0.0;
    double footer_h = 0.0;
    if (!out || !backdrop || !backdrop->svg_prefix) return;
    prefix = backdrop->svg_prefix;
    root = strstr(prefix, "<svg");
    if (!root) return;
    root_end = strchr(root, '>');
    if (!root_end) return;
    body = root_end + 1;
    fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out,
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.0f\" height=\"%.0f\" viewBox=\"%.2f %.2f %.2f %.2f\" role=\"img\" aria-label=\"CCT trace render\">\n",
            backdrop->width,
            canvas_height,
            backdrop->min_x,
            backdrop->min_y,
            backdrop->width,
            canvas_height);
    footer_y = backdrop->min_y + backdrop->height;
    footer_h = canvas_height - backdrop->height;
    if (footer_h > 0.0) {
        fprintf(out,
                "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" fill=\"#f6f0e6\"/>\n",
                backdrop->min_x,
                footer_y,
                backdrop->width,
                footer_h);
    }
    fprintf(out, "%s", body);
}

static void trace_render_emit_panel_frame(FILE *out, const trace_render_panel_t *panel, const char *title, const char *subtitle) {
    fprintf(out, "<rect class=\"panel-bg\" x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"18\"/>\n",
            panel->panel_x, panel->panel_y, panel->panel_w, panel->panel_h);
    fprintf(out, "<circle class=\"halo-soft\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", panel->cx, panel->cy, panel->panel_w * 0.28);
    fprintf(out, "<circle class=\"halo\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", panel->cx, panel->cy, panel->panel_w * 0.22);
    fprintf(out, "<circle class=\"halo-soft\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n", panel->cx, panel->cy, panel->panel_w * 0.15);
    fprintf(out, "<text class=\"title\" x=\"%.2f\" y=\"%.2f\">", panel->panel_x + 20.0, panel->panel_y + 28.0);
    trace_render_svg_escape(out, title);
    fprintf(out, "</text>\n");
    fprintf(out, "<text class=\"subtitle\" x=\"%.2f\" y=\"%.2f\">", panel->panel_x + 20.0, panel->panel_y + 45.0);
    trace_render_svg_escape(out, subtitle);
    fprintf(out, "</text>\n");
    fprintf(out,
            "<g><circle class=\"core\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/><circle class=\"core-mark\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>"
            "<path class=\"core-mark\" d=\"M %.2f %.2f L %.2f %.2f L %.2f %.2f L %.2f %.2f Z\"/></g>\n",
            panel->cx, panel->cy, panel->core_r,
            panel->cx, panel->cy, panel->core_r - 16.0,
            panel->cx, panel->cy - 24.0,
            panel->cx + 21.0, panel->cy,
            panel->cx, panel->cy + 24.0,
            panel->cx - 21.0, panel->cy);
}

static void trace_render_emit_caption(FILE *out,
                                      const trace_render_panel_t *panel,
                                      const char *title,
                                      const char *subtitle) {
    double x = 0.0;
    double y = 0.0;
    if (!out || !panel) return;
    x = panel->panel_x + 18.0;
    y = panel->panel_y + 18.0;
    if (trace_render_can_place_footer_boxes(panel)) {
        x = panel->timeline_x + panel->timeline_w + 18.0 + 352.0 + 14.0;
        y = panel->timeline_y + 8.0;
    }
    fprintf(out,
            "<g id=\"trace-caption-wrap\" class=\"trace-caption-wrap\" data-draggable=\"overlay\" data-base-class=\"trace-caption-wrap\" data-x=\"%.2f\" data-y=\"%.2f\" transform=\"translate(%.2f %.2f)\">",
            x, y, x, y);
    fprintf(out, "<g id=\"trace_caption\">");
    fprintf(out, "<rect class=\"trace-caption-box\" x=\"0\" y=\"0\" width=\"338\" height=\"46\" rx=\"10\"/>");
    fprintf(out, "<text class=\"trace-caption-title\" x=\"12\" y=\"18\">");
    trace_render_svg_escape(out, title ? title : "trace");
    fprintf(out, "</text>");
    fprintf(out, "<text class=\"trace-caption-subtitle\" x=\"12\" y=\"32\">");
    trace_render_svg_escape(out, subtitle ? subtitle : "");
    fprintf(out, "</text></g></g>\n");
}

static void trace_render_emit_summary_labels(FILE *out,
                                             const TraceRenderTrace *trace,
                                             const TraceRenderInput *input,
                                             const trace_render_panel_t *panel) {
    int *indices = NULL;
    int count = 0;
    int emitted = 0;
    int i = 0;
    double box_h = 0.0;
    double box_x = 0.0;
    double box_y = 0.0;
    if (!out || !trace || !panel) return;
    count = trace_render_sorted_indices(trace, &indices, input);
    if (count <= 0 || !indices) {
        free(indices);
        return;
    }
    for (i = 0; i < count; i++) {
        const TraceRenderSpan *span = &trace->spans[indices[i]];
        if (trace_render_should_list_summary_span(span)) emitted++;
        if (emitted >= 4) break;
    }
    if (emitted == 0) {
        free(indices);
        return;
    }
    box_h = 24.0 + (double)emitted * 14.0;
    box_x = panel->timeline_x + 8.0;
    box_y = panel->timeline_y - box_h - 12.0;
    if (trace_render_can_place_footer_boxes(panel)) {
        box_x = panel->timeline_x + panel->timeline_w + 18.0;
        box_y = panel->timeline_y + 8.0;
    }
    fprintf(out,
            "<g id=\"trace-summary-wrap\" class=\"trace-summary-wrap\" data-draggable=\"overlay\" data-base-class=\"trace-summary-wrap\" data-x=\"%.2f\" data-y=\"%.2f\" transform=\"translate(%.2f %.2f)\">",
            box_x, box_y, box_x, box_y);
    fprintf(out, "<rect class=\"trace-summary-box\" x=\"0\" y=\"0\" width=\"352\" height=\"%.2f\" rx=\"10\"/>", box_h);
    fprintf(out, "<text class=\"trace-summary-title\" x=\"12\" y=\"16\">trace spans</text>");
    emitted = 0;
    for (i = 0; i < count && emitted < 4; i++) {
        const TraceRenderSpan *span = &trace->spans[indices[i]];
        double line_y = 30.0 + (double)emitted * 14.0;
        if (!trace_render_should_list_summary_span(span)) continue;
        fprintf(out, "<text class=\"trace-summary-line\" x=\"12\" y=\"%.2f\">", line_y);
        trace_render_svg_escape(out, span->name);
        fprintf(out, " (%lldms)</text>", span->duration_us);
        emitted++;
    }
    fprintf(out, "</g>\n");
    free(indices);
}

static void trace_render_emit_animation_controls(FILE *out,
                                                 const TraceRenderInput *input,
                                                 const trace_render_panel_t *panel,
                                                 int backdrop_ready,
                                                 const char *id_prefix) {
    double x = 0.0;
    double y = 0.0;
    const char *prefix = id_prefix ? id_prefix : "trace";
    if (!out || !input || !input->animated || input->mode == TRACE_RENDER_STEP || !panel) return;
    x = panel->timeline_x + 8.0;
    y = panel->timeline_y - 44.0;
    if (trace_render_can_place_footer_boxes(panel)) {
        x = panel->timeline_x + panel->timeline_w + 18.0;
        y = panel->timeline_y + 86.0;
    } else if (backdrop_ready) {
        x = panel->panel_x + panel->panel_w - 116.0;
        y = panel->panel_y + 18.0;
    }
    fprintf(out,
            "<g id=\"%s-anim-controls\" class=\"anim-control-wrap\" data-anim-action=\"toggle\" transform=\"translate(%.2f %.2f)\">",
            prefix, x, y);
    fprintf(out, "<rect class=\"anim-button\" x=\"0\" y=\"0\" width=\"96\" height=\"28\" rx=\"9\"/>");
    fprintf(out, "<text id=\"%s-anim-button-label\" class=\"anim-button-text\" x=\"22\" y=\"18\">stop</text>", prefix);
    fprintf(out, "<title>toggle trace animation start/stop</title>");
    fprintf(out, "</g>\n");
    if (input->animation_loop) {
        fprintf(out,
                "<g id=\"%s-anim-status\" transform=\"translate(%.2f %.2f)\">",
                prefix, x + 104.0, y);
        fprintf(out, "<text class=\"anim-button-subtitle\" x=\"0\" y=\"18\">loop on</text>");
        fprintf(out, "</g>\n");
    }
}

static void trace_render_emit_timeline_internal(const TraceRenderTrace *trace,
                                                const TraceRenderInput *input,
                                                const trace_render_panel_t *panel,
                                                FILE *out,
                                                int active_limit) {
    trace_render_bounds_t bounds = trace_render_collect_bounds(trace, input);
    int *indices = NULL;
    int count = trace_render_sorted_indices(trace, &indices, input);
    int i = 0;
    int lane_count = bounds.max_depth + 1;
    double label_w = 72.0;
    double content_x = panel->timeline_x + label_w;
    double content_w = panel->timeline_w - label_w - 10.0;
    double scrubber_h = (input && input->mode == TRACE_RENDER_STEP) ? 24.0 : 0.0;
    double rows_h = panel->timeline_h - 18.0 - scrubber_h;
    double lane_height = rows_h / (double)(lane_count > 0 ? lane_count : 1);
    if (count < 0 || !out) {
        free(indices);
        return;
    }
    fprintf(out, "<g id=\"timeline\">\n");
    fprintf(out, "<rect class=\"timeline-lane\" x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"12\"/>\n",
            panel->timeline_x, panel->timeline_y, panel->timeline_w, panel->timeline_h);
    fprintf(out, "<path class=\"timeline-axis\" d=\"M %.2f %.2f L %.2f %.2f\"/>\n",
            content_x, panel->timeline_y + panel->timeline_h - 12.0,
            content_x + content_w, panel->timeline_y + panel->timeline_h - 12.0);
    for (i = 0; i < lane_count; i++) {
        double lane_y = panel->timeline_y + 8.0 + (double)i * lane_height;
        double row_y = lane_y + 4.0;
        double row_h = lane_height - 7.0;
        fprintf(out, "<text class=\"mapping-label\" x=\"%.2f\" y=\"%.2f\">depth %d</text>\n",
                panel->timeline_x + 8.0, lane_y + (lane_height * 0.5) + 3.0, i);
        fprintf(out, "<rect class=\"timeline-row\" x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" rx=\"7\"/>\n",
                content_x, row_y, content_w, row_h);
    }
    for (i = 0; i < count; i++) {
        const TraceRenderSpan *span = &trace->spans[indices[i]];
        SpanOverlayMeta meta;
        char class_buf[128];
        char active_class[128];
        char pending_class[128];
        double start_ratio = 0.0;
        double end_ratio = 1.0;
        double lane_top = panel->timeline_y + 8.0 + (double)span->depth * lane_height;
        double lane_y = lane_top + 6.0;
        double bar_h = lane_height - 10.0;
        double bar_x = 0.0;
        double bar_w = 0.0;
        int active = (active_limit < 0) || (i < active_limit);
        if (bounds.max_end > bounds.min_start) {
            start_ratio = (double)(span->start_us - bounds.min_start) / (double)(bounds.max_end - bounds.min_start);
            end_ratio = (double)(span->end_us - bounds.min_start) / (double)(bounds.max_end - bounds.min_start);
        }
        if (end_ratio < start_ratio) end_ratio = start_ratio;
        trace_overlay_compute_meta(span, &meta);
        bar_x = content_x + start_ratio * content_w;
        bar_w = (end_ratio - start_ratio) * content_w;
        if (bar_w < 2.0) bar_w = 2.0;
        trace_render_build_overlay_classes(active_class, sizeof(active_class), "timeline-bar", &meta);
        trace_render_build_overlay_classes(pending_class, sizeof(pending_class), "timeline-bar-pending", &meta);
        snprintf(class_buf, sizeof(class_buf), "%s", active ? active_class : pending_class);
        fprintf(out, "<g id=\"timeline-span-%s\" class=\"timeline-entry\"", span->span_id ? span->span_id : "unknown");
        trace_overlay_emit_attrs(&meta, out);
        fprintf(out, ">");
        fprintf(out, "<rect class=\"%s\"", class_buf);
        trace_overlay_emit_attrs(&meta, out);
        if (input && input->mode == TRACE_RENDER_STEP) {
            trace_render_emit_step_state_attrs(out, i, active_class, pending_class);
            fprintf(out, " data-step-role=\"timeline\"");
        }
        fprintf(out, " x=\"%.2f\" y=\"%.2f\" height=\"%.2f\" rx=\"5\"", bar_x, lane_y, bar_h);
        if (input && input->animated && active) {
            char begin_buf[64];
            double delay = start_ratio * 5.0;
            double duration = (bar_w / content_w) * 5.0;
            if (duration < 0.2) duration = 0.2;
            trace_render_format_anim_begin(begin_buf, sizeof(begin_buf), delay);
            fprintf(out, " width=\"0\"><animate attributeName=\"width\" from=\"0\" to=\"%.2f\" begin=\"%s\" dur=\"%.2fs\" fill=\"freeze\"/></rect>",
                    bar_w, begin_buf, duration);
        } else {
            fprintf(out, " width=\"%.2f\"/>", bar_w);
        }
        fprintf(out, "<title>");
        trace_render_svg_escape(out, span->name ? span->name : "");
        fprintf(out, "\ndepth=%d", span->depth);
        fprintf(out, "\nstart=%lldus", span->start_us);
        fprintf(out, "\nend=%lldus", span->end_us);
        fprintf(out, "\nduration=%lldus", span->duration_us);
        fprintf(out, "</title></g>\n");
    }
    if (input && input->mode == TRACE_RENDER_STEP && active_limit >= 0 && count > 0 && active_limit <= count) {
        int marker_index = active_limit == 0 ? 0 : active_limit - 1;
        double rail_x0 = content_x;
        double rail_x1 = content_x + content_w;
        double rail_y = panel->timeline_y + panel->timeline_h - 12.0;
        double knob_x = rail_x0;
        if (count > 1) knob_x = rail_x0 + ((double)marker_index / (double)(count - 1)) * content_w;
        else knob_x = rail_x0;
        fprintf(out, "<g id=\"step-scrubber\" data-step-count=\"%d\" data-step-current=\"%d\">", count, marker_index);
        fprintf(out, "<text class=\"step-label\" id=\"step-label\" x=\"%.2f\" y=\"%.2f\">step %d/%d</text>",
                content_x, rail_y - 10.0, marker_index + 1, count);
        fprintf(out,
                "<path id=\"step-rail\" class=\"step-rail\" data-x0=\"%.2f\" data-x1=\"%.2f\" data-y=\"%.2f\" d=\"M %.2f %.2f L %.2f %.2f\"/>",
                rail_x0, rail_x1, rail_y, rail_x0, rail_y, rail_x1, rail_y);
        for (i = 0; i < count; i++) {
            double dot_x = count > 1 ? rail_x0 + ((double)i / (double)(count - 1)) * content_w : rail_x0;
            fprintf(out, "<circle class=\"step-dot%s\" data-step-dot=\"%d\" cx=\"%.2f\" cy=\"%.2f\" r=\"3.7\">",
                    i == marker_index ? " active" : "", i, dot_x, rail_y);
            fprintf(out, "<title>step %d/%d</title></circle>", i + 1, count);
        }
        fprintf(out, "<circle id=\"step-knob\" class=\"step-knob\" cx=\"%.2f\" cy=\"%.2f\" r=\"6.0\"><title>drag or click the rail</title></circle>",
                knob_x, rail_y);
        fprintf(out, "</g>\n");
    }
    fprintf(out, "</g>\n");
    free(indices);
}

void trace_render_emit_timeline(TraceRenderTrace *trace, FILE *out) {
    trace_render_panel_t panel;
    TraceRenderInput input;
    memset(&input, 0, sizeof(input));
    trace_render_panel_defaults(&panel, 0.0, 0.0, 960.0, 720.0);
    trace_render_emit_timeline_internal(trace, &input, &panel, out, -1);
}

static void trace_render_emit_route_hits(FILE *out,
                                         const trace_render_route_hit_t *hits,
                                         int hit_count,
                                         const TraceRenderInput *input) {
    int i = 0;
    if (!hits || hit_count <= 0) return;
    fprintf(out, "<g id=\"trace_route_hits\">\n");
    for (i = 0; i < hit_count; i++) {
        fprintf(out, "<g class=\"node-wrap\" data-route-id=\"");
        trace_render_svg_escape(out, hits[i].route && hits[i].route->route_id ? hits[i].route->route_id : "");
        fprintf(out, "\">");
        trace_render_emit_route_hit(out, hits[i].route, hits[i].x, hits[i].y, 10.0);
        if (input && input->focus_route && hits[i].route && hits[i].route->route_id &&
            strcmp(input->focus_route, hits[i].route->route_id) == 0) {
            fprintf(out, "<circle class=\"focus-ring\" cx=\"%.2f\" cy=\"%.2f\" r=\"16\"/>", hits[i].x, hits[i].y);
        }
        fprintf(out, "<title>route ");
        trace_render_svg_escape(out, hits[i].route && hits[i].route->route_id ? hits[i].route->route_id : "");
        fprintf(out, " → ");
        trace_render_svg_escape(out, hits[i].route && hits[i].route->method ? hits[i].route->method : "");
        fprintf(out, " ");
        trace_render_svg_escape(out, hits[i].route && hits[i].route->path ? hits[i].route->path : "");
        fprintf(out, "</title></g>\n");
    }
    fprintf(out, "</g>\n");
}

static void trace_render_emit_links(FILE *out,
                                    const TraceRenderTrace *trace,
                                    const TraceRenderInput *input,
                                    const trace_render_route_hit_t *hits,
                                    int hit_count,
                                    const trace_render_node_t *nodes,
                                    int active_limit) {
    trace_render_bounds_t bounds = trace_render_collect_bounds(trace, input);
    int *indices = NULL;
    int count = trace_render_sorted_indices(trace, &indices, input);
    int i = 0;
    if (count < 0) return;
    fprintf(out, "<g id=\"trace_links\">\n");
    for (i = 0; i < trace->span_count; i++) {
        const TraceRenderSpan *span = &trace->spans[i];
        int active_rank = -1;
        int j = 0;
        if (!nodes[i].visible) continue;
        for (j = 0; j < count; j++) if (indices[j] == i) active_rank = j;
        if (span->parent_index >= 0 && nodes[span->parent_index].visible) {
            SpanOverlayMeta meta;
            char class_buf[128];
            char active_class[128];
            char pending_class[128];
            double px = nodes[span->parent_index].x;
            double py = nodes[span->parent_index].y;
            double x = nodes[i].x;
            double y = nodes[i].y;
            double delay = 0.0;
            double arrival = 0.0;
            double duration = 0.0;
            double dash = trace_render_path_length_hint(px, py, x, y);
            int active = (active_limit < 0) || (active_rank >= 0 && active_rank < active_limit);
            trace_overlay_compute_meta(span, &meta);
            trace_render_build_overlay_classes(active_class, sizeof(active_class), "span-link", &meta);
            trace_render_build_overlay_classes(pending_class, sizeof(pending_class), "span-link span-node-pending", &meta);
            snprintf(class_buf, sizeof(class_buf), "%s", active ? active_class : pending_class);
            fprintf(out, "<path class=\"%s\"", class_buf);
            trace_overlay_emit_attrs(&meta, out);
            if (input && input->mode == TRACE_RENDER_STEP) {
                trace_render_emit_step_state_attrs(out, active_rank, active_class, pending_class);
                fprintf(out, " data-step-role=\"link\"");
            }
                fprintf(out, " d=\"");
                trace_render_emit_curve_path(out, px, py, x, y);
                fprintf(out, "\"");
                if (input && input->animated && active) {
                    char begin_buf[64];
                    arrival = trace_render_anim_span_start(trace, &bounds, span);
                    delay = trace_render_anim_link_start(trace, &bounds, span);
                    duration = trace_render_anim_link_duration(delay, arrival);
                    trace_render_format_anim_begin(begin_buf, sizeof(begin_buf), delay);
                    fprintf(out, " style=\"stroke-dasharray:%.2f;stroke-dashoffset:%.2f\">", dash, dash);
                    fprintf(out,
                        "<animate attributeName=\"stroke-dashoffset\" from=\"%.2f\" to=\"0\" begin=\"%s\" dur=\"%.2fs\" fill=\"freeze\"/>",
                        dash, begin_buf, duration);
                    fprintf(out, "</path>\n");
                } else {
                    fprintf(out, "/>\n");
                }
            if (input && input->animated && active) trace_render_emit_motion_packet(out, class_buf, 4.10, delay, duration, px, py, x, y);
        }
        if (span->route_id) {
            int route_idx = trace_render_route_index(hits, hit_count, span->route_id);
            int primary_idx = trace_render_first_route_span(trace, input, span->route_id);
            if (route_idx >= 0 && primary_idx == i) {
                SpanOverlayMeta meta;
                char class_buf[128];
                char active_class[128];
                char pending_class[128];
                double delay = 0.0;
                double arrival = trace_render_anim_span_start(trace, &bounds, span);
                double duration = trace_render_anim_link_duration(0.0, arrival);
                double dash = trace_render_path_length_hint(hits[route_idx].x, hits[route_idx].y, nodes[i].x, nodes[i].y);
                double dx = hits[route_idx].x - nodes[i].x;
                double dy = hits[route_idx].y - nodes[i].y;
                if ((dx * dx + dy * dy) < 4.0) continue;
                trace_overlay_compute_meta(span, &meta);
                trace_render_build_overlay_classes(active_class, sizeof(active_class), "route-flow", &meta);
                trace_render_build_overlay_classes(pending_class, sizeof(pending_class), "route-flow span-node-pending", &meta);
                snprintf(class_buf, sizeof(class_buf), "%s",
                         ((active_limit < 0) || (active_rank >= 0 && active_rank < active_limit)) ? active_class : pending_class);
                fprintf(out, "<path class=\"%s\"", class_buf);
                trace_overlay_emit_attrs(&meta, out);
                if (input && input->mode == TRACE_RENDER_STEP) {
                    trace_render_emit_step_state_attrs(out, active_rank, active_class, pending_class);
                    fprintf(out, " data-step-role=\"route-link\"");
                }
                fprintf(out, " d=\"");
                trace_render_emit_curve_path(out, hits[route_idx].x, hits[route_idx].y, nodes[i].x, nodes[i].y);
                fprintf(out, "\"");
                if (input && input->animated) {
                    char begin_buf[64];
                    delay = trace_render_anim_link_start(trace, &bounds, span);
                    trace_render_format_anim_begin(begin_buf, sizeof(begin_buf), delay);
                    fprintf(out, " style=\"stroke-dasharray:%.2f;stroke-dashoffset:%.2f\">", dash, dash);
                    fprintf(out,
                            "<animate attributeName=\"stroke-dashoffset\" from=\"%.2f\" to=\"0\" begin=\"%s\" dur=\"%.2fs\" fill=\"freeze\"/>",
                            dash, begin_buf, duration);
                    fprintf(out, "</path>\n");
                } else {
                    fprintf(out, "/>\n");
                }
                if (input && input->animated) trace_render_emit_motion_packet(out, class_buf, 4.40, delay, duration, hits[route_idx].x, hits[route_idx].y, nodes[i].x, nodes[i].y);
            }
        }
    }
    fprintf(out, "</g>\n");
    free(indices);
}

static void trace_render_emit_nodes(FILE *out,
                                    const TraceRenderTrace *trace,
                                    const TraceRenderInput *input,
                                    const trace_render_node_t *nodes,
                                    int active_limit) {
    trace_render_bounds_t bounds = trace_render_collect_bounds(trace, input);
    int *indices = NULL;
    int count = trace_render_sorted_indices(trace, &indices, input);
    int i = 0;
    fprintf(out, "<g id=\"trace_nodes\">\n");
    if (count < 0) return;
    for (i = 0; i < trace->span_count; i++) {
        const TraceRenderSpan *span = &trace->spans[i];
        SpanOverlayMeta meta;
        char class_buf[160];
        char active_class[160];
        char pending_class[160];
        int active_rank = -1;
        int j = 0;
        int active = 1;
        if (!nodes[i].visible) continue;
        for (j = 0; j < count; j++) if (indices[j] == i) active_rank = j;
        active = (active_limit < 0) || (active_rank >= 0 && active_rank < active_limit);
        trace_overlay_compute_meta(span, &meta);
        fprintf(out, "<g id=\"span-%s\" class=\"node-wrap\" data-depth=\"%d\"",
                span->span_id ? span->span_id : "unknown", span->depth);
        trace_overlay_emit_attrs(&meta, out);
        if (span->route_id) {
            fprintf(out, " data-route-id=\"");
            trace_render_svg_escape(out, span->route_id);
            fprintf(out, "\"");
        }
        if (span->module_path) {
            fprintf(out, " data-module=\"");
            trace_render_svg_escape(out, span->module_path);
            fprintf(out, "\"");
        }
        fprintf(out, ">");
        trace_render_build_overlay_classes(active_class, sizeof(active_class),
                                           span->depth == 0 ? "span-core span-node-active" : "span-node span-node-active",
                                           &meta);
        trace_render_build_overlay_classes(pending_class, sizeof(pending_class),
                                           span->depth == 0 ? "span-core span-node-pending" : "span-node span-node-pending",
                                           &meta);
        snprintf(class_buf, sizeof(class_buf), "%s", active ? active_class : pending_class);
        if (nodes[i].mapped_route) {
            if (strlen(active_class) + strlen(" span-node-hit") + 1 < sizeof(active_class)) strcat(active_class, " span-node-hit");
            if (strlen(pending_class) + strlen(" span-node-hit") + 1 < sizeof(pending_class)) strcat(pending_class, " span-node-hit");
            if (strlen(class_buf) + strlen(" span-node-hit") + 1 < sizeof(class_buf)) strcat(class_buf, " span-node-hit");
        }
        fprintf(out, "<circle class=\"%s\"", class_buf);
        trace_overlay_emit_attrs(&meta, out);
        if (input && input->mode == TRACE_RENDER_STEP) {
            trace_render_emit_step_state_attrs(out, active_rank, active_class, pending_class);
            fprintf(out, " data-step-role=\"node\"");
        }
        fprintf(out, " cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"",
                nodes[i].x, nodes[i].y, nodes[i].r);
        if (input && input->animated && active) {
            char begin_buf[64];
            double delay = trace_render_anim_span_start(trace, &bounds, span);
            trace_render_format_anim_begin(begin_buf, sizeof(begin_buf), delay);
            fprintf(out, " opacity=\"0.18\">");
            fprintf(out,
                    "<animate attributeName=\"opacity\" from=\"0.18\" to=\"1\" begin=\"%s\" dur=\"0.18s\" fill=\"freeze\"/>",
                    begin_buf);
            fprintf(out, "</circle>");
        } else if (input && input->animated) {
            fprintf(out, " opacity=\"0.18\"/>");
        } else {
            fprintf(out, "/>");
        }
        fprintf(out, "<title>");
        trace_render_svg_escape(out, span->name ? span->name : "");
        fprintf(out, " (%lldms)", span->duration_us);
        if (span->route_id) {
            fprintf(out, "\nroute=");
            trace_render_svg_escape(out, span->route_id);
        }
        if (span->module_path) {
            fprintf(out, "\nmodule=");
            trace_render_svg_escape(out, span->module_path);
        }
        if (span->attributes_json && span->attributes_json[0] != '\0' && strcmp(span->attributes_json, "{}") != 0) {
            fprintf(out, "\n");
            trace_render_svg_escape(out, span->attributes_json);
        }
        fprintf(out, "</title></g>\n");
    }
    fprintf(out, "</g>\n");
    free(indices);
}

static int trace_render_panel(FILE *out,
                              const TraceRenderTrace *trace,
                              const TraceRenderInput *input,
                              const trace_render_panel_t *panel,
                              const char *panel_title,
                              const char *panel_subtitle) {
    cct_sigil_document_t sigil_doc;
    trace_render_svg_backdrop_t backdrop;
    bool has_sigil = false;
    trace_render_route_hit_t *hits = NULL;
    int hit_count = 0;
    trace_render_node_t *nodes = NULL;
    SpanCategory legend_cats[SPAN_CAT_GENERIC + 1];
    int legend_count = 0;
    int active_limit = -1;
    int i = 0;
    memset(&sigil_doc, 0, sizeof(sigil_doc));
    memset(&backdrop, 0, sizeof(backdrop));
    if (!out || !trace || !panel) return -1;
    if (input && input->sigil_path && input->sigil_path[0] != '\0') {
        has_sigil = cct_sigil_parse_file(input->sigil_path, CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT, &sigil_doc) &&
                    !cct_sigil_document_has_errors(&sigil_doc);
        if (has_sigil) (void)trace_render_load_svg_backdrop(input->sigil_path, &sigil_doc, &backdrop);
    }
    active_limit = trace_render_active_count(trace, input);
    nodes = (trace_render_node_t*)calloc((size_t)(trace->span_count > 0 ? trace->span_count : 1), sizeof(*nodes));
    if (!nodes) {
        trace_render_svg_backdrop_dispose(&backdrop);
        if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
        return -1;
    }
    hit_count = trace_render_collect_routes(trace, input, has_sigil ? &sigil_doc : NULL, &hits);
    if (hit_count < 0) {
        free(nodes);
        trace_render_svg_backdrop_dispose(&backdrop);
        if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
        return -1;
    }
    trace_render_assign_positions(trace, input, panel, has_sigil ? &sigil_doc : NULL, backdrop.ready ? &backdrop : NULL, hits, hit_count, nodes);
    legend_count = trace_render_collect_active_categories(trace, input, legend_cats, (int)(sizeof(legend_cats) / sizeof(legend_cats[0])));
    if (backdrop.ready) {
        trace_render_emit_caption(out, panel, panel_title, panel_subtitle);
    } else {
        trace_render_emit_panel_frame(out, panel, panel_title, panel_subtitle);
        fprintf(out, "<g id=\"trace_ornaments\">\n");
        for (i = 0; i < 12; i++) {
            u64 seed = trace_render_compute_hash(trace);
            double a = (2.0 * TRACE_RENDER_PI) * (double)i / 12.0;
            double rr = panel->panel_w * 0.34 + (trace_render_hash_unit(seed, 580u + (u32)i) - 0.5) * 22.0;
            double ox = panel->cx + cos(a) * rr;
            double oy = panel->cy + sin(a) * rr;
            fprintf(out, "<circle class=\"trace-orn\" cx=\"%.2f\" cy=\"%.2f\" r=\"%.2f\"/>\n",
                    ox, oy, 1.2 + trace_render_hash_unit(seed, 620u + (u32)i) * 2.0);
        }
        fprintf(out, "</g>\n");
        trace_render_emit_route_hits(out, hits, hit_count, input);
    }
    trace_render_emit_summary_labels(out, trace, input, panel);
    trace_render_emit_links(out, trace, input, hits, hit_count, nodes, active_limit);
    trace_render_emit_nodes(out, trace, input, nodes, active_limit);
    if (legend_count > 0) {
        double legend_x = panel->panel_x + panel->panel_w - 150.0;
        double legend_y = backdrop.ready ? panel->panel_y + 78.0 : panel->panel_y + 72.0;
        fprintf(out, "<g class=\"trace-legend-wrap\" data-draggable=\"overlay\" data-base-class=\"trace-legend-wrap\" data-x=\"%.2f\" data-y=\"%.2f\" transform=\"translate(%.2f %.2f)\">",
                legend_x, legend_y, legend_x, legend_y);
        trace_overlay_emit_legend(legend_cats, legend_count, out);
        fprintf(out, "</g>\n");
    }
    if (!(input && input->hide_timeline)) trace_render_emit_timeline_internal(trace, input, panel, out, active_limit);
    free(hits);
    free(nodes);
    trace_render_svg_backdrop_dispose(&backdrop);
    if (has_sigil) cct_sigil_document_dispose(&sigil_doc);
    return 0;
}

static void trace_render_emit_interaction_script(FILE *out,
                                                 int step_count,
                                                 int current_step,
                                                 int enable_legend_drag,
                                                 int enable_replay,
                                                 int enable_loop) {
    if (!out || (step_count <= 0 && !enable_legend_drag && !enable_replay && !enable_loop)) return;
    fprintf(out, "<script><![CDATA[\n");
    fprintf(out, "(function(){\n");
    fprintf(out, "var root=document.documentElement;\n");
    fprintf(out, "function q(sel){return Array.prototype.slice.call(root.querySelectorAll(sel));}\n");
    fprintf(out, "function toPoint(evt){var pt=root.createSVGPoint();pt.x=evt.clientX;pt.y=evt.clientY;return pt.matrixTransform(root.getScreenCTM().inverse());}\n");
    if (enable_replay) {
        fprintf(out, "var loopMs=%d;\n", (int)(trace_render_anim_cycle_duration() * 1000.0));
        fprintf(out, "var loopTimer=null;\n");
        fprintf(out, "var running=true;\n");
        fprintf(out, "function toggleLabel(){q('[data-anim-action=\"toggle\"]').forEach(function(btn){var label=btn.querySelector('text');if(label) label.textContent=running?'stop':'start';});}\n");
        fprintf(out, "function restartAnimations(){var restarted=false;\n");
        fprintf(out, " try{if(typeof root.pauseAnimations==='function') root.pauseAnimations();if(typeof root.setCurrentTime==='function'){root.setCurrentTime(0);restarted=true;}if(typeof root.unpauseAnimations==='function') root.unpauseAnimations();}catch(_err){}\n");
        fprintf(out, " if(!restarted){q('animate,animateMotion').forEach(function(el){var parent=el.parentNode;var next=el.nextSibling;var clone=el.cloneNode(true);if(!parent) return;parent.removeChild(el);if(next) parent.insertBefore(clone,next);else parent.appendChild(clone);});}}\n");
        fprintf(out, "function stopPlayback(){if(loopTimer){clearInterval(loopTimer);loopTimer=null;}try{if(typeof root.pauseAnimations==='function') root.pauseAnimations();}catch(_err){}running=false;toggleLabel();}\n");
        fprintf(out, "function startPlayback(){var t=0;try{if(typeof root.getCurrentTime==='function') t=root.getCurrentTime();}catch(_err){}if(t>5.10||t<0.01) restartAnimations();else {try{if(typeof root.unpauseAnimations==='function') root.unpauseAnimations();}catch(_err){restartAnimations();}}");
        if (enable_loop) {
            fprintf(out, " if(loopTimer){clearInterval(loopTimer);} loopTimer=setInterval(function(){restartAnimations();}, loopMs);");
        }
        fprintf(out, " running=true;toggleLabel();}\n");
        fprintf(out, " q('[data-anim-action=\"toggle\"]').forEach(function(btn){btn.addEventListener('click',function(evt){if(running) stopPlayback(); else startPlayback(); evt.preventDefault();});});\n");
        if (enable_loop) {
            fprintf(out, " loopTimer=setInterval(function(){restartAnimations();}, loopMs);\n");
        }
        fprintf(out, " toggleLabel();\n");
    }
    if (step_count > 0) {
        fprintf(out, "var knob=document.getElementById('step-knob');\n");
        fprintf(out, "var rail=document.getElementById('step-rail');\n");
        fprintf(out, "var label=document.getElementById('step-label');\n");
        fprintf(out, "var maxStep=%d;\n", step_count - 1);
        fprintf(out, "var current=%d;\n", current_step < 0 ? 0 : current_step);
        fprintf(out, "var draggingStep=false;\n");
        fprintf(out, "function railBounds(){return {x0:parseFloat(rail.getAttribute('data-x0')),x1:parseFloat(rail.getAttribute('data-x1')),y:parseFloat(rail.getAttribute('data-y'))};}\n");
        fprintf(out, "function clampStep(v){if(v<0) return 0;if(v>maxStep) return maxStep;return v;}\n");
        fprintf(out, "function applyState(step){\n");
        fprintf(out, " current=clampStep(step);\n");
        fprintf(out, " q('[data-step-role]').forEach(function(el){var rank=parseInt(el.getAttribute('data-step-rank')||'-1',10);var active=rank>=0&&rank<=current;el.setAttribute('class',active?el.getAttribute('data-step-active-class'):el.getAttribute('data-step-pending-class'));});\n");
        fprintf(out, " q('[data-step-dot]').forEach(function(dot){var idx=parseInt(dot.getAttribute('data-step-dot')||'0',10);dot.setAttribute('class',idx===current?'step-dot active':'step-dot');});\n");
        fprintf(out, " var b=railBounds(); var x=maxStep>0?b.x0+((current/maxStep)*(b.x1-b.x0)):b.x0; knob.setAttribute('cx',x); knob.setAttribute('cy',b.y); label.textContent='step '+(current+1)+'/%d';\n", step_count);
        fprintf(out, "}\n");
        fprintf(out, "function stepFromEvent(evt){var p=toPoint(evt);var b=railBounds();if(maxStep<=0) return 0;return clampStep(Math.round(((p.x-b.x0)/(b.x1-b.x0))*maxStep));}\n");
        fprintf(out, "function startStepDrag(evt){draggingStep=true;knob.setAttribute('class','step-knob dragging');applyState(stepFromEvent(evt));evt.preventDefault();}\n");
        fprintf(out, "function moveStepDrag(evt){if(!draggingStep) return;applyState(stepFromEvent(evt));evt.preventDefault();}\n");
        fprintf(out, "function endStepDrag(){draggingStep=false;if(knob) knob.setAttribute('class','step-knob');}\n");
        fprintf(out, "if(root&&knob&&rail&&label){\n");
        fprintf(out, " rail.addEventListener('pointerdown',startStepDrag);\n");
        fprintf(out, " knob.addEventListener('pointerdown',startStepDrag);\n");
        fprintf(out, " root.addEventListener('pointermove',moveStepDrag);\n");
        fprintf(out, " root.addEventListener('pointerup',endStepDrag);\n");
        fprintf(out, " root.addEventListener('pointerleave',endStepDrag);\n");
        fprintf(out, " q('[data-step-dot]').forEach(function(dot){dot.addEventListener('click',function(){applyState(parseInt(dot.getAttribute('data-step-dot')||'0',10));});});\n");
        fprintf(out, " applyState(current);\n");
        fprintf(out, "}\n");
    }
    if (enable_legend_drag) {
        fprintf(out, "q('[data-draggable=\"overlay\"]').forEach(function(box){\n");
        fprintf(out, " var dragging=false, dx=0, dy=0;\n");
        fprintf(out, " function boxX(){return parseFloat(box.getAttribute('data-x')||'0');}\n");
        fprintf(out, " function boxY(){return parseFloat(box.getAttribute('data-y')||'0');}\n");
        fprintf(out, " function baseClass(){return box.getAttribute('data-base-class')||'';}\n");
        fprintf(out, " function setBox(x,y){box.setAttribute('data-x',x);box.setAttribute('data-y',y);box.setAttribute('transform','translate('+x+' '+y+')');}\n");
        fprintf(out, " function startOverlayDrag(evt){var p=toPoint(evt);dragging=true;dx=p.x-boxX();dy=p.y-boxY();box.setAttribute('class',baseClass()+' dragging');evt.preventDefault();}\n");
        fprintf(out, " function moveOverlayDrag(evt){if(!dragging) return;var p=toPoint(evt);setBox(p.x-dx,p.y-dy);evt.preventDefault();}\n");
        fprintf(out, " function endOverlayDrag(){dragging=false;box.setAttribute('class',baseClass());}\n");
        fprintf(out, " box.addEventListener('pointerdown',startOverlayDrag);\n");
        fprintf(out, " root.addEventListener('pointermove',moveOverlayDrag);\n");
        fprintf(out, " root.addEventListener('pointerup',endOverlayDrag);\n");
        fprintf(out, " root.addEventListener('pointerleave',endOverlayDrag);\n");
        fprintf(out, "});\n");
    }
    fprintf(out, "})();\n");
    fprintf(out, "]]></script>\n");
}

int trace_render_single(TraceRenderInput *input, FILE *out) {
    trace_render_panel_t panel;
    cct_sigil_document_t sigil_doc;
    trace_render_svg_backdrop_t backdrop;
    double canvas_height = 720.0;
    char subtitle[256];
    char title[256];
    u64 hash = 0;
    int *indices = NULL;
    int count = 0;
    if (!input || !out || input->trace_a.span_count <= 0) return -1;
    memset(&sigil_doc, 0, sizeof(sigil_doc));
    memset(&backdrop, 0, sizeof(backdrop));
    if (input->sigil_path && cct_sigil_parse_file(input->sigil_path, CCT_SIGIL_PARSE_MODE_CURRENT_DEFAULT, &sigil_doc) &&
        !cct_sigil_document_has_errors(&sigil_doc)) {
        (void)trace_render_load_svg_backdrop(input->sigil_path, &sigil_doc, &backdrop);
    }
    if (backdrop.ready) {
        double footer_top = 24.0;
        double footer_bottom = 28.0;
        canvas_height = backdrop.height + footer_top + (input->mode == TRACE_RENDER_STEP ? 118.0 : 92.0) + footer_bottom;
        trace_render_panel_defaults(&panel, backdrop.min_x, backdrop.min_y, backdrop.width, canvas_height);
        panel.cx = backdrop.min_x + backdrop.width * 0.5;
        panel.cy = backdrop.min_y + backdrop.height * 0.36;
        panel.core_r = backdrop.width * 0.075;
        panel.timeline_x = backdrop.min_x + 22.0;
        panel.timeline_w = backdrop.width > 520.0 ? 440.0 : (backdrop.width - 44.0);
        if (panel.timeline_w < 300.0) panel.timeline_w = backdrop.width - 44.0;
        panel.timeline_h = input->mode == TRACE_RENDER_STEP ? 118.0 : 92.0;
        panel.timeline_y = backdrop.min_y + backdrop.height + footer_top;
    } else {
        trace_render_panel_defaults(&panel, 12.0, 12.0, 936.0, 696.0);
        if (input->mode == TRACE_RENDER_STEP && !input->hide_timeline) {
            panel.timeline_y -= 26.0;
            panel.timeline_h += 26.0;
        }
    }
    hash = trace_render_compute_hash(&input->trace_a);
    snprintf(title, sizeof(title), "trace %s", input->trace_a.trace_id ? input->trace_a.trace_id : "trace");
    snprintf(subtitle, sizeof(subtitle), "spans=%d total=%lldms mode=%s hash=%016llx",
             input->trace_a.span_count,
             input->trace_a.total_us,
             input->mode == TRACE_RENDER_STEP ? "step" : (input->animated ? "animated" : "static"),
             (unsigned long long)hash);
    if (backdrop.ready && backdrop.svg_prefix) {
        trace_render_emit_backdrop_root(out, &backdrop, canvas_height);
    } else {
        fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 960 720\" width=\"960\" height=\"720\" role=\"img\" aria-label=\"CCT trace render\">\n");
        fprintf(out, "<rect class=\"bg\" x=\"0\" y=\"0\" width=\"960\" height=\"720\"/>\n");
    }
    trace_render_emit_style(out, input->animated);
    if (trace_render_panel(out, &input->trace_a, input, &panel,
                           title,
                           subtitle) != 0) {
        trace_render_svg_backdrop_dispose(&backdrop);
        cct_sigil_document_dispose(&sigil_doc);
        fputs("</svg>\n", out);
        return -1;
    }
    trace_render_emit_animation_controls(out, input, &panel, backdrop.ready, "trace");
    if (input->mode == TRACE_RENDER_STEP) {
        count = trace_render_sorted_indices(&input->trace_a, &indices, input);
        free(indices);
        trace_render_emit_interaction_script(out, count, input->step_index, 1, 0, 0);
    } else {
        trace_render_emit_interaction_script(out, 0, 0, 1, input->animated, input->animation_loop);
    }
    if (!backdrop.ready) fprintf(out, "<text class=\"hash\" x=\"930\" y=\"704\" text-anchor=\"end\">%016llx</text>\n", (unsigned long long)hash);
    fputs("</svg>\n", out);
    trace_render_svg_backdrop_dispose(&backdrop);
    cct_sigil_document_dispose(&sigil_doc);
    return 0;
}

int trace_render_step(TraceRenderInput *input, FILE *out) {
    int *indices = NULL;
    int count = 0;
    if (!input) return -1;
    input->mode = TRACE_RENDER_STEP;
    count = trace_render_sorted_indices(&input->trace_a, &indices, input);
    free(indices);
    if (count < 0) return -1;
    if (input->step_index >= count) return 1;
    return trace_render_single(input, out);
}

int trace_render_compare(TraceRenderInput *input, FILE *out) {
    trace_render_panel_t left;
    trace_render_panel_t right;
    long long delta = 0;
    char title_left[256];
    char title_right[256];
    char subtitle_left[256];
    char subtitle_right[256];
    if (!input || !out || input->trace_a.span_count <= 0 || input->trace_b.span_count <= 0) return -1;
    trace_render_panel_defaults(&left, 12.0, 12.0, 456.0, 696.0);
    trace_render_panel_defaults(&right, 492.0, 12.0, 456.0, 696.0);
    delta = input->trace_b.total_us - input->trace_a.total_us;
    snprintf(title_left, sizeof(title_left), "trace %s", input->trace_a.trace_id ? input->trace_a.trace_id : "trace-a");
    snprintf(title_right, sizeof(title_right), "trace %s", input->trace_b.trace_id ? input->trace_b.trace_id : "trace-b");
    snprintf(subtitle_left, sizeof(subtitle_left), "spans=%d total=%lldms baseline",
             input->trace_a.span_count, input->trace_a.total_us);
    snprintf(subtitle_right, sizeof(subtitle_right), "spans=%d total=%lldms delta=%+lldms",
             input->trace_b.span_count, input->trace_b.total_us, delta);
    fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 960 720\" width=\"960\" height=\"720\" role=\"img\" aria-label=\"CCT trace compare\">\n");
    trace_render_emit_style(out, input->animated);
    fprintf(out, "<rect class=\"bg\" x=\"0\" y=\"0\" width=\"960\" height=\"720\"/>\n");
    fprintf(out, "<path class=\"compare-divider\" d=\"M 480 20 L 480 700\"/>\n");
    if (trace_render_panel(out, &input->trace_a, input, &left,
                           title_left,
                           subtitle_left) != 0 ||
        trace_render_panel(out, &input->trace_b, input, &right,
                           title_right,
                           subtitle_right) != 0) {
        fputs("</svg>\n", out);
        return -1;
    }
    fprintf(out, "<text class=\"subtitle\" x=\"480\" y=\"26\" text-anchor=\"middle\">compare delta=%+lldms</text>\n", delta);
    if (input->animated) {
        fprintf(out, "<g id=\"compare-anim-controls\" class=\"anim-control-wrap\" data-anim-action=\"toggle\" transform=\"translate(432 34)\">");
        fprintf(out, "<rect class=\"anim-button\" x=\"0\" y=\"0\" width=\"96\" height=\"28\" rx=\"9\"/>");
        fprintf(out, "<text class=\"anim-button-text\" x=\"22\" y=\"18\">stop</text>");
        fprintf(out, "<title>toggle animated compare start/stop</title>");
        fprintf(out, "</g>\n");
    }
    trace_render_emit_interaction_script(out, 0, 0, 1, input->animated, input->animation_loop);
    fputs("</svg>\n", out);
    return 0;
}
