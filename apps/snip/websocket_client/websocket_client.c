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
 * Websocket Client Application
 *
 * This application snippet demonstrates how to use a simple Websocket Client protocol
 *
 * Features demonstrated
 *  - Wi-Fi client mode
 *  - DNS lookup
 *  - Secure/unsecure websocket( wiced_websocket_secure_connect/wiced_websocket_connect) client connection
 *
 * Application Instructions
 * 1. Modify the CLIENT_AP_SSID/CLIENT_AP_PASSPHRASE Wi-Fi credentials
 *    in the wifi_config_dct.h header file to match your Wi-Fi access point
 * 2. Connect a PC terminal to the serial port of the WICED Eval board,
 *    then build and download the application as described in the WICED
 *    Quick Start Guide
 *
 * After the download completes, the application :
 *  - Connects to the Wi-Fi network specified
 *  - Resolves the websocket server ip address
 *  - Connects to the websocket server and sends two test frames
 *  --1. character set 0-9
 *  --2. websocket ping request/response (not icmp)
 *
 *  The snippet also shows how the websocket API can be used.
 *  i.e. wiced_websocket_t API has the following callbacks:
 *   on open callback       : called once websocket handshake is complete
 *   on close callback      : called on socket close
 *   on error callback      : called on websocket error defined by wiced_websocket_error_t
 *   on message callback    : called whenever a new arrives on the websocket
 *
 *   Also note when sending frames through the wiced_websocket_frame_t structure,
 *   the application must define if this is last frame in message or not
 *   Frames are currently limited to 1024 bytes
 *
 *Limitations:
 *   Does not yet handle fragmentation across multiple packets
 *
 *   Note: The default websocket server used with this snippet is from echo.websocket.org
 *   This is a publicly available server, and is not managed by Cypress
 */

#include <stdlib.h>
#include "wiced.h"
#include "websocket.h"
#include "wiced_tls.h"

/******************************************************
 *                      Macros
 ******************************************************/

#define WEBSOCKET_SNIPPET_VERIFY(x)  {wiced_result_t res = (x); if (res != WICED_SUCCESS){ goto exit;}}
#define FINAL_FRAME WICED_TRUE
#define USE_WEBSOCKET_SECURE_CLIENT

/******************************************************
 *                    Constants
 ******************************************************/
#define BUFFER_LENGTH     (1024)
#define PING_MESSAGE      "This is a PING message from the websocket client"

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
 *               Function Declarations
 ******************************************************/

wiced_result_t on_open_websocket_callback   ( void* websocket );
wiced_result_t on_close_websocket_callback  ( void* websocket );
wiced_result_t on_error_websocket_callback  ( void* websocket );
wiced_result_t on_message_websocket_callback( void* websocket );

/******************************************************
 *               Variables Definitions
 ******************************************************/
static wiced_websocket_frame_t tx_frame;
static wiced_websocket_frame_t rx_frame;

static char rx_buffer[ BUFFER_LENGTH ];
static char tx_buffer[ BUFFER_LENGTH ] = { '0','1','2','3','4','5','6','7','8','9','\0' };

static wiced_semaphore_t received_message_event;

