/*
 * $ Copyright Broadcom Corporation $
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************
 *                      Macros
 ******************************************************/
#ifdef WPRINT_ENABLE_MFI_INFO
    #define WPRINT_MFI_INFO(args) WPRINT_MACRO(args)
#else
    #define WPRINT_MFI_INFO(args)
#endif
#ifdef WPRINT_ENABLE_MFI_DEBUG
    #define WPRINT_MFI_DEBUG(args) WPRINT_MACRO(args)
#else
    #define WPRINT_MFI_DEBUG(args)
#endif
#ifdef WPRINT_ENABLE_MFI_ERROR
    #define WPRINT_MFI_ERROR(args) { WPRINT_MACRO(args); WICED_BREAK_IF_DEBUG(); }
#else
    #define WPRINT_MFI_ERROR(args) { WICED_BREAK_IF_DEBUG(); }
#endif
/******************************************************
 *                    Constants
 ******************************************************/
#define MAX_MFI_DEVICE_CERTIFICATE_SIZE   ( 1280 )

/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

wiced_result_t  wiced_mfi_init             ( void );
void            wiced_mfi_deinit           ( void );
wiced_result_t  wiced_mfi_create_signature ( const void* data_to_sign, uint16_t data_size, uint8_t* signature_buffer, uint16_t signature_buffer_size, uint16_t* signature_size );
wiced_result_t  wiced_mfi_copy_certificate ( uint8_t** certificate, unsigned int* certificate_size );
wiced_result_t  wiced_mfi_get_random_bytes ( void *buffer, uint16_t buffer_size );


#ifdef __cplusplus
}
#endif


