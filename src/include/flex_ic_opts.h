//
// Created by puhlz on 5/28/25.
//

#ifndef FLEX_IC_OPTS_H
#define FLEX_IC_OPTS_H



/*
 * Enables O(1) [fast] lookup of message IDs, but requires MUCH higher RAM consumption.
 *  This option should be set to 0 for systems with reduced RAM where there is a wide
 *  spread of CAN message ID values. When message ID values are highly localized, the
 *  memory consumption difference is negligible.
 */
#define IC_OPT_ID_MAPPING 1

/*
 * If this option is set to 1, a static renderer configuration is bundled with the
 *  compiled application. This removes the file-system dependency of the application
 *  altogether and allows the entire application to be self-contained. This works by
 *  embedding the configuration in a known variable name (WIDGET_CONFIGURATION) in the
 *  `renderer.c` module. When this is NOT set, WIDGET_CONFIGURATION is sourced from a
 *  well-known path on the local filesystem.
 */
#define IC_OPT_STATIC_CONFIG 1

/*
 * Define background gradient colors.
 *  If the background is static (a non-gradient), use the top-left color value.
 */
#define IC_OPT_BG_STATIC 0
#define IC_OPT_BG_TOP_LEFT_RGBA     { 0xDD, 0x32, 0x11, 0xFF }
#define IC_OPT_BG_BOTTOM_LEFT_RGBA  { 0xFF, 0xAE, 0xCC, 0xFF }
#define IC_OPT_BG_TOP_RIGHT_RGBA    { 0x11, 0x44, 0x82, 0xFF }
#define IC_OPT_BG_BOTTOM_RIGHT_RGBA { 0x98, 0xAA, 0xED, 0xFF }

/* Where to find the dynamic widget configuration when it's not static. */
#define IC_OPT_CONFIG_PATH "/etc/flex_ic_widgets.config.csv"

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
