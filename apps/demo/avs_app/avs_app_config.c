/*
 * Copyright 2017, Cypress Semiconductor Corporation or a subsidiary of 
 * Cypress Semiconductor Corporation. All Rights Reserved.
 * 
 * This software, associated documentation and materials ("Software"),
 * is owned by Cypress Semiconductor Corporation
 * or one of its subsidiaries ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/** @file
 *
 */

#include "wiced.h"
#include "wiced_log.h"

#include "avs_app_config.h"
#include "platform_audio.h"
#include "command_console.h"

#include "avs_app_dct.h"


/******************************************************
 *                      Macros
 ******************************************************/

#define DIRTY_CHECK(flags,bit)       ((flags & bit) != 0 ? '*' : ' ')

/******************************************************
 *                    Constants
 ******************************************************/

#define APP_DCT_LOG_LEVEL_DIRTY             (1 << 0)
#define APP_DCT_AUDIO_DEVICE_RX_DIRTY       (1 << 1)
#define APP_DCT_AUDIO_DEVICE_TX_DIRTY       (1 << 2)
#define APP_DCT_VOLUME_DIRTY                (1 << 3)
#define APP_DCT_UTC_DIRTY                   (1 << 4)

#define MAC_STR_LEN                         (18)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    CONFIG_CMD_NONE         = 0,
    CONFIG_CMD_HELP,
    CONFIG_CMD_LOG_LEVEL,
    CONFIG_CMD_AUDIO_DEVICE_RX,
    CONFIG_CMD_AUDIO_DEVICE_TX,
    CONFIG_CMD_VOLUME,
    CONFIG_CMD_UTC,
    CONFIG_CMD_SAVE,

    CONFIG_CMD_MAX,
} CONFIG_CMDS_T;

/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef struct cmd_lookup_s
{
    char* cmd;
    uint32_t event;
} cmd_lookup_t;

typedef struct security_lookup_s
{
    char* name;
    wiced_security_t sec_type;
} security_lookup_t;

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

static wiced_result_t avs_app_save_app_dct(avs_app_dct_collection_t* dct_tables);
static void avs_app_print_app_info(avs_app_dct_collection_t* dct_tables);
static void avs_app_print_network_info(avs_app_dct_collection_t* dct_tables);
static void avs_app_print_wifi_info(avs_app_dct_collection_t* dct_tables);

/******************************************************
 *               Variables Definitions
 ******************************************************/

/* dirty flags for determining what to save */
static uint32_t app_dct_dirty = 0;

static cmd_lookup_t config_command_lookup[] = {
    { "help",               CONFIG_CMD_HELP                 },
    { "?",                  CONFIG_CMD_HELP                 },

    { "log_level",          CONFIG_CMD_LOG_LEVEL            },
    { "log",                CONFIG_CMD_LOG_LEVEL            },

    { "volume",             CONFIG_CMD_VOLUME               },
    { "vol",                CONFIG_CMD_VOLUME               },

    { "utc",                CONFIG_CMD_UTC,                 },

    { "audio_device_rx",    CONFIG_CMD_AUDIO_DEVICE_RX      },
    { "ad_rx",              CONFIG_CMD_AUDIO_DEVICE_RX      },
    { "audio_device_tx",    CONFIG_CMD_AUDIO_DEVICE_TX      },
    { "ad_tx",              CONFIG_CMD_AUDIO_DEVICE_TX      },

    { "save",               CONFIG_CMD_SAVE                 },

    { "", CONFIG_CMD_NONE },
};

static void avs_app_config_command_help(void)
{
    wiced_log_printf("Config commands:\n");
    wiced_log_printf("    config                              : output current config\n");
    wiced_log_printf("    config <?|help>                     : show this list\n");
    wiced_log_printf("    config log_level <level>            : set the log level\n");
    wiced_log_printf("    config log <level>\n");
    wiced_log_printf("    config volume <0-100>               : set the playback volume\n");
    wiced_log_printf("    config vol <0-100>\n");
    wiced_log_printf("    config utc <offset in ms>           : local time offset from UTC in milliseconds\n");
    wiced_log_printf("    config audio_device_rx <device_X>   : enter X as in WICED_AUDIO_X, X starts at 0 for all WICED platforms\n");
    wiced_log_printf("    config ad_rx           <device_X>\n");
    wiced_log_printf("    config audio_device_tx <device_X>   : enter X as in WICED_AUDIO_X, X starts at 0 for all WICED platforms\n");
    wiced_log_printf("    config ad_tx           <device_X>\n");
    wiced_log_printf("    config save                         : save data to flash NOTE: Changes not \n");
    wiced_log_printf("                                        :   automatically saved to flash!\n");
}

