//
// Copyright: Avnet, Softweb Inc. 2020
// Created by Nikola Markovic on 6/15/20.
//

#ifndef IOTCONNECT_NRF_MODEM_IF_H
#define IOTCONNECT_NRF_MODEM_IF_H

#ifdef __cplusplus
extern "C" {
#endif

int nrf_modem_get_time(void);

const char *nrf_modem_get_imei(void);

#ifdef __cplusplus
}
#endif
#endif //IOTCONNECT_NRF_MODEM_TIME_H
