use std::str::FromStr;

#[derive(Debug)]
pub enum InferredUnitType
{
    Kelvin,
    Celsius,
    Fahrenheit,

    Radians,
    Degrees,

    MilesPerHour,
    FeetPerSecond,
    KilometersPerHour,
    MetersPerSecond,

    RevolutionsPerMinute,

    // TODO: Metric prefixes need to be separated from the unit type.
    Bytes,
    Kilobytes,
    Kibibytes,
    Megabytes,
    Mebibytes,
    Gigabytes,
    Gibibytes,
    Terabytes,
    Tebibytes,
    Petabytes,
    Pebibytes,

    Volts,
    Amperes,
    Resistance,
    Capacity,
    Power,

    Seconds,
    Minutes,
    Hours,
    Days,

    Percentage,

    Raw,
}


impl FromStr for InferredUnitType
{
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err>
    {
        match s.trim().to_lowercase().as_str() {
            "k" | "deg k" | "kelvin" | "kelvins" => Ok(InferredUnitType::Kelvin),
            "c" | "deg c" | "celsius" => Ok(InferredUnitType::Celsius),
            "f" | "deg f" | "fahrenheit" => Ok(InferredUnitType::Fahrenheit),

            "rad" | "rads" | "radians" => Ok(InferredUnitType::Radians),
            "deg" | "degrees" => Ok(InferredUnitType::Degrees),

            "mph" | "miles per hour" | "m/h" => Ok(InferredUnitType::MilesPerHour),
            "fps" | "feet per second" | "f/s" => Ok(InferredUnitType::FeetPerSecond),
            "kph" | "kilometers per hour" | "kmh" | "km/h" => Ok(InferredUnitType::KilometersPerHour),
            "mps" | "meters per second" | "m/s" => Ok(InferredUnitType::MetersPerSecond),

            "rpm" | "r.p.m." => Ok(InferredUnitType::RevolutionsPerMinute),

            "b" | "byte" | "bytes" | "bytes per second" | "b/s" => Ok(InferredUnitType::Bytes),
            "kb" | "kilobyte" | "kilobytes" | "kilobytes per second" | "kb/s" => Ok(InferredUnitType::Kilobytes),
            "kib" | "kibibyte" | "kibibytes" | "kibibytes per second" | "kib/s" => Ok(InferredUnitType::Kibibytes),
            "mb" | "megabyte" | "megabytes" | "megabytes per second" | "mb/s" => Ok(InferredUnitType::Megabytes),
            "mib" | "mebibyte" | "mebibytes" | "mebibytes per second" | "mib/s" => Ok(InferredUnitType::Mebibytes),
            "gb" | "gigabyte" | "gigabytes" | "gigabytes per second" | "gb/s" => Ok(InferredUnitType::Gigabytes),
            "gib" | "gibibyte" | "gibibytes" | "gibibytes per second" | "gib/s" => Ok(InferredUnitType::Gibibytes),
            "tb" | "terabyte" | "terabytes" | "terabytes per second" | "tb/s" => Ok(InferredUnitType::Terabytes),
            "tib" | "tebibyte" | "tebibytes" | "tebibytes per second" | "tib/s" => Ok(InferredUnitType::Tebibytes),
            "pb" | "petabyte" | "petabytes" | "petabytes per second" | "pb/s" => Ok(InferredUnitType::Petabytes),
            "pib" | "pebibyte" | "pebibytes" | "pebibytes per second" | "pib/s" => Ok(InferredUnitType::Pebibytes),

            "v" | "volts" | "voltage" => Ok(InferredUnitType::Volts),
            "a" | "amps" | "amperes" | "current" => Ok(InferredUnitType::Amperes),
            "o" | "ohms" | "resistance" => Ok(InferredUnitType::Resistance),
            "wh" => Ok(InferredUnitType::Capacity),
            "w" | "watts" => Ok(InferredUnitType::Power),

            "s" | "sec" | "seconds" => Ok(InferredUnitType::Seconds),
            "m" | "min" | "minutes" => Ok(InferredUnitType::Minutes),
            "h" | "hr" | "hours" => Ok(InferredUnitType::Hours),
            "days" => Ok(InferredUnitType::Days),

            "%" | "percent" | "percentage" => Ok(InferredUnitType::Percentage),

            "" | "unknown" | "raw" | "?" => Ok(InferredUnitType::Raw),

            _ => Err(format!("Could not infer unit type from '{}'", s)),
        }
    }
}