#if 1
//Entrust.Net Certification key
static const char httpbin_root_ca_certificate[] =
        "-----BEGIN CERTIFICATE-----\n"                                          \
        "MIIEKjCCAxKgAwIBAgIEOGPe+DANBgkqhkiG9w0BAQUFADCBtDEUMBIGA1UEChML\n"     \
        "RW50cnVzdC5uZXQxQDA+BgNVBAsUN3d3dy5lbnRydXN0Lm5ldC9DUFNfMjA0OCBp\n"     \
        "bmNvcnAuIGJ5IHJlZi4gKGxpbWl0cyBsaWFiLikxJTAjBgNVBAsTHChjKSAxOTk5\n"     \
        "IEVudHJ1c3QubmV0IExpbWl0ZWQxMzAxBgNVBAMTKkVudHJ1c3QubmV0IENlcnRp\n"     \
        "ZmljYXRpb24gQXV0aG9yaXR5ICgyMDQ4KTAeFw05OTEyMjQxNzUwNTFaFw0yOTA3\n"     \
        "MjQxNDE1MTJaMIG0MRQwEgYDVQQKEwtFbnRydXN0Lm5ldDFAMD4GA1UECxQ3d3d3\n"     \
        "LmVudHJ1c3QubmV0L0NQU18yMDQ4IGluY29ycC4gYnkgcmVmLiAobGltaXRzIGxp\n"     \
        "YWIuKTElMCMGA1UECxMcKGMpIDE5OTkgRW50cnVzdC5uZXQgTGltaXRlZDEzMDEG\n"     \
        "A1UEAxMqRW50cnVzdC5uZXQgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkgKDIwNDgp\n"     \
        "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArU1LqRKGsuqjIAcVFmQq\n"     \
        "K0vRvwtKTY7tgHalZ7d4QMBzQshowNtTK91euHaYNZOLGp18EzoOH1u3Hs/lJBQe\n"     \
        "sYGpjX24zGtLA/ECDNyrpUAkAH90lKGdCCmziAv1h3edVc3kw37XamSrhRSGlVuX\n"     \
        "MlBvPci6Zgzj/L24ScF2iUkZ/cCovYmjZy/Gn7xxGWC4LeksyZB2ZnuU4q941mVT\n"     \
        "XTzWnLLPKQP5L6RQstRIzgUyVYr9smRMDuSYB3Xbf9+5CFVghTAp+XtIpGmG4zU/\n"     \
        "HoZdenoVve8AjhUiVBcAkCaTvA5JaJG/+EfTnZVCwQ5N328mz8MYIWJmQ3DW1cAH\n"     \
        "4QIDAQABo0IwQDAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNV\n"     \
        "HQ4EFgQUVeSB0RGAvtiJuQijMfmhJAkWuXAwDQYJKoZIhvcNAQEFBQADggEBADub\n"     \
        "j1abMOdTmXx6eadNl9cZlZD7Bh/KM3xGY4+WZiT6QBshJ8rmcnPyT/4xmf3IDExo\n"     \
        "U8aAghOY+rat2l098c5u9hURlIIM7j+VrxGrD9cv3h8Dj1csHsm7mhpElesYT6Yf\n"     \
        "zX1XEC+bBAlahLVu2B064dae0Wx5XnkcFMXj0EyTO2U87d89vqbllRrDtRnDvV5b\n"     \
        "u/8j72gZyxKTJ1wDLW8w0B62GqzeWvfRqqgnpv55gcR5mTNXuhKwqeBCbJPKVt7+\n"     \
        "bYQLCIt+jerXmCHG8+c8eS9enNFMFY3h7CI3zJpDC5fcgJCNs2ebb0gIFVbPv/Er\n"     \
        "fF6adulZkMV8gzURZVE=\n"                                                 \
        "-----END CERTIFICATE-----\n";
#endif

