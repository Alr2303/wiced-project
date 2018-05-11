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
 * AVS Application Playlist Routines
 *
 */

#include <ctype.h>

#include "avs_app.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Static Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/

static wiced_result_t avs_app_parse_pls(avs_app_audio_node_t* play)
{
    char* playlist;
    char* ptr;
    char* end;

    /*
     * Parse the pls playlist. Simple parse that's looking for
     * one (and only one) playlist entry.
     */

    playlist = play->playlist_data;

    /*
     * Make sure that there's a playlist tag.
     */

    if ((ptr = strstr(playlist, "[playlist]")) == NULL)
    {
        return WICED_ERROR;
    }

    /*
     * We only want one entry (i.e. radio station streaming playlist).
     */

    if ((ptr = strcasestr(playlist, "NumberOfEntries")) == NULL)
    {
        return WICED_ERROR;
    }

    while (*ptr && *ptr != '=')
    {
        ptr++;
    }

    if (*ptr++ != '=')
    {
        return WICED_ERROR;
    }

    if (atoi(ptr) != 1)
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "PLS playlist has more than one entry - only processing first entry\n");
    }

    /*
     * Now isolate the URL and move it to the beginning of the playlist.
     */

    if ((ptr = strstr(playlist, "File1")) != NULL)
    {
        while (*ptr && *ptr != '=')
        {
            ptr++;
        }

        if (*ptr++ != '=')
        {
            return WICED_ERROR;
        }

        end = ptr;
        while (*end && !isspace((int)*end))
        {
            end++;
        }
        *end++ = '\0';

        if (ptr != playlist)
        {
            memmove(playlist, ptr, strlen(ptr) + 1);
        }
        play->num_playlist_entries++;
    }

    return WICED_SUCCESS;
}


static wiced_result_t avs_app_parse_m3u(avs_app_audio_node_t* play)
{
    char* entry;
    char* start;
    char* ptr;
    char* end;
    int len;

    /*
     * Parse the m3u playlist. All we're going to do is isolate the
     * non-comment and non-empty lines. We aren't going to try and validate the
     * entries.
     */

    entry = play->playlist_data;
    ptr   = play->playlist_data;
    end   = &play->playlist_data[strlen(play->playlist_data)];
    while (ptr < end)
    {
        if (*ptr == '#' || isspace((int)*ptr))
        {
            /*
             * This line is a comment or an empty line.
             */

            while (*ptr != '\r' && *ptr != '\n')
            {
                ptr++;
            }

            while (*ptr == '\r' || *ptr == '\n')
            {
                ptr++;
            }
            continue;
        }

        /*
         * We have a playlist entry. Find the end of it and make sure it's nul terminated.
         */

        start = ptr;
        while (*ptr && !isspace((int)*ptr))
        {
            ptr++;
        }
        *ptr++ = '\0';

        len = strlen(start) + 1;
        if (start != entry)
        {
            memmove(entry, start, len);
        }
        entry += len;
        play->num_playlist_entries++;
    }

    return WICED_SUCCESS;
}


wiced_result_t avs_app_parse_playlist(avs_app_audio_node_t* play)
{
    wiced_result_t result = WICED_SUCCESS;

    /*
     * Important note about our playlist processing.
     * This routine does not try and perform comprehensive parsing and
     * validation. Rather we just try and pull out some basic information
     * so that we can play.
     */

    play->cur_playlist_entry   = 0;
    play->num_playlist_entries = 0;

    if (play->playlist_type == AUDIO_CLIENT_PLAYLIST_M3U || play->playlist_type == AUDIO_CLIENT_PLAYLIST_M3U8)
    {
        result = avs_app_parse_m3u(play);
    }
    else if (play->playlist_type == AUDIO_CLIENT_PLAYLIST_PLS)
    {
        result = avs_app_parse_pls(play);
    }
    else
    {
        wiced_log_msg(WLF_DEF, WICED_LOG_ERR, "Unknown playlist type: %d\n", play->playlist_type);
        result = WICED_ERROR;
    }

    return result;
}
