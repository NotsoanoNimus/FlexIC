//
// Created by puhlz on 5/28/25.
//

#ifndef UNIT_TYPES_H
#define UNIT_TYPES_H



typedef
enum {
    UnitNone = 0,

    UnitKelvin,
    UnitCelsius,
    UnitFahrenheit,

    UnitRadians,
    UnitDegrees,

    UnitMilesPerHour,
    UnitFeetPerSecond,
    UnitKilometersPerHour,
    UnitMetersPerSecond,

    UnitRevolutionsPerMinute,

    UnitBytes,
    UnitKilobytes,
    UnitKibibytes,
    UnitMegabytes,
    UnitMebibytes,
    UnitGigabytes,
    UnitGibibytes,
    UnitTerabytes,
    UnitTebibytes,
    UnitPetabytes,
    UnitPebibytes,

    UnitRaw,
} unit_type_t;


void
convert_unit(
    float *mutable_input,
    unit_type_t in_type,
    unit_type_t out_type
);



#endif /* UNIT_TYPES_H */