static security_lookup_t avs_app_security_name_table[] =
{
    { "open",        WICED_SECURITY_OPEN           },
    { "none",        WICED_SECURITY_OPEN           },
    { "wep",         WICED_SECURITY_WEP_PSK        },
    { "shared",      WICED_SECURITY_WEP_SHARED     },
    { "wpa_tkip",    WICED_SECURITY_WPA_TKIP_PSK   },
    { "wpa_aes",     WICED_SECURITY_WPA_AES_PSK    },
    { "wpa_mix",     WICED_SECURITY_WPA_MIXED_PSK  },
    { "wpa_tkipent", WICED_SECURITY_WPA_TKIP_ENT   },
    { "wpa_aesent",  WICED_SECURITY_WPA_AES_ENT    },
    { "wpa_mixent",  WICED_SECURITY_WPA_MIXED_ENT  },
    { "wpa2_aes",    WICED_SECURITY_WPA2_AES_PSK   },
    { "wpa2_tkip",   WICED_SECURITY_WPA2_TKIP_PSK  },
    { "wpa2_mix",    WICED_SECURITY_WPA2_MIXED_PSK },
    { "wpa2_tkipent",WICED_SECURITY_WPA2_TKIP_ENT  },
    { "wpa2_aesent", WICED_SECURITY_WPA2_AES_ENT   },
    { "wpa2_mixent", WICED_SECURITY_WPA2_MIXED_ENT },
    { "ibss",        WICED_SECURITY_IBSS_OPEN      },
    { "wps_open",    WICED_SECURITY_WPS_OPEN       },
    { "wps_none",    WICED_SECURITY_WPS_OPEN       },
    { "wps_aes",     WICED_SECURITY_WPS_SECURE     },
    { "invalid",     WICED_SECURITY_UNKNOWN        },
};


/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t avs_app_save_app_dct(avs_app_dct_collection_t* dct_tables)
{
    return wiced_dct_write((void*)dct_tables->dct_app, DCT_APP_SECTION, 0, sizeof(avs_app_dct_t));
}

static char* avs_app_get_security_type_name(wiced_security_t type)
{
    int table_index;

    for (table_index = 0; table_index < sizeof(avs_app_security_name_table)/sizeof(security_lookup_t); table_index++)
    {
        if (avs_app_security_name_table[table_index].sec_type == type)
        {
            return avs_app_security_name_table[table_index].name;
        }
    }

    return "";
}

static wiced_bool_t avs_app_get_mac_addr_text(wiced_mac_t *mac, char* out, int out_len)
{
    if ((mac == NULL) || (out == NULL) || (out_len < MAC_STR_LEN))
    {
        return WICED_FALSE;
    }

    snprintf(out, out_len, "%02x:%02x:%02x:%02x:%02x:%02x", mac->octet[0], mac->octet[1], mac->octet[2], mac->octet[3], mac->octet[4], mac->octet[5]);

    return WICED_TRUE;
}

