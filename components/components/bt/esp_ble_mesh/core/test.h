/*  Bluetooth Mesh */

/*
 * SPDX-FileCopyrightText: 2017 Intel Corporation
 * SPDX-FileContributor: 2018-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _BLE_MESH_TEST_H_
#define _BLE_MESH_TEST_H_

#include "net.h"
#include "proxy_client.h"
#include "mesh/adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bt_mesh_device_network_info {
    uint8_t  net_key[16];
    uint16_t net_idx;
    uint8_t  flags;
    uint32_t iv_index;
    uint16_t unicast_addr;
    uint8_t  dev_key[16];
    uint8_t  app_key[16];
    uint16_t app_idx;
    uint16_t group_addr;
};

int bt_mesh_device_auto_enter_network(struct bt_mesh_device_network_info *info);

/* Before trying to update the white list, users need to make sure that
 * one of the following conditions is satisfied:
 * 1. BLE scanning is disabled;
 * 2. BLE scanning is enabled with scan filter policy disabled;
 * If BLE scanning is enabled with scan filter policy enabled, users need
 * to stop BLE scanning firstly, then the white list can be updated.
 */
int bt_mesh_test_update_white_list(struct bt_mesh_white_list *wl);

int bt_mesh_test_start_scanning(bool wl_en);

int bt_mesh_test_stop_scanning(void);

typedef void (* bt_mesh_test_net_pdu_cb_t)(const uint8_t *data, uint16_t length);

extern bt_mesh_test_net_pdu_cb_t net_pdu_test_cb;

void bt_mesh_test_register_net_pdu_cb(bt_mesh_test_net_pdu_cb_t cb);

void bt_mesh_test_set_seq(uint32_t seq);

#ifdef __cplusplus
}
#endif

#endif /* _BLE_MESH_TEST_H_ */
