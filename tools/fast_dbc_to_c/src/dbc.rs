use std::fs::File;
use std::io;
use std::io::prelude::*;


pub fn load_dbc(from_file: &String) -> io::Result<can_dbc::DBC>
{
    let mut conf = File::open(from_file)?;
    let mut buff = Vec::new();

    conf.read_to_end(&mut buff)?;

    let dbc = can_dbc::DBC::from_slice(&buff)
        .expect("Failed to parse input vehicle DBC file.");

    Ok(dbc.clone())
}
