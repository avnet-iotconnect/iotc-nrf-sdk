//
// Copyright: Avnet, Softweb Inc. 2020
// Created by Nik Markovic <nikola.markovic@avnet.com> on 2/4/21.
//

#ifndef IOTCONNECT_APP_COMMON_H
#define IOTCONNECT_APP_COMMON_H

#include <stdbool.h>
#include "iotconnect_event.h"

#define LED_MAX 20U
#ifdef __cplusplus
extern "C" {
#endif

void command_status(IotclEventData data, bool status, const char *command_name, const char *message);
void process_command(IotclEventData data, char *args);

#ifdef __cplusplus
}
#endif

#endif //IOTCONNECT_APP_COMMON_H
