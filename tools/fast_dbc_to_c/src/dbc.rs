use std::fs::File;
use std::io::prelude::*;


pub fn load_dbc(from_file: &String) -> Result<can_dbc::DBC, Box<dyn std::error::Error>>
{
    let mut conf = File::open(from_file)?;
    let mut buff = Vec::new();

    conf.read_to_end(&mut buff)?;

    let Ok(dbc) = can_dbc::DBC::from_slice(&buff) else {
        return Err(Box::from("Invalid DBC input data."));
    };

    Ok(dbc.clone())
}
