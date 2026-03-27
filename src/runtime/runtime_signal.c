/*
 * CCT — Clavicula Turing
 * Signal Runtime Helper Emission
 *
 * FASE 34D: signal host runtime bridge
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime.h"

bool cct_runtime_emit_signal_helpers(FILE *out) {
    if (!out) return false;

    fputs("static volatile sig_atomic_t cct_rt_signal_installed = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_last_kind_code = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_sequence = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_shutdown = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_seen_term = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_seen_int = 0;\n", out);
    fputs("static volatile sig_atomic_t cct_rt_signal_seen_hup = 0;\n\n", out);

    fputs("static void cct_rt_signal_handler(int signo) {\n", out);
    fputs("    if (signo == SIGTERM) {\n", out);
    fputs("        cct_rt_signal_last_kind_code = 1;\n", out);
    fputs("        cct_rt_signal_seen_term = 1;\n", out);
    fputs("        cct_rt_signal_shutdown = 1;\n", out);
    fputs("    } else if (signo == SIGINT) {\n", out);
    fputs("        cct_rt_signal_last_kind_code = 2;\n", out);
    fputs("        cct_rt_signal_seen_int = 1;\n", out);
    fputs("        cct_rt_signal_shutdown = 1;\n", out);
    fputs("    } else if (signo == SIGHUP) {\n", out);
    fputs("        cct_rt_signal_last_kind_code = 3;\n", out);
    fputs("        cct_rt_signal_seen_hup = 1;\n", out);
    fputs("    }\n", out);
    fputs("    cct_rt_signal_sequence += 1;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_is_supported(void) {\n", out);
    fputs("    return 1LL;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_install(void) {\n", out);
    fputs("    if (cct_rt_signal_installed) return 1LL;\n", out);
    fputs("    if (signal(SIGTERM, cct_rt_signal_handler) == SIG_ERR) return 0LL;\n", out);
    fputs("    if (signal(SIGINT, cct_rt_signal_handler) == SIG_ERR) return 0LL;\n", out);
    fputs("    if (signal(SIGHUP, cct_rt_signal_handler) == SIG_ERR) return 0LL;\n", out);
    fputs("    cct_rt_signal_installed = 1;\n", out);
    fputs("    return 1LL;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_last_kind(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_last_kind_code;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_last_sequence(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_sequence;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_last_unix_ms(void) {\n", out);
    fputs("    struct timespec ts;\n", out);
    fputs("    if (cct_rt_signal_last_kind_code == 0) return 0LL;\n", out);
    fputs("    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return 0LL;\n", out);
    fputs("    return (long long)ts.tv_sec * 1000LL + (long long)(ts.tv_nsec / 1000000L);\n", out);
    fputs("}\n\n", out);

    fputs("static void cct_rt_signal_clear(void) {\n", out);
    fputs("    cct_rt_signal_last_kind_code = 0;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_check_shutdown(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_shutdown;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_received_sigterm(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_seen_term;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_received_sigint(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_seen_int;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_received_sighup(void) {\n", out);
    fputs("    return (long long)cct_rt_signal_seen_hup;\n", out);
    fputs("}\n\n", out);

    fputs("static long long cct_rt_signal_raise_self(long long code) {\n", out);
    fputs("    int signo = 0;\n", out);
    fputs("    if (code == 1LL) signo = SIGTERM;\n", out);
    fputs("    else if (code == 2LL) signo = SIGINT;\n", out);
    fputs("    else if (code == 3LL) signo = SIGHUP;\n", out);
    fputs("    if (signo == 0) return 0LL;\n", out);
    fputs("    return (raise(signo) == 0) ? 1LL : 0LL;\n", out);
    fputs("}\n\n", out);
    return true;
}
