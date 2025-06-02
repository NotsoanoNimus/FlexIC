//
// Created by puhlz on 5/28/25.
//

#ifndef FLEX_IC_OPTS_H
#define FLEX_IC_OPTS_H



/* Virtual CAN interface name. */
#define IC_OPT_CAN_IF_NAME "vcan0"

/*
 * Enables O(1) [fast] lookup of message IDs, but requires MUCH higher RAM consumption.
 *  This option should be set to 0 for systems with reduced RAM where there is a wide
 *  spread of CAN message ID values. When message ID values are highly localized, the
 *  memory consumption difference is negligible.
 */
#define IC_OPT_ID_MAPPING 1

/*
 * Define background gradient colors.
 *  If the background is static (i.e., a non-gradient), use the top-left color value.
 */
#define IC_OPT_BG_STATIC 0
#define IC_OPT_BG_TOP_LEFT_RGBA     { 0xDD, 0x32, 0x11, 0xFF }
#define IC_OPT_BG_BOTTOM_LEFT_RGBA  { 0xFF, 0xAE, 0xCC, 0xFF }
#define IC_OPT_BG_TOP_RIGHT_RGBA    { 0x11, 0x44, 0x82, 0xFF }
#define IC_OPT_BG_BOTTOM_RIGHT_RGBA { 0x98, 0xAA, 0xED, 0xFF }

/* The FPS limit to use when rendering the IC. */
#define IC_OPT_FPS_LIMIT 60

/* Whether to use full-screen mode when rendering (note: ignores SCREEN_WIDTH/HEIGHT). */
#define IC_OPT_FULL_SCREEN 0

/* The title of the GUI window when displayed in a desktop environment. */
#define IC_OPT_GUI_TITLE "Flex-IC (MR DEV)"
/* GUI screen width */
#define IC_OPT_SCREEN_WIDTH 800
/* GUI screen height */
#define IC_OPT_SCREEN_HEIGHT 400

/* If set, disables render-time logging, even when IC_DEBUG is on. */
#define IC_OPT_DISABLE_RENDER_TIME      1

/* If set, disables received CAN message logging, even when IC_DEBUG is on. */
#define IC_OPT_DISABLE_CAN_DETAILS      1


/*
 * Whether to enable support for CAN FD or Extended (64-byte) data packets.
 *  Note that CAN buses which aren't sending frames with data over 8 bytes in
 *  length will be WASTING RAM if this option is enabled.
 */
#define IC_OPT_CAN_FD_EXTENDED      0

/*
 * No standard library. You should use this if the platform you're running on isn't running
 *  a basic Linux version. BEWARE: you will have to implement all STDLIB calls yourself and
 *  link them to the resulting executable!
 */
#define IC_OPT_NOSTDLIB         0

/*
 * Define whether custom hooks are provided for each given message type.
 *  Hook symbols follow the format: '_msg_hook_{name}' where '{name}' is the DBC Message Name.
 *  Hooks MUST conform to the _func__update_vehicle_data prototype in "flex_ic.h"
 */
#define IC_OPT_CUSTOM_HOOK_RPM  0



#endif   /* FLEX_IC_OPTS_H */
