/*
 * CCT — Clavicula Turing
 * Common Types and Definitions
 *
 * FASE 0: Types base definitions
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_COMMON_TYPES_H
#define CCT_COMMON_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Project version
 */
#define CCT_VERSION_MAJOR 0
#define CCT_VERSION_MINOR 12
#define CCT_VERSION_PATCH 0
#define CCT_VERSION_SUFFIX ""

#define CCT_VERSION_STRING "0.12.0"

/*
 * Common type aliases
 * Keep these minimal and only add when truly necessary
 */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

typedef float    f32;
typedef double   f64;

/*
 * Boolean types (using stdbool.h)
 */
/* Already defined by stdbool.h: bool, true, false */

#endif /* CCT_COMMON_TYPES_H */
