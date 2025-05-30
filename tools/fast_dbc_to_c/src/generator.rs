use std::io::{Error, Write};
use std::fs::File;

use can_dbc::DBC;


const VEHICLE_C: &str = "vehicle.c";
const VEHICLE_H: &str = "vehicle.h";
const DBC_RAW: &str = "parsed_dbc.raw";


pub fn generate_from_dbc(into_dir: &str, dbc: &DBC) -> Result<(), Error>
{
    dbg!(&dbc);

    // Write the debug dump of the parsed DBC file to a file adjacent to other generated artifacts.
    let mut raw_file = File::create(&format!("{}/{}", into_dir, DBC_RAW))?;
    raw_file.write_all(&format!("{:#?}", dbc).as_bytes())?;

    Ok(())
}
