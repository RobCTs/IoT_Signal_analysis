/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "soc/interrupts.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PERIPH_LEDC_MODULE = 0,
    PERIPH_UART0_MODULE,
    PERIPH_UART1_MODULE,
    PERIPH_UART2_MODULE,
    PERIPH_USB_MODULE,
    PERIPH_I2C0_MODULE,
    PERIPH_I2C1_MODULE,
    PERIPH_I2S0_MODULE,
    PERIPH_I2S1_MODULE,
    PERIPH_LCD_CAM_MODULE,
    PERIPH_TIMG0_MODULE,
    PERIPH_TIMG1_MODULE,
    PERIPH_PWM0_MODULE,
    PERIPH_PWM1_MODULE,
    PERIPH_PWM2_MODULE,
    PERIPH_PWM3_MODULE,
    PERIPH_UHCI0_MODULE,
    PERIPH_UHCI1_MODULE,
    PERIPH_RMT_MODULE,
    PERIPH_PCNT_MODULE,
    PERIPH_SPI_MODULE,
    PERIPH_SPI2_MODULE,
    PERIPH_SPI3_MODULE,
    PERIPH_SDMMC_MODULE,
    PERIPH_TWAI_MODULE,
    PERIPH_RNG_MODULE,
    PERIPH_WIFI_MODULE,
    PERIPH_BT_MODULE,
    PERIPH_WIFI_BT_COMMON_MODULE,
    PERIPH_BT_BASEBAND_MODULE,
    PERIPH_BT_LC_MODULE,
    PERIPH_AES_MODULE,
    PERIPH_SHA_MODULE,
    PERIPH_HMAC_MODULE,
    PERIPH_DS_MODULE,
    PERIPH_RSA_MODULE,
    PERIPH_SYSTIMER_MODULE,
    PERIPH_GDMA_MODULE,
    PERIPH_DEDIC_GPIO_MODULE,
    PERIPH_SARADC_MODULE,
    PERIPH_TEMPSENSOR_MODULE,
    PERIPH_MODULE_MAX
} periph_module_t;

#ifdef __cplusplus
}
#endif
