//
// Created by puhlz on 5/28/25.
//

#ifndef UNIT_CONVERSIONS_H
#define UNIT_CONVERSIONS_H

#include "unit_types.h"



inline void _f_to_c(float *f) { *f = (*f - 32.0f) / 1.8f; }
inline void _c_to_f(float *c) { *c = (*c * 1.8f) + 32.0f; }
inline void _c_to_k(float *c) { *c = *c + 273.15f; }
inline void _k_to_c(float *k) { *k = *k - 273.15f; }
inline void _f_to_k(float *f) { _f_to_c(f); _c_to_k(f); }
inline void _k_to_f(float *k) { _k_to_c(k); _c_to_f(k); }

inline void convert_temperature(float *mutable_input, const unit_type_t in_type, const unit_type_t out_type)
{
    switch (in_type) {
        case TEMP_FAHRENHEIT: {
            switch (out_type) {
                case TEMP_CELSIUS: _f_to_c(mutable_input); break;
                case TEMP_KELVIN: _f_to_k(mutable_input); break;
                default: break;
            }
        }
        case TEMP_CELSIUS: {
            switch (out_type) {
                case TEMP_FAHRENHEIT: _c_to_f(mutable_input); break;
                case TEMP_KELVIN: _c_to_k(mutable_input); break;
                default: break;
            }
        }
        case TEMP_KELVIN: {
                switch (out_type) {
                case TEMP_CELSIUS: _k_to_c(mutable_input); break;
                case TEMP_FAHRENHEIT: _k_to_f(mutable_input); break;
                default: break;
            }
        }
        default: break;
    }
}



#endif   /* UNIT_CONVERSIONS_H */
