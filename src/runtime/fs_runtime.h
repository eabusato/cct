#ifndef CCT_FS_RUNTIME_H
#define CCT_FS_RUNTIME_H

char* cct_rt_fs_read_all(const char *path);
void cct_rt_fs_write_all(const char *path, const char *content);
void cct_rt_fs_append_all(const char *path, const char *content);
long long cct_rt_fs_exists(const char *path);
long long cct_rt_fs_size(const char *path);

#endif