void avs_app_set_config(avs_app_dct_collection_t* dct_tables, int argc, char *argv[])
{
    WICED_LOG_LEVEL_T log_save;
    platform_audio_device_id_t device;
    CONFIG_CMDS_T cmd;
    int device_index;
    int log_level;
    int arg;
    int i;

    if (argc < 2)
    {
        avs_app_config_print_info(dct_tables);
        return;
    }

    /*
     * Make sure the notices about setting changes get displayed.
     */

    log_save = wiced_log_get_facility_level(WLF_DEF);
    if (log_save < WICED_LOG_NOTICE)
    {
        wiced_log_set_facility_level(WLF_DEF, WICED_LOG_NOTICE);
    }

    cmd = CONFIG_CMD_NONE;
    for (i = 0; i < (sizeof(config_command_lookup) / sizeof(cmd_lookup_t)); ++i)
    {
        if (strcasecmp(config_command_lookup[i].cmd, argv[1]) == 0)
        {
            cmd = config_command_lookup[i].event;
            break;
        }
    }

    switch (cmd)
    {
        case CONFIG_CMD_HELP:
            avs_app_config_command_help();
            break;

        case CONFIG_CMD_LOG_LEVEL:
            if (argc > 2)
            {
                log_level = atoi(argv[2]);
                /* validity check */
                if (log_level < WICED_LOG_OFF || log_level >= WICED_LOG_MAX)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error: out of range min: %d  max: %d\n",
                                  WICED_LOG_OFF, WICED_LOG_MAX);
                    break;
                }
                if (dct_tables->dct_app->log_level != log_level)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "CONFIG_CMD_LOG_LEVEL: %d -> %d\n", dct_tables->dct_app->log_level, log_level);
                    dct_tables->dct_app->log_level = log_level;
                    app_dct_dirty |= APP_DCT_LOG_LEVEL_DIRTY;
                }
            }
            wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Log level: %d\n", dct_tables->dct_app->log_level);
            break;

        case CONFIG_CMD_VOLUME:
            if (argc > 2)
            {
                arg = atoi(argv[2]);
                /* validity check */
                if (arg < AVS_APP_VOLUME_MIN || arg > AVS_APP_VOLUME_MAX)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error: out of range min: %d  max: %d\n",
                                  AVS_APP_VOLUME_MIN, AVS_APP_VOLUME_MAX);
                    break;
                }
                if (dct_tables->dct_app->volume != arg)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "CONFIG_CMD_VOLUME: %d -> %d\n", dct_tables->dct_app->volume, arg);
                    dct_tables->dct_app->volume = arg;
                    app_dct_dirty |= APP_DCT_VOLUME_DIRTY;
                }
            }
            wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Volume: %d\n", dct_tables->dct_app->volume);
            break;

        case CONFIG_CMD_UTC:
            if (argc > 2)
            {
                arg = atoi(argv[2]);
                /* validity check */
                if (arg < AVS_APP_UTC_OFFSET_MIN || arg > AVS_APP_UTC_OFFSET_MAX)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Error: out of range min: %d  max: %d\n",
                                  AVS_APP_UTC_OFFSET_MIN, AVS_APP_UTC_OFFSET_MAX);
                    break;
                }
                if (dct_tables->dct_app->utc_offset_ms != arg)
                {
                    wiced_log_msg(WLF_DEF, WICED_LOG_DEBUG0, "CONFIG_CMD_UTC: %d -> %d\n", dct_tables->dct_app->utc_offset_ms, arg);
                    dct_tables->dct_app->utc_offset_ms = arg;
                    app_dct_dirty |= APP_DCT_UTC_DIRTY;
                }
            }
            wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "UTC Offset: %d\n", dct_tables->dct_app->utc_offset_ms);
            break;

        case CONFIG_CMD_AUDIO_DEVICE_RX:
            device = dct_tables->dct_app->audio_device_rx;
            if (argc > 2)
            {
                device_index = atoi(argv[2]);
                if (device_index >= 0 && device_index < PLATFORM_AUDIO_NUM_INPUTS)
                {
                    device = platform_audio_input_devices[device_index].device_id;
                    if (device != dct_tables->dct_app->audio_device_rx)
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Audio device RX ID: 0x%x -> 0x%x\n", dct_tables->dct_app->audio_device_rx,
                                      device, platform_audio_input_devices[device_index].device_name);
                        dct_tables->dct_app->audio_device_rx = device;
                        app_dct_dirty |= APP_DCT_AUDIO_DEVICE_RX_DIRTY;
                    }
                }
            }
            platform_audio_print_device_list(dct_tables->dct_app->audio_device_rx, (app_dct_dirty & APP_DCT_AUDIO_DEVICE_RX_DIRTY), AUDIO_DEVICE_ID_NONE, 0, 1);
            break;

        case CONFIG_CMD_AUDIO_DEVICE_TX:
            device = dct_tables->dct_app->audio_device_tx;
            if (argc > 2)
            {
                device_index = atoi(argv[2]);
                if (device_index >= 0 && device_index < PLATFORM_AUDIO_NUM_OUTPUTS)
                {
                    device = platform_audio_output_devices[device_index].device_id;
                    if (device != dct_tables->dct_app->audio_device_tx)
                    {
                        wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Audio device TX ID: 0x%x -> 0x%x %s\n", dct_tables->dct_app->audio_device_tx,
                                      device, platform_audio_output_devices[device_index].device_name);
                        dct_tables->dct_app->audio_device_tx = device;
                        app_dct_dirty |= APP_DCT_AUDIO_DEVICE_TX_DIRTY;
                    }
                }
            }
            platform_audio_print_device_list(AUDIO_DEVICE_ID_NONE, WICED_FALSE, dct_tables->dct_app->audio_device_tx, (app_dct_dirty & APP_DCT_AUDIO_DEVICE_RX_DIRTY), 1);
            break;

        case CONFIG_CMD_SAVE:
            if (app_dct_dirty != 0)
            {
                wiced_log_msg(WLF_DEF, WICED_LOG_NOTICE, "Saving App DCT:\n");
                avs_app_save_app_dct(dct_tables);
                app_dct_dirty = 0;
                avs_app_print_app_info(dct_tables);
            }
            break;

        default:
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unrecognized config command: %s\n", (argv[1][0] != 0) ? argv[1] : "");
            avs_app_config_command_help();
            break;
    }

    /*
     * Restore the original log level.
     */

    wiced_log_set_facility_level(WLF_DEF, log_save);
}

