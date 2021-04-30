/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/* Copyright (C) 2020 Avnet, Softweb Inc. - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */
#include <string.h>
#include <stdio.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include "nrf_cert_store.h"
#include "iotconnect_socket_https.h"

#define HTTPS_PORT 443
#define MAX_RECV_LEN 2048


static int parse_response(IotconnectNrfHttpResponse *response,
                          char *recv_buff,
                          size_t recv_buff_len
) {
    char *header_end = strstr(recv_buff, "\r\n\r\n"); // end of headers has 2 newlines
    if (!header_end) {
        printk("parse_response: Did not receive a valid HTTP header\n");
        return -1;
    }
    size_t header_size = header_end - recv_buff;
    response->header = (char *) malloc(header_size + 1);
    response->data = (char *) malloc(recv_buff_len - header_size + 1);
    *response->header = 0;
    *response->data = 0;
    if (NULL == response->header) {
        printk("parse_response: Unable to allocate header buffer\n");
        return -2;

    }
    strncpy(response->header, recv_buff, header_size);
    response->header[header_size] = 0;
    const size_t payload_len = recv_buff_len - header_size - 4; // 4 = CRLF
    char *data_start = header_end + 4;
    if (strstr(response->header, "Transfer-Encoding: chunked")) {
        int chunk_size = 0;
        int data_size = 0;
        do {
            if (1 != sscanf(data_start, "%x", &chunk_size)) {
                printk("parse_response: Cannot scan chunk header size in string %s\n", data_start);
                return -1;
            }
            if (0 == chunk_size) {
                if (0 == data_size) {
                    printk("parse_response: Got chunk size 0 and no data\n");
                } // else we are done
                break;
            }
            if (chunk_size + data_size > payload_len) {
                printk(
                        "parse_response: Unable to read chunk of size %u exceeding data length of %u\n",
                        chunk_size,
                        payload_len
                );
                return -4;
            }
            printk("Chunk size %d\n", chunk_size);
            data_start = strstr(data_start, "\r\n");
            if (!data_start) {
                printk("parse_response: Cannot find newline after chunk header\n");
                return -3;
            }
            data_start += 2; // \r\n at end of chunk header
            strncat(response->data, data_start, chunk_size);
            data_size += chunk_size;
            response->data[data_size] = 0;
            data_start = strstr(data_start, "\r\n");
            data_start += 2; // \r\n at end of data
        } while (chunk_size > 0);
    } else {
        // assume payload with content length and return
        memcpy(response->data, data_start, payload_len /* two newlines */);
        response->data[payload_len] = 0;
        printk("Data: size %d data:====\n%s\n====\n", payload_len, response->data);
    }
    return 0;
}

void iotconnect_https_request(
        IotconnectNrfHttpResponse *response,
        const char *host,
        sec_tag_t sec_tag,
        const char *send_str
) {
    struct addrinfo *ai = NULL;
    int err;
    int fd = -1;
    size_t off;

    response->data = NULL;
    response->header = NULL;
    char *recv_buff = response->raw_response = malloc(MAX_RECV_LEN);

    if (NULL == recv_buff) {
        printk("Out of memory while allocating receive buffer");
        goto clean_up;
    }
    recv_buff[0] = 0; // terminate it as a string

    struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_STREAM,
    };
    err = getaddrinfo(host, NULL, &hints, &ai);
    if (err) {
        printk("Unable to resolve host %s\n", host);
        goto clean_up;
    }

    ((struct sockaddr_in *) ai->ai_addr)->sin_port = htons(HTTPS_PORT);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

    if (fd == -1) {
        printk("Failed to open socket!\n");
        goto clean_up;
    }

    err = nrf_cert_store_configure_https_fd(sec_tag, fd);
    if (err) {
        goto clean_up;
    }

    struct sockaddr_in *a = (struct sockaddr_in *) (ai->ai_addr);
    printk("Connecting to %s %d.%d.%d.%d ... ", host,
           a->sin_addr.s4_addr[0],
           a->sin_addr.s4_addr[1],
           a->sin_addr.s4_addr[2],
           a->sin_addr.s4_addr[3]
    );

    err = connect(fd, ai->ai_addr, sizeof(struct sockaddr_in));
    if (err) {
        printk("connect() failed, err: %d\n", errno);
        goto clean_up;
    }
    printk("OK\n");
    off = 0;  //
    //printk("send_buf: %s\n", send_buf);
    size_t send_str_len = strlen(send_str);
    do {
        int bytes = send(fd, &send_str[off], send_str_len - off, 0);
        if (bytes < 0) {
            printk("send() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
    } while (off < send_str_len);

    printk("%d bytes sent.\n", off);

    off = 0;
    int bytes;
    do {
        bytes = recv(fd, &recv_buff[off], MAX_RECV_LEN - 1 /* for null */ - off, 0);
        if (bytes < 0) {
            printk("recv() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
        //printk("got %d bytes\n", bytes);
    } while (bytes != 0 /* peer closed connection */);
    printk("%d bytes received.\n", off);
    recv_buff[off] = 0;

    if (off == 0) {
        printk("Got empty response from server\n");
        goto clean_up;
    }

    if (parse_response(response, recv_buff, off)) {
        goto clean_up;
    }

    // good response, so only clean up resources
    goto clean_up_resources;

    clean_up:
    iotconnect_free_https_response(response);

    // fall through
    clean_up_resources:
    if (ai) {
        freeaddrinfo(ai);
    }
    if (fd >= 0) {
        close(fd);
    }
}

void iotconnect_free_https_response(IotconnectNrfHttpResponse *response) {
    free(response->header);
    free(response->data);
    free(response->raw_response);
    response->data = NULL;
    response->header = NULL;
    response->raw_response = NULL;
}
