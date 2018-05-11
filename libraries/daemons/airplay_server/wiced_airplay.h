
/*
 * $ Copyright Broadcom Corporation $
 */

#pragma once

#include "wiced_result.h"
#include "platform_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

#define MAX_METADATA_STRING_LENGTH (60)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum
{
    DACP_DISCONNECTED,
    DACP_CONNECTED
} airplay_dacp_connection_state_t;

typedef enum
{
    AIRPLAY_SERVER_IDLE,
    AIRPLAY_SERVER_CONNECTED,
    AIRPLAY_SERVER_STREAMING,
    AIRPLAY_SERVER_PAUSED
} airplay_server_state_t;

typedef enum
{
    PLAYBACK_PROGRESS,
    PLAYBACK_METADATA
} airplay_playback_property_t;


/******************************************************
 *                 Type Definitions
 ******************************************************/

typedef wiced_result_t( *airplay_playpack_property_update_callback_t )( void* property_value, airplay_playback_property_t property_type );
typedef wiced_result_t( *airplay_state_transition_callback_t )( airplay_server_state_t state );

/******************************************************
 *                    Structures
 ******************************************************/

typedef struct
{
    uint32_t elapsed_time;
    uint32_t duration;
} airplay_playback_progress_t;

typedef struct
{
    char album_name[MAX_METADATA_STRING_LENGTH];
    char artist_name[MAX_METADATA_STRING_LENGTH];
    char composer_name[MAX_METADATA_STRING_LENGTH];
    char genre_name[MAX_METADATA_STRING_LENGTH];
    char song_name[MAX_METADATA_STRING_LENGTH];
} airplay_playback_metadata_t;

/******************************************************
 *                 Global Variables
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/** Starts the Airplay audio daemon.
 *
 * @param[in] interface                     : Network interface on which the daemon will be started, @ref wiced_interface_t
 * @param[in] device_name                   : Pointer to a string containing the desired name of the Airplay audio device
 * @param[in] device_model                  : Pointer to a string containing the desired model name of the Airplay audio device
 * @param[in] play_password                 : Pointer to a string containing the required playback password; can be NULL if no password is needed/required
 * @param[in] state_callback                : Call-back function providing playback status of the Airplay audio daemon, @ref airplay_state_transition_callback_t
 * @param[in] property_update_callback      : Call-back function providing updates regarding a set of properties monitored by the Airplay audio daemon, @ref airplay_playpack_property_update_callback_t
 * @param[in] properties_to_listen          : Bit-mask of playback properties to be monitored and updated by the Airplay audio daemon, @ref airplay_playback_property_t
 * @param[in] audio_playback_device_name    : Pointer to a string containing the name of the WICED audio playback device; use NULL to use the default WICED audio playback device on the platform
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_airplay_start_server            ( wiced_interface_t                           interface,
                                                       const char*                                 device_name,
                                                       const char*                                 device_model,
                                                       const char*                                 play_password,
                                                       airplay_state_transition_callback_t         state_callback,
                                                       airplay_playpack_property_update_callback_t property_update_callback,
                                                       uint16_t                                    properties_to_listen,
                                                       const platform_audio_device_id_t            audio_playback_device_id );


/** Stops the Airplay audio daemon.
 *
 * @param[in] interface                     : Network interface on which the daemon is running, @ref wiced_interface_t
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_airplay_stop_server             ( wiced_interface_t interface );


/** Sends playback transport commands (play, pause, stop, ...) and control playback-related functions (volume up, volume down, mute,...)
 *
 * @param[in] command                       : String containing the command name
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_airplay_dacp_schedule_command   ( const char* command );


/** Prevents the Airplay audio daemon from starting playback
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_airplay_prevent_playback        ( void );


/** Allows the Airplay audio daemon to start playback
 *
 * @return @ref wiced_result_t
 */
wiced_result_t wiced_airplay_allow_playback          ( void );


/** Provides a string containing the firmware version of the Airplay audio device; must be implemented by the application
 *
 * @param[in,out] version_string            : Address of a pointer to a string
 *
 * @return void
 */
extern void    host_get_firmware_version             ( const char** version_string );


/** Provides update on the status of the DACP connection; must be implemented by the application
 *
 * @param[in] new_state                     : status of the DACP connection, @ref airplay_dacp_connection_state_t
 *
 * @return void
 */
extern void    host_dacp_connection_state_changes    ( airplay_dacp_connection_state_t new_state );


#ifdef __cplusplus
}
#endif