static void avs_app_print_app_info(avs_app_dct_collection_t* dct_tables)
{
    int hours;
    int mins;

    hours = dct_tables->dct_app->utc_offset_ms / 1000 / 60 / 60;
    mins  = (abs(dct_tables->dct_app->utc_offset_ms) / 1000 / 60) % 60;

    wiced_log_printf("  AVS Test App DCT:\n");
    wiced_log_printf("           Log level: %d %c\n", dct_tables->dct_app->log_level,           DIRTY_CHECK(app_dct_dirty, APP_DCT_LOG_LEVEL_DIRTY));
    wiced_log_printf("              Volume: %d %c\n", dct_tables->dct_app->volume,              DIRTY_CHECK(app_dct_dirty, APP_DCT_VOLUME_DIRTY));
    wiced_log_printf("          UTC offset: %d (%02d:%02d) %c\n", dct_tables->dct_app->utc_offset_ms, hours, mins, DIRTY_CHECK(app_dct_dirty, APP_DCT_UTC_DIRTY));
    platform_audio_print_device_list(dct_tables->dct_app->audio_device_rx, (app_dct_dirty & APP_DCT_AUDIO_DEVICE_RX_DIRTY),
                                     dct_tables->dct_app->audio_device_tx, (app_dct_dirty & APP_DCT_AUDIO_DEVICE_TX_DIRTY), 0);
}

static void avs_app_print_network_info(avs_app_dct_collection_t* dct_tables)
{
    wiced_ip_address_t ip_addr;

    wiced_log_printf("  Network DCT:\n");
    wiced_log_printf("           Interface: %s\n",
           (dct_tables->dct_network->interface == (wiced_interface_t)WWD_STA_INTERFACE)      ? "STA" :
           (dct_tables->dct_network->interface == (wiced_interface_t)WWD_AP_INTERFACE)       ? "AP" :
           (dct_tables->dct_network->interface == (wiced_interface_t)WWD_ETHERNET_INTERFACE) ? "Ethernet" :
           "Unknown");
    wiced_log_printf("            Hostname: %s\n", dct_tables->dct_network->hostname.value);
    wiced_ip_get_ipv4_address(dct_tables->dct_network->interface, &ip_addr);
    wiced_log_printf("             IP addr: %d.%d.%d.%d\n",
           (int)((ip_addr.ip.v4 >> 24) & 0xFF), (int)((ip_addr.ip.v4 >> 16) & 0xFF),
           (int)((ip_addr.ip.v4 >> 8) & 0xFF),  (int)(ip_addr.ip.v4 & 0xFF));
}

static void avs_app_print_wifi_info(avs_app_dct_collection_t* dct_tables)
{
    wiced_security_t sec;
    uint32_t channel;
    int band;
    char mac_str[MAC_STR_LEN], wiced_mac_str[MAC_STR_LEN];
    wiced_mac_t wiced_mac;

    wwd_wifi_get_mac_address( &wiced_mac, WICED_STA_INTERFACE );
    avs_app_get_mac_addr_text(&wiced_mac, wiced_mac_str, sizeof(wiced_mac_str));
    avs_app_get_mac_addr_text(&dct_tables->dct_wifi->mac_address, mac_str, sizeof(mac_str));

    if (dct_tables->dct_network->interface == WICED_STA_INTERFACE)
    {
        sec = dct_tables->dct_wifi->stored_ap_list[0].details.security;
        wiced_log_printf("  WiFi DCT:\n");

        wiced_log_printf("     WICED MAC (STA): %s\n", wiced_mac_str);

        wiced_log_printf("                 MAC: %s\n", mac_str);
        wiced_log_printf("                SSID: %.*s\n", dct_tables->dct_wifi->stored_ap_list[0].details.SSID.length, dct_tables->dct_wifi->stored_ap_list[0].details.SSID.value);
        wiced_log_printf("            Security: %s\n", avs_app_get_security_type_name(sec));

        if (dct_tables->dct_wifi->stored_ap_list[0].details.security != WICED_SECURITY_OPEN)
        {
            wiced_log_printf("          Passphrase: %.*s\n", dct_tables->dct_wifi->stored_ap_list[0].security_key_length, dct_tables->dct_wifi->stored_ap_list[0].security_key);
        }
        else
        {
            wiced_log_printf("          Passphrase: none\n");
        }

        channel = dct_tables->dct_wifi->stored_ap_list[0].details.channel;
        band = dct_tables->dct_wifi->stored_ap_list[0].details.band;
        wiced_log_printf("             Channel: %d\n", (int)channel);
        wiced_log_printf("                Band: %s\n", (band == WICED_802_11_BAND_2_4GHZ) ? "2.4GHz" : "5GHz");
    }
    else
    {
        /*
         * Nothing for AP interface yet.
         */
    }
}