#if 0
// Using this root certificate will throw a TLS handshake error
static const char httpbin_root_ca_certificate[] =
        "-----BEGIN CERTIFICATE-----\n"
    "MIIGCDCCA/CgAwIBAgIQKy5u6tl1NmwUim7bo3yMBzANBgkqhkiG9w0BAQwFADCB\n"
    "hTELMAkGA1UEBhMCR0IxGzAZBgNVBAgTEkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4G\n"
    "A1UEBxMHU2FsZm9yZDEaMBgGA1UEChMRQ09NT0RPIENBIExpbWl0ZWQxKzApBgNV\n"
    "BAMTIkNPTU9ETyBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTQwMjEy\n"
    "MDAwMDAwWhcNMjkwMjExMjM1OTU5WjCBkDELMAkGA1UEBhMCR0IxGzAZBgNVBAgT\n"
    "EkdyZWF0ZXIgTWFuY2hlc3RlcjEQMA4GA1UEBxMHU2FsZm9yZDEaMBgGA1UEChMR\n"
    "Q09NT0RPIENBIExpbWl0ZWQxNjA0BgNVBAMTLUNPTU9ETyBSU0EgRG9tYWluIFZh\n"
    "bGlkYXRpb24gU2VjdXJlIFNlcnZlciBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEP\n"
    "ADCCAQoCggEBAI7CAhnhoFmk6zg1jSz9AdDTScBkxwtiBUUWOqigwAwCfx3M28Sh\n"
    "bXcDow+G+eMGnD4LgYqbSRutA776S9uMIO3Vzl5ljj4Nr0zCsLdFXlIvNN5IJGS0\n"
    "Qa4Al/e+Z96e0HqnU4A7fK31llVvl0cKfIWLIpeNs4TgllfQcBhglo/uLQeTnaG6\n"
    "ytHNe+nEKpooIZFNb5JPJaXyejXdJtxGpdCsWTWM/06RQ1A/WZMebFEh7lgUq/51\n"
    "UHg+TLAchhP6a5i84DuUHoVS3AOTJBhuyydRReZw3iVDpA3hSqXttn7IzW3uLh0n\n"
    "c13cRTCAquOyQQuvvUSH2rnlG51/ruWFgqUCAwEAAaOCAWUwggFhMB8GA1UdIwQY\n"
    "MBaAFLuvfgI9+qbxPISOre44mOzZMjLUMB0GA1UdDgQWBBSQr2o6lFoL2JDqElZz\n"
    "30O0Oija5zAOBgNVHQ8BAf8EBAMCAYYwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNV\n"
    "HSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwGwYDVR0gBBQwEjAGBgRVHSAAMAgG\n"
    "BmeBDAECATBMBgNVHR8ERTBDMEGgP6A9hjtodHRwOi8vY3JsLmNvbW9kb2NhLmNv\n"
    "bS9DT01PRE9SU0FDZXJ0aWZpY2F0aW9uQXV0aG9yaXR5LmNybDBxBggrBgEFBQcB\n"
    "AQRlMGMwOwYIKwYBBQUHMAKGL2h0dHA6Ly9jcnQuY29tb2RvY2EuY29tL0NPTU9E\n"
    "T1JTQUFkZFRydXN0Q0EuY3J0MCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5jb21v\n"
    "ZG9jYS5jb20wDQYJKoZIhvcNAQEMBQADggIBAE4rdk+SHGI2ibp3wScF9BzWRJ2p\n"
    "mj6q1WZmAT7qSeaiNbz69t2Vjpk1mA42GHWx3d1Qcnyu3HeIzg/3kCDKo2cuH1Z/\n"
    "e+FE6kKVxF0NAVBGFfKBiVlsit2M8RKhjTpCipj4SzR7JzsItG8kO3KdY3RYPBps\n"
    "P0/HEZrIqPW1N+8QRcZs2eBelSaz662jue5/DJpmNXMyYE7l3YphLG5SEXdoltMY\n"
    "dVEVABt0iN3hxzgEQyjpFv3ZBdRdRydg1vs4O2xyopT4Qhrf7W8GjEXCBgCq5Ojc\n"
    "2bXhc3js9iPc0d1sjhqPpepUfJa3w/5Vjo1JXvxku88+vZbrac2/4EjxYoIQ5QxG\n"
    "V/Iz2tDIY+3GH5QFlkoakdH368+PUq4NCNk+qKBR6cGHdNXJ93SrLlP7u3r7l+L4\n"
    "HyaPs9Kg4DdbKDsx5Q5XLVq4rXmsXiBmGqW5prU5wfWYQ//u+aen/e7KJD2AFsQX\n"
    "j4rBYKEMrltDR5FL1ZoXX/nUh8HCjLfn4g8wGTeGrODcQgPmlKidrv0PJFGUzpII\n"
    "0fxQ8ANAe4hZ7Q7drNJ3gjTcBpUC2JD5Leo31Rpg0Gcg19hCC0Wvgmje3WYkN5Ap\n"
    "lBlGGSW4gNfL1IYoakRwJiNiqZ+Gb7+6kHDSVneFeO/qJakXzlByjAA6quPbYzSf\n"
    "+AZxAeKCINT+b72x\n"
    "-----END CERTIFICATE-----\n";
