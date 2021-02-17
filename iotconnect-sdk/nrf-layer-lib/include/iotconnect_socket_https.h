/* Copyright (C) 2020 Avnet - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */


#ifndef IOTCONNECT_SAMPLE_NRF_HTTP_H
#define IOTCONNECT_SAMPLE_NRF_HTTP_H

#include <stddef.h>
#include <net/socket.h>
#include <net/tls_credentials.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct IOTCONNECT_NRF_HTTP_RESPONSE_T {
    char* header;
    char* data;
    char* raw_response; // for diagnostic purposes
} IOTCONNECT_NRF_HTTP_RESPONSE;

// Helper to deal with http chunked transfers which are always retuned by iotconnect services.
// Limited support for content with
// Content-Length headers instead of Transfer-Encoding: chunked.
void iotconnect_https_request(
        IOTCONNECT_NRF_HTTP_RESPONSE *response,
        const char *host,
        sec_tag_t sec_tag,
        const char *send_str
);
void iotconnect_free_https_response(IOTCONNECT_NRF_HTTP_RESPONSE *response);

#ifdef __cplusplus
}
#endif

#endif //IOTCONNECT_SAMPLE_NRF_HTTP_H
