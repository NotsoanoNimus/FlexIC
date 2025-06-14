use std::io::{Error, Write};
use std::fs::File;
use std::process::Command;

use can_dbc::*;
use crate::dbc::HasSignals;
use crate::units::InferredUnitType;

const VEHICLE_C: &str = "vehicle.c";
const VEHICLE_H: &str = "vehicle.h";
const DBC_RAW: &str = "parsed_dbc.raw";

const AUTO_GENERATED_STAMP: &str =
r#"/*
    ========================================================================================
     THIS CODE HAS BEEN AUTO-GENERATED BY THE FAST_DBC_TO_C RUST TOOL.

        *~*~*~* DO NOT MODIFY THIS UNLESS YOU KNOW PRECISELY WHAT YOU ARE DOING. *~*~*~*


    @#@#@ PROJECT LICENSE @#@#@

    MIT License

    Copyright (c) 2025 Zack Puhl <github@xmit.xyz>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    ========================================================================================
*/

"#;


/* Filters signal and message names when generating name string and other references. */
fn name_filter(c: char) -> char {
    if c.is_alphanumeric() || c == '_' {
        c
    } else {
        '_'
    }
}


pub fn generate_from_dbc(into_dir: &str, dbc: &DBC) -> Result<(), Error>
{
    dbg!(&dbc);

    // Write the debug dump of the parsed DBC file to a file adjacent to other generated artifacts.
    let mut raw_file = File::create(&format!("{}/{}", into_dir, DBC_RAW))?;
    raw_file.write_all(&format!("{:#?}", dbc).as_bytes())?;

    // Let the magic begin...
    let mut src_file = File::create(&format!("{}/{}", into_dir, VEHICLE_C))?;
    let mut hdr_file = File::create(&format!("{}/{}", into_dir, VEHICLE_H))?;

    let ts_output = Command::new("date").arg("+%s").output()?.stdout;
    let gh_output = Command::new("git").arg("rev-parse").arg("HEAD").output()?.stdout;
    let timestamp = String::from_utf8_lossy(&ts_output);
    let git_hash = String::from_utf8_lossy(&gh_output);
    src_file.write_all(AUTO_GENERATED_STAMP.as_bytes())?;
    src_file.write_all(format!("// Generation timestamp: {}\n// Git Hash: {}\n\n\n",
                               timestamp.trim(), git_hash.trim()).as_bytes())?;
    hdr_file.write_all(AUTO_GENERATED_STAMP.as_bytes())?;
    hdr_file.write_all(format!("// Generation timestamp: {}\n// Git Hash: {}\n\n\n",
                               timestamp.trim(), git_hash.trim()).as_bytes())?;

    generate_header(&mut hdr_file, &dbc)?;
    generate_source(&mut src_file, &dbc)?;

    Ok(())
}


fn extract_value_from_comments(dbc: &DBC, token: &str) -> String
{
    match dbc.comments().iter().find_map(
        |comment| {
            if let Comment::Message { message_id: msg_id, comment: msg_comment } = comment {
                if !msg_comment.starts_with(&format!("IC_{}:", token)) { return None; }

                if let MessageId::Standard(message_id) = msg_id {
                    if *message_id == 0 {
                        return Some(msg_comment);
                    }
                } else if let MessageId::Extended(message_id) = msg_id {
                    if *message_id == 0 {
                        return Some(msg_comment);
                    }
                }
            }

            None
        })
    {
        Some(value) => value.replace(&format!("IC_{}:", token), ""),
        _ => String::from("UNKNOWN")
    }.clone()
}


fn gen_src_get_property(dbc: &DBC, target_file: &mut File, token: &str) -> Result<(), Error>
{
    let ephemeral_value = extract_value_from_comments(dbc, token);
    target_file.write_all(format!("const char *IC_{} = \"{}\";\n", token, ephemeral_value).as_bytes())?;

    Ok(())
}


fn generate_header(hdr_file: &mut File, dbc: &DBC) -> Result<(), Error>
{
    // The header is fairly simple: wrap everything with your typical header-guard, define
    //  some well-known prototypes, throw in some consts, and close up.
    hdr_file.write_all("#ifndef GEN_IC_VEHICLE_H\n#define GEN_IC_VEHICLE_H\n\n#include \"units.h\"\n\n".as_bytes())?;

    hdr_file.write_all("void init_vehicle_dbc_data(void);\n\n".as_bytes())?;
    hdr_file.write_all(
        format!(
            "#define DBC_SIGNALS_LEN {}\n#define DBC_MESSAGES_LEN {}\n\n",
            dbc.signals().len(), dbc.messages().len()
        ).as_bytes()
    )?;

    hdr_file.write_all("\n\n\n#endif   /* GEN_IC_VEHICLE_H */\n".as_bytes())?;
    Ok(())
}


fn generate_source(src_file: &mut File, dbc: &DBC) -> Result<(), Error>
{
    // Can't forget to include the generated header file.
    src_file.write_all("#include \"vehicle.h\"\n#include \"flex_ic.h\"\n\n#include <pthread.h>\n\n".as_bytes())?;

    // First thing's first: look for special comments that give the application special info.
    //  See the list of 'extern' properties in 'flex_ic.h' to link the two items together.
    for item in vec!(
        "dbc_compiled_by", "name", "short_name", "version",
        "owner_name", "owner_phone", "owner_email", "extra_info"
    ).iter() {
        gen_src_get_property(dbc, src_file, &format!("VEHICLE_{}", item.to_uppercase()))?;
    }

    let struct_bodies = gen_src_dbc_structs(&dbc)?;

    src_file.write_all(
        &format!(
            r#"

const dbc_message_t messages[DBC_MESSAGES_LEN];


dbc_signal_t signals[DBC_SIGNALS_LEN] =
{{
{}
}};

const dbc_message_t messages[DBC_MESSAGES_LEN] =
{{
{}
}};


void init_vehicle_dbc_data()
{{
{}
}}


dbc_t DBC = (dbc_t)
{{
    .messages = (const dbc_message_t *)&messages,
    .signals = (dbc_signal_t *)&signals,
}};

"#,
            struct_bodies.0,
            struct_bodies.1,
            gen_src_func_init_vehicle_dbc_data(&dbc)?
        ).as_bytes()
    )?;

    Ok(())
}


