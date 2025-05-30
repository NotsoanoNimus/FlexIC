mod dbc;
mod generator;

use std::env;


fn usage(app_name: &String)
{
    println!(
r#"USAGE:  {} {{dbc-file}} {{out-dir}}
    Quickly creates FlexIC code in the output directory from the input DBC file.

OPTIONS:
        dbc-file:   The vehicle CAN signals database to parse, in DBC format.
        out-dir:    The folder in which generated vehicle-specific code should be placed.

"#,
        app_name
    );

    std::process::exit(1);
}


fn main()
{
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        eprintln!("Invalid program arguments.");
        usage(&args[0]);
    }

    println!("Loading DBC...");
}
