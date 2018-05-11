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
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "wiced_log.h"
#include "platform_audio.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define AVS_APP_DEFAULT_AUDIO_INPUT     AUDIO_DEVICE_ID_AK4961_ADC_DIGITAL_MIC
#define AVS_APP_DEFAULT_AUDIO_OUTPUT    PLATFORM_DEFAULT_AUDIO_OUTPUT

#define AVS_APP_VOLUME_MIN              0
#define AVS_APP_VOLUME_DEFAULT          70
#define AVS_APP_VOLUME_MAX              100

#define AVS_APP_REFRESH_TOKEN_LEN       2048
#define AVS_APP_CLIENT_ID_LEN           100
#define AVS_APP_CLIENT_SECRET_LEN       100

#define AVS_APP_UTC_OFFSET_MIN          ((-12) * 60 * 60 * 1000)
#define AVS_APP_UTC_OFFSET_MAX          (12 * 60 * 60 * 1000)

#define UTC_OFFSET_PDT                  ((-7) * 60 * 60 * 1000)
#define UTC_OFFSET_PST                  ((-8) * 60 * 60 * 1000)

#define AVS_APP_DEFAULT_REFRESH_TOKEN   ""
#define AVS_APP_DEFAULT_CLIENT_ID       ""
#define AVS_APP_DEFAULT_CLIENT_SECRET   ""

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    int                             volume;
    WICED_LOG_LEVEL_T               log_level;
    int32_t                         utc_offset_ms;

    platform_audio_device_id_t      audio_device_rx;     /* Audio capture device  */
    platform_audio_device_id_t      audio_device_tx;     /* Audio playback device */

    char                            refresh_token[AVS_APP_REFRESH_TOKEN_LEN];
    char                            client_id[AVS_APP_CLIENT_ID_LEN];
    char                            client_secret[AVS_APP_CLIENT_SECRET_LEN];

} avs_app_dct_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
