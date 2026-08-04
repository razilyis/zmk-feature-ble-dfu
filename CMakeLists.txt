# Copyright (c) 2026 zmk-feature-ble-dfu contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_BLE_DFU)
  zephyr_library()
  zephyr_library_sources(src/behavior_ble_dfu.c)
endif()