#endif
/* Following are the standard header fields in use
 * by the websoket protocol
 * Additional header fields may be added
 *  GET /chat HTTP/1.1
    Host: server.example.com
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
    Origin: http://example.com
    Sec-WebSocket-Protocol: chat, superchat
    Sec-WebSocket-Version: 13
 */

static const wiced_websocket_client_url_protocol_t websocket_header =
{
   .request_uri            = "?encoding=text",
   .host                   = "echo.websocket.org", // connection to hosts with an ip-address(like internal servers) will also work.
   //.host                   = "192.168.1.49",
   .origin                 = "172.16.61.228",
   .sec_websocket_protocol =  NULL,
};

static wiced_websocket_t    websocket;
#ifdef USE_WEBSOCKET_SECURE_CLIENT
static wiced_tls_identity_t  tls_identity;
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{

    wiced_init( );

    wiced_rtos_init_semaphore( &received_message_event );

    if ( wiced_network_up( WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL ) != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "\r\nFailed to bring up network\r\n" ) );
        return;
    }

#ifdef USE_WEBSOCKET_SECURE_CLIENT
    wiced_result_t result = WICED_ERROR;
    /* Initialize the root CA certificate */
    result = wiced_tls_init_root_ca_certificates( httpbin_root_ca_certificate, strlen(httpbin_root_ca_certificate) );
    if ( result != WICED_SUCCESS )
    {
        WPRINT_APP_INFO( ( "Error: Root CA certificate failed to initialize: %u\n", result) );
        return;
    }
#endif

    /* Initialise the websocket */
    wiced_websocket_initialise( &websocket );

    /* Set up websocket transmit and receive buffers */
    wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_TEXT_FRAME, strlen(tx_buffer), tx_buffer, BUFFER_LENGTH );

    wiced_websocket_initialise_rx_frame( &rx_frame, rx_buffer, BUFFER_LENGTH );

    /*register websocket callbacks */
    wiced_websocket_register_callbacks( &websocket, on_open_websocket_callback, on_close_websocket_callback, on_message_websocket_callback, on_error_websocket_callback  );

    WPRINT_APP_INFO( ( "\r\nConnecting to server...\r\n\r\n" ) );

    /* Establish a websocket connection with the server */
    WPRINT_APP_INFO(("Trying to connect\n"));
#ifdef USE_WEBSOCKET_SECURE_CLIENT
    WEBSOCKET_SNIPPET_VERIFY( wiced_websocket_secure_connect( &websocket, &websocket_header, &tls_identity ) );
#else
    WEBSOCKET_SNIPPET_VERIFY( wiced_websocket_connect( &websocket, &websocket_header ) );
#endif

    WPRINT_APP_INFO( ( "\r\nConnected to server \r\n\r\n" ) );

    WPRINT_APP_INFO( ( "Sending text frame with data 0123456789. Expecting echo response...\r\n\r\n" ) );

    /* Send the text data to the server */
    WEBSOCKET_SNIPPET_VERIFY( wiced_websocket_send( &websocket, &tx_frame ) );

    wiced_rtos_get_semaphore( &received_message_event, WICED_WAIT_FOREVER );

    /* prepare PING frame */
    wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_PING, strlen( PING_MESSAGE ), PING_MESSAGE, strlen( PING_MESSAGE ) );

    WPRINT_APP_INFO( ( "Sending ping frame. Expecting PONG response...\r\n\r\n" ) );

    /* send PING frame */
    WEBSOCKET_SNIPPET_VERIFY( wiced_websocket_send( &websocket, &tx_frame ) );

    wiced_rtos_get_semaphore( &received_message_event, WICED_WAIT_FOREVER );

    WPRINT_APP_INFO( ( "\r\nWebsocket client demo complete. Closing connection.\r\n") );

