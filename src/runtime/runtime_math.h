/*
 * CCT — Clavicula Turing
 * Runtime Math Library - Header
 *
 * FASE 13A: Advanced math functions backed by C <math.h>
 *
 * Copyright (c) Erick Andrade Busato. Todos os direitos reservados.
 */

#ifndef CCT_RUNTIME_MATH_H
#define CCT_RUNTIME_MATH_H

/* Power and Roots */
double cct_sqrt(double x);
double cct_cbrt(double x);
double cct_pow(double base, double exponent);
double cct_hypot(double x, double y);

/* Trigonometry */
double cct_sin(double radians);
double cct_cos(double radians);
double cct_tan(double radians);
double cct_asin(double x);
double cct_acos(double x);
double cct_atan(double x);
double cct_atan2(double y, double x);

/* Angle Conversion */
double cct_deg_to_rad(double degrees);
double cct_rad_to_deg(double radians);

/* Exponentials and Logarithms */
double cct_exp(double x);
double cct_log(double x);
double cct_log10(double x);
double cct_log2(double x);

#endif /* CCT_RUNTIME_MATH_H */
