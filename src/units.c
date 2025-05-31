//
// Created by puhlz on 5/28/25.
//

#include "units.h"



/* Temperatures. */
static inline void f_to_c(float *f) { *f = (*f - 32.0f) / 1.8f; }
static inline void c_to_f(float *c) { *c = (*c * 1.8f) + 32.0f; }
static inline void c_to_k(float *c) { *c = *c + 273.15f; }
static inline void k_to_c(float *k) { *k = *k - 273.15f; }
static inline void f_to_k(float *f) { f_to_c(f); c_to_k(f); }
static inline void k_to_f(float *k) { k_to_c(k); c_to_f(k); }


void
convert_unit(
    float *mutable_input,
    unit_type_t in_type,
    unit_type_t out_type
) {
    switch (in_type) {
        case UnitRaw: return;   /* raw values are never mutated, since input unit types are undefined */
        case UnitFahrenheit: {
            switch (out_type) {
                case UnitCelsius: f_to_c(mutable_input); break;
                case UnitKelvin: f_to_k(mutable_input); break;
                default: break;
            }
        }
        case UnitCelsius: {
            switch (out_type) {
                case UnitFahrenheit: c_to_f(mutable_input); break;
                case UnitKelvin: c_to_k(mutable_input); break;
                default: break;
            }
        }
        case UnitKelvin: {
            switch (out_type) {
                case UnitCelsius: k_to_c(mutable_input); break;
                case UnitFahrenheit: k_to_f(mutable_input); break;
                default: break;
            }
        }
        default: break;
    }
}