fn gen_src_dbc_structs(dbc: &DBC) -> Result<(String, String), Error>
{
    let mut signals_struct_body = String::new();
    let mut messages_struct_body = String::new();
    let mut msg_index = 0;
    let mut signal_freeze = 0;
    let mut signal_at = 0;

    dbc.messages()
        .iter().for_each(|message| {
            let parent_msg_name: String = message.message_name().chars().map(name_filter).collect();

            signal_freeze = signal_at;
            signal_at += message.signals().len();

            signals_struct_body.push_str(
                message.signals()
                    .iter().map(|signal| {
                        // Filter signal names too.
                        let signal_name: String = signal.name().chars().map(name_filter).collect();

                        let multiplex_type: &str =
                            match signal.multiplexer_indicator() {
                                MultiplexIndicator::MultiplexorAndMultiplexedSignal(_) => "MultiplexorAndMultiplexedSignal",
                                MultiplexIndicator::MultiplexedSignal(_) => "MultiplexedSignal",
                                MultiplexIndicator::Multiplexor => "Multiplexor",
                                MultiplexIndicator::Plain => "Plain",
                            };

                        let multiplexor =
                            match signal.multiplexer_indicator() {
                                MultiplexIndicator::MultiplexedSignal(i) => i,
                                MultiplexIndicator::MultiplexorAndMultiplexedSignal(i) => i,
                                _ => &0,
                            };

                        /* Note: defaults to RAW when the type can't be inferred. */
                        let parsed_unit_type =
                            str::parse::<InferredUnitType>(signal.unit()).unwrap_or_else(|e| {
                                eprintln!("{}", e);
                                InferredUnitType::Raw
                            });

                        String::from(
                            &format!(r#"
    {{
        .name = "{0}",
        .parent_message = (dbc_message_t *)&messages[{msg_index}],
        .widget_instances = NULL,
        .num_widget_instances = 0,
        .real_time_data = {{ false, 0.0f, PTHREAD_MUTEX_INITIALIZER }},
        .start_bit = {1},
        .signal_size = {2},
        .is_little_endian = {3},
        .is_unsigned = {4},
        .multiplex_type = {5},
        .multiplexor = {6},
        .factor = {7},
        .offset = {8},
        .minimum_value = {9},
        .maximum_value = {10},
        .unit_text = "{11}",
        .parsed_unit_type = Unit{12:?},
    }},"#,
                                format!("{}_{}", parent_msg_name, signal_name),
                                signal.start_bit,
                                signal.signal_size,
                                matches!(signal.byte_order(), ByteOrder::LittleEndian {}),
                                matches!(signal.value_type(), ValueType::Unsigned {}),
                                multiplex_type,
                                multiplexor,
                                signal.factor,
                                signal.offset,
                                signal.min,
                                signal.max,
                                signal.unit(),
                                parsed_unit_type
                            )
                        )
                    })
                    .collect::<Vec<String>>()
                    .join("\n")
                    .as_str()
            );

            let msg_id: u32 =
                match message.message_id() {
                    MessageId::Standard(i) => *i as u32,
                    MessageId::Extended(i) => *i,
                };

            let mut signal_refs = String::new();
            for x in signal_freeze..signal_at {
                signal_refs.push_str(format!(" &signals[{x}],").as_str());
            }

            messages_struct_body.push_str(
                format!(r#"
    {{
        .id = 0x{0:x},
        .name = "{1}",
        .expected_length = {2},
        .signals = (dbc_signal_t *[]){{{3} }},
        .num_signals = {4}
    }},"#,
                    msg_id,
                    message.message_name().chars().map(name_filter).collect::<String>(),
                    message.message_size(),
                    signal_refs,
                    signal_at - signal_freeze,
                ).as_str()
            );

            msg_index += 1;
        });

    Ok((signals_struct_body, messages_struct_body))
}


fn gen_src_func_init_vehicle_dbc_data(dbc: &DBC) -> Result<String, Error>
{
//     let mut init_func_body = String::new();
//
//     for (index, signal) in dbc.signals().iter().enumerate() {
//         init_func_body.push_str(
//             &format!(
// r#"    /* SS: {0:04} bits */ signals[0x{1:04X}].real_time_data.value = (volatile double *)calloc(1, 8);"#,
//         //signals[0x{1:04}].real_time_data.value_width = {2};
//                 signal.signal_size,
//                 index,
//                 // match signal.signal_size {
//                 //     0..=8 => 1,
//                 //     9..=16 => 2,
//                 //     17..=32 => 4,
//                 //     33..=64 => 8,
//                 //     _ => panic!("Invalid signal size {}: signals cannot be larger than 64 bits.", signal.signal_size),
//                 // }
//             )
//         );
//     }

    Ok(String::from("    /* PLACEHOLDER - Hi! */"))
    // Ok(init_func_body)
}