static void avs_app_print_current_info(avs_app_dct_collection_t* dct_tables)
{
    uint32_t channel;

    wiced_wifi_get_channel(&channel);
    wiced_log_printf(" Current:\n");
    wiced_log_printf("             Channel: %lu\n                Band: %s\n",
                     channel, channel <= 13 ? "2.4GHz" : "5GHz");
}

void avs_app_config_print_info(avs_app_dct_collection_t* dct_tables)
{
    wiced_log_printf("\nConfig Info: * = dirty\n");
    avs_app_print_app_info(dct_tables);
    avs_app_print_network_info(dct_tables);
    avs_app_print_wifi_info(dct_tables);
    avs_app_print_current_info(dct_tables);
    wiced_log_printf("\n");
}

static wiced_result_t avs_app_config_load_dct_wifi(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    result = wiced_dct_read_lock((void**)&dct_tables->dct_wifi, WICED_TRUE, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t));
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Can't get WIFi configuration!\n");
    }
    return result;
}

static wiced_result_t avs_app_config_load_dct_network(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    result = wiced_dct_read_lock( (void**)&dct_tables->dct_network, WICED_TRUE, DCT_NETWORK_CONFIG_SECTION, 0, sizeof(platform_dct_network_config_t) );
    if (result != WICED_SUCCESS)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Can't get Network configuration!\n");
    }
    return result;
}

wiced_result_t avs_app_config_init(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    /* wifi */
    result = avs_app_config_load_dct_wifi(dct_tables);

    /* network */
    result |= avs_app_config_load_dct_network(dct_tables);

    /* App */
    result |= wiced_dct_read_lock( (void**)&dct_tables->dct_app, WICED_TRUE, DCT_APP_SECTION, 0, sizeof(avs_app_dct_t));

    return result;
}

static wiced_result_t avs_app_config_unload_dct_wifi(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result = WICED_SUCCESS;

    if (dct_tables != NULL && dct_tables->dct_wifi != NULL)
    {
        result = wiced_dct_read_unlock(dct_tables->dct_wifi, WICED_TRUE);
        if (result == WICED_SUCCESS)
        {
            dct_tables->dct_wifi = NULL;
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Can't Free/Release WiFi Configuration !\n");
        }
    }

    return result;
}

static wiced_result_t avs_app_config_unload_dct_network(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result = WICED_SUCCESS;

    if (dct_tables != NULL && dct_tables->dct_network != NULL)
    {
        result = wiced_dct_read_unlock(dct_tables->dct_network, WICED_TRUE);
        if (result == WICED_SUCCESS)
        {
            dct_tables->dct_network = NULL;
        }
        else
        {
            wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Can't Free/Release Network Configuration !\n");
        }
    }

    return result;
}

wiced_result_t avs_app_config_deinit(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    result = wiced_dct_read_unlock(dct_tables->dct_app, WICED_TRUE);

    result |= avs_app_config_unload_dct_network(dct_tables);
    result |= avs_app_config_unload_dct_wifi(dct_tables);

    return result;
}

wiced_result_t avs_app_config_reload_dct_wifi(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    result  = avs_app_config_unload_dct_wifi(dct_tables);
    result |= avs_app_config_load_dct_wifi(dct_tables);

    return result;
}

wiced_result_t avs_app_config_reload_dct_network(avs_app_dct_collection_t* dct_tables)
{
    wiced_result_t result;

    result  = avs_app_config_unload_dct_network(dct_tables);
    result |= avs_app_config_load_dct_network(dct_tables);

    return result;
}
