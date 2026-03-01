#include "project.h"

#include "project_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void pj_print_usage(void) {
    printf("Project Commands:\n");
    printf("  cct build [--release] [--project DIR] [--entry FILE.cct] [--out PATH] [--lint] [--fmt-check]\n");
    printf("  cct run [--release] [--project DIR] [--entry FILE.cct] [--out PATH] [-- ARGS...]\n");
    printf("  cct test [PATTERN] [--project DIR] [--strict-lint] [--fmt-check]\n");
    printf("  cct bench [PATTERN] [--project DIR] [--iterations N] [--release]\n");
    printf("  cct clean [--project DIR] [--all]\n");
}

bool cct_project_is_command(const char *arg) {
    return arg && (
        strcmp(arg, "build") == 0 ||
        strcmp(arg, "run") == 0 ||
        strcmp(arg, "test") == 0 ||
        strcmp(arg, "bench") == 0 ||
        strcmp(arg, "clean") == 0
    );
}

static bool pj_need_value(int i, int argc, const char *opt) {
    if (i + 1 < argc) return true;
    fprintf(stderr, "cct: error: %s requires a value\n", opt);
    return false;
}

static int pj_parse_common_flag(const char *arg,
                                int *i,
                                int argc,
                                char **argv,
                                cct_project_options_t *opts,
                                bool allow_out,
                                bool allow_entry,
                                bool allow_project) {
    if (allow_project && strcmp(arg, "--project") == 0) {
        if (!pj_need_value(*i, argc, "--project")) return -1;
        opts->project_override = argv[++(*i)];
        return 1;
    }

    if (allow_entry && strcmp(arg, "--entry") == 0) {
        if (!pj_need_value(*i, argc, "--entry")) return -1;
        opts->entry_override = argv[++(*i)];
        return 1;
    }

    if (allow_out && strcmp(arg, "--out") == 0) {
        if (!pj_need_value(*i, argc, "--out")) return -1;
        opts->out_override = argv[++(*i)];
        return 1;
    }

    return 0;
}

static int pj_dispatch_build(int argc, char **argv, const char *self_path) {
    cct_project_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.profile = CCT_PROJECT_PROFILE_DEBUG;
    opts.iterations = 5;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--release") == 0) {
            opts.profile = CCT_PROJECT_PROFILE_RELEASE;
            continue;
        }
        if (strcmp(arg, "--lint") == 0) {
            opts.run_lint = true;
            continue;
        }
        if (strcmp(arg, "--fmt-check") == 0) {
            opts.fmt_check = true;
            continue;
        }

        int parsed = pj_parse_common_flag(arg, &i, argc, argv, &opts, true, true, true);
        if (parsed == -1) return 1;
        if (parsed == 1) continue;

        fprintf(stderr, "cct: error: unknown build option: %s\n", arg);
        return 1;
    }

    return cct_project_cmd_build(self_path, &opts, NULL, 0);
}

static int pj_dispatch_run(int argc, char **argv, const char *self_path) {
    cct_project_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.profile = CCT_PROJECT_PROFILE_DEBUG;
    opts.iterations = 5;

    int args_start = argc;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--") == 0) {
            args_start = i + 1;
            break;
        }
        if (strcmp(arg, "--release") == 0) {
            opts.profile = CCT_PROJECT_PROFILE_RELEASE;
            continue;
        }

        int parsed = pj_parse_common_flag(arg, &i, argc, argv, &opts, true, true, true);
        if (parsed == -1) return 1;
        if (parsed == 1) continue;

        fprintf(stderr, "cct: error: unknown run option: %s\n", arg);
        return 1;
    }

    if (args_start < argc) {
        opts.passthrough_argc = argc - args_start;
        opts.passthrough_argv = &argv[args_start];
    }

    return cct_project_cmd_run(self_path, &opts);
}

static int pj_dispatch_test(int argc, char **argv, const char *self_path) {
    cct_project_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.profile = CCT_PROJECT_PROFILE_DEBUG;
    opts.iterations = 5;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--strict-lint") == 0) {
            opts.strict_lint = true;
            continue;
        }
        if (strcmp(arg, "--fmt-check") == 0) {
            opts.fmt_check = true;
            continue;
        }

        int parsed = pj_parse_common_flag(arg, &i, argc, argv, &opts, false, false, true);
        if (parsed == -1) return 1;
        if (parsed == 1) continue;

        if (arg[0] != '-' && !opts.pattern) {
            opts.pattern = arg;
            continue;
        }

        fprintf(stderr, "cct: error: unknown test option: %s\n", arg);
        return 1;
    }

    return cct_project_cmd_test(self_path, &opts);
}

static int pj_dispatch_bench(int argc, char **argv, const char *self_path) {
    cct_project_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.profile = CCT_PROJECT_PROFILE_RELEASE;
    opts.iterations = 5;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--release") == 0) {
            opts.profile = CCT_PROJECT_PROFILE_RELEASE;
            continue;
        }
        if (strcmp(arg, "--iterations") == 0) {
            if (!pj_need_value(i, argc, "--iterations")) return 1;
            opts.iterations = atoi(argv[++i]);
            if (opts.iterations <= 0) {
                fprintf(stderr, "cct: error: --iterations must be > 0\n");
                return 1;
            }
            continue;
        }

        int parsed = pj_parse_common_flag(arg, &i, argc, argv, &opts, false, false, true);
        if (parsed == -1) return 1;
        if (parsed == 1) continue;

        if (arg[0] != '-' && !opts.pattern) {
            opts.pattern = arg;
            continue;
        }

        fprintf(stderr, "cct: error: unknown bench option: %s\n", arg);
        return 1;
    }

    return cct_project_cmd_bench(self_path, &opts);
}

static int pj_dispatch_clean(int argc, char **argv) {
    cct_project_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.profile = CCT_PROJECT_PROFILE_DEBUG;
    opts.iterations = 5;

    for (int i = 2; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--all") == 0) {
            opts.clean_all = true;
            continue;
        }

        int parsed = pj_parse_common_flag(arg, &i, argc, argv, &opts, false, false, true);
        if (parsed == -1) return 1;
        if (parsed == 1) continue;

        fprintf(stderr, "cct: error: unknown clean option: %s\n", arg);
        return 1;
    }

    return cct_project_cmd_clean(&opts);
}

int cct_project_dispatch(int argc, char **argv, const char *self_path) {
    if (argc < 2) {
        pj_print_usage();
        return 1;
    }

    const char *cmd = argv[1];
    if (strcmp(cmd, "build") == 0) return pj_dispatch_build(argc, argv, self_path);
    if (strcmp(cmd, "run") == 0) return pj_dispatch_run(argc, argv, self_path);
    if (strcmp(cmd, "test") == 0) return pj_dispatch_test(argc, argv, self_path);
    if (strcmp(cmd, "bench") == 0) return pj_dispatch_bench(argc, argv, self_path);
    if (strcmp(cmd, "clean") == 0) return pj_dispatch_clean(argc, argv);

    pj_print_usage();
    return 1;
}
