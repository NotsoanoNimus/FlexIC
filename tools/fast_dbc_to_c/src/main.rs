mod dbc;
mod generator;

use std::{env, process};
/* I <3 subletting dirty work. */
use yes_or_no::yes_or_no;


fn usage(app_name: &String)
{
    println!(
r#"USAGE:  {} {{dbc-file}} {{out-dir}} [non_interactive]
  Quickly creates FlexIC code in the output directory from the input DBC file.

OPTIONS:
  dbc-file:           The vehicle CAN signals database to parse, in DBC format.
  out-dir:            The folder in which generated vehicle-specific code should be placed.
  non_interactive:    If this param is non-null, the program will not confirm overwrites.

"#,
        app_name
    );

    process::exit(1);
}


fn check_input_files(src_dbc: &str, out_dir: &str, non_interactive: &bool) -> ()
{
    let make_err = |name: &str| format!(
        "Unexpected error while checking {}. You should confirm your filesystem permissions.", name
    ).clone();

    let src_dbc_exists: bool = std::fs::exists(src_dbc).expect(&make_err("src_dbc"));
    let out_dir_exists: bool = std::fs::exists(out_dir).expect(&make_err("out_dir"));

    if !src_dbc_exists {
        eprintln!("Missing src_dbc '{}'.", src_dbc);
        process::exit(1);
    }

    if !out_dir_exists {
        if *non_interactive {
            std::fs::create_dir_all(out_dir)
                .expect(&format!("Could not create output directory '{}'", out_dir));
        } else {
            println!("=> out_dir doesn't exist. Entering prompt...");

            if yes_or_no(&format!("The output directory '{}' does not exist.\r\n\r\nCreate it now?", out_dir), true) {
                std::fs::create_dir_all(out_dir)
                    .expect(&format!("Could not create output directory '{}'", out_dir));

                println!("===> Done!");
            } else {
                println!("===> Understood. Terminating.");
                process::exit(1);
            }
        }
    }

    if
        std::fs::exists(&format!("{}/vehicle.c", out_dir)).expect(&make_err("vehicle.c"))
        || std::fs::exists(&format!("{}/vehicle.c", out_dir)).expect(&make_err("vehicle.h"))
    {
        if *non_interactive {
            println!("=> Overwriting existing generated vehicle files.");
        } else {
            println!("=> One or more generated vehicle files already exist in out_dir. Entering prompt...");

            if yes_or_no("The output directory already contains one or more generated files.\r\n\r\nOverwrite them?", false) {
                println!("===> Understood. Terminating.");
                process::exit(1);
            }
        }
    }
}


fn main()
{
    let args: Vec<String> = env::args().collect();

    if args.len() < 3 {
        eprintln!("Invalid program arguments.");
        usage(&args[0]);
    }

    let non_interactive: bool = args.len() > 3;
    let src_dbc: &String = &args[1];
    let out_dir: &String = &args[2];

    check_input_files(src_dbc, out_dir, &non_interactive);

    println!("=> Loading DBC...");
    let Ok(dbc) = dbc::load_dbc(src_dbc) else {
        eprintln!("Failed to parse input DBC data. Please make sure this is a valid CAN bus DBC file.");
        process::exit(1);
    };

    println!("=> Generating C code...");
    match generator::generate_from_dbc(&out_dir, &dbc) {
        Err(err) => {
            eprintln!("Failed to generate code from DBC: {}", err);
            process::exit(1);
        },
        _ => {},
    }

    println!("=> All done!\r\n  Check '{}/vehicle.c' for a list of your vehicle's signals to\r\n  use in your FlexIC widgets configuration!\r\n", out_dir);
}
