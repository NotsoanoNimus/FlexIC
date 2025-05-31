use std::fs::File;
use std::io::prelude::*;

use can_dbc::{DBC, Signal};

pub trait HasSignals {
    fn signals(&self) -> Vec<Signal>;
}

impl HasSignals for DBC
{
    fn signals(&self) -> Vec<Signal>
    {
        self.messages()
            .iter()
            .flat_map(|msg| msg.signals().iter())
            .cloned()
            .collect()
    }
}


pub fn load_dbc(from_file: &String) -> Result<can_dbc::DBC, Box<dyn std::error::Error>>
{
    let mut conf = File::open(from_file)?;
    let mut buff = Vec::new();

    conf.read_to_end(&mut buff)?;

    let Ok(dbc) = DBC::from_slice(&buff) else {
        return Err(Box::from("Invalid DBC input data."));
    };

    Ok(dbc.clone())
}
