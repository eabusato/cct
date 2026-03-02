/*
 * CCT — Clavicula Turing
 * Runtime Math Library
 *
 * FASE 13A: Advanced math functions backed by C <math.h>
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#include "runtime_math.h"
#include <math.h>

/* Power and Roots */

double cct_sqrt(double x) {
    return sqrt(x);
}

double cct_cbrt(double x) {
    return cbrt(x);
}

double cct_pow(double base, double exponent) {
    return pow(base, exponent);
}

double cct_hypot(double x, double y) {
    return hypot(x, y);
}

/* Trigonometry */

double cct_sin(double radians) {
    return sin(radians);
}

double cct_cos(double radians) {
    return cos(radians);
}

double cct_tan(double radians) {
    return tan(radians);
}

double cct_asin(double x) {
    return asin(x);
}

double cct_acos(double x) {
    return acos(x);
}

double cct_atan(double x) {
    return atan(x);
}

double cct_atan2(double y, double x) {
    return atan2(y, x);
}

/* Angle Conversion */

double cct_deg_to_rad(double degrees) {
    return degrees * (3.141592653589793 / 180.0);
}

double cct_rad_to_deg(double radians) {
    return radians * (180.0 / 3.141592653589793);
}

/* Exponentials and Logarithms */

double cct_exp(double x) {
    return exp(x);
}

double cct_log(double x) {
    return log(x);
}

double cct_log10(double x) {
    return log10(x);
}

double cct_log2(double x) {
    return log2(x);
}
