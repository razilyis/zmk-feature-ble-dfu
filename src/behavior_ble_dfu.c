/*
 * Copyright (c) 2026 zmk-feature-ble-dfu contributors
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT razily_behavior_ble_dfu

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>

#include <zmk_feature_ble_dfu/ble_dfu.h>

LOG_MODULE_REGISTER(zmk_ble_dfu, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    LOG_INF("Rebooting into Adafruit nRF52 BLE DFU mode");
    sys_reboot(ZMK_BLE_DFU_ADAFRUIT_OTA_REBOOT_TYPE);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_ble_dfu_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .locality = BEHAVIOR_LOCALITY_EVENT_SOURCE,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define BLE_DFU_INST(n)                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_ble_dfu_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BLE_DFU_INST)

#endif