exit:

    wiced_websocket_close( &websocket, WEBSOCKET_CLOSE_STATUS_CODE_NORMAL, "Closing session\r\n" );

    wiced_websocket_uninitialise( &websocket );

}

wiced_result_t on_open_websocket_callback( void* websocket )
{
    UNUSED_PARAMETER( websocket );

    WPRINT_APP_INFO( ( "\r\nConnection open received\r\n " ) );

    return WICED_SUCCESS;
}

wiced_result_t on_close_websocket_callback( void* websocket )
{
    UNUSED_PARAMETER( websocket );

    WPRINT_APP_INFO( ( "\r\nConnection closed received\r\n " ) );

    return WICED_SUCCESS;
}

wiced_result_t on_message_websocket_callback( void* websocket )
{

    WPRINT_APP_INFO( ( "[Message received]\r\n ") );

    wiced_websocket_receive( (wiced_websocket_t*)websocket, &rx_frame );

    WPRINT_APP_INFO( ( "Server returned:\r\n") );

    switch( rx_frame.payload_type )
    {
        case WEBSOCKET_TEXT_FRAME :
        {
            WPRINT_APP_INFO( ( "\tFrame Type: TEXT FRAME\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            break;
        }

        case WEBSOCKET_PONG:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: PONG\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            break;
        }

        case WEBSOCKET_PING:
        {
            char* pong_payload = NULL;
            uint8_t nr_bytes = 0;
            WPRINT_APP_INFO( ( "\tFrame Type: PING\r\n" ) );
            WPRINT_APP_INFO( ( "\tFrame Data: %s\r\n", (char*)rx_frame.payload ) );
            nr_bytes = strlen(rx_frame.payload) + 1;
            pong_payload = malloc(nr_bytes);
            if(pong_payload == NULL)
            {
                WPRINT_APP_INFO(("[App] Error sending PONG\n"));
                break;
            }

            memcpy( pong_payload, rx_frame.payload, nr_bytes );
            wiced_websocket_initialise_tx_frame( &tx_frame, FINAL_FRAME, WEBSOCKET_PONG, (nr_bytes- 1), pong_payload, nr_bytes );
            WICED_VERIFY( wiced_websocket_send(websocket, &tx_frame) );
            WPRINT_APP_INFO((" \tSending PONG done\r\n"));
            free(pong_payload);
            break;
        }

        case WEBSOCKET_CONNECTION_CLOSE:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: CONNECTION CLOSE\r\n" ) );

            wiced_websocket_unregister_callbacks( (wiced_websocket_t*)websocket );
            break;
        }

        case WEBSOCKET_BINARY_FRAME:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: BINARY\r\n" ) );
            break;
        }

        case WEBSOCKET_CONTINUATION_FRAME:
        {
            WPRINT_APP_INFO( ( "\tFrame Type: CONTINUATION\r\n" ) );
            break;
        }

        default:
            WPRINT_APP_INFO( ( "\tFrame Type: RESERVED\r\n" ) );
            break;
    }

    printf("\r\n");

    wiced_rtos_set_semaphore( &received_message_event );

    return WICED_SUCCESS;
}

wiced_result_t on_error_websocket_callback( void* websocket )
{
    WPRINT_APP_INFO( ( "\r\nError number received = %d\r\n", ((wiced_websocket_t*)websocket)->error_type ) );

    return WICED_SUCCESS;
}
