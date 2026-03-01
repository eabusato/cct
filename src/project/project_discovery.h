#ifndef CCT_PROJECT_DISCOVERY_H
#define CCT_PROJECT_DISCOVERY_H

#include <stdbool.h>
#include <stddef.h>

#define CCT_PROJECT_PATH_MAX 4096

typedef struct {
    char root_path[CCT_PROJECT_PATH_MAX];
    char entry_path[CCT_PROJECT_PATH_MAX];
    char out_dir[CCT_PROJECT_PATH_MAX];
    char project_name[128];
    bool has_manifest;
} cct_project_layout_t;

bool cct_project_discover(
    const char *cwd,
    const char *project_override,
    const char *entry_override,
    cct_project_layout_t *out_layout,
    char *error_message,
    size_t error_message_size
);

bool cct_project_join_path(
    const char *left,
    const char *right,
    char *out,
    size_t out_size
);

bool cct_project_path_exists(const char *path);
bool cct_project_path_is_dir(const char *path);

#endif
