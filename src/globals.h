#pragma once

#include <stdint.h>

#include "ux.h"

#include "io.h"
#include "types.h"
#include "constants.h"

/**
 * Global buffer for interactions between SE and MCU.
 */
extern uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

/**
 * Global structure to perform asynchronous UX aside IO operations.
 */
extern ux_state_t G_ux;

/**
 * Global structure with the parameters to exchange with the BOLOS UX application.
 */
extern bolos_ux_params_t G_ux_params;

/**
 * Global context for user requests.
 */
extern global_ctx_t G_context;

/**
 * Is it called through the exchange app
 */
extern bool G_called_from_swap;

/**
 * Use an union to avoid the UI variable footprints for the swap flow and vice versa
 */
typedef union swap_or_ui_u {
    struct {
        /**
         * The response to the swap is ready
         */
        bool response_ready;

        /**
         * Global context for swap requests.
         */
        swap_values_t values;
    } swap;

    struct {
        /**
         * Global variable with the caption of the current UI detail.
         */
        char detail_caption[DETAIL_CAPTION_MAX_LENGTH];
        /**
         * Global variable with the value of the current UI detail.
         */
        char detail_value[DETAIL_VALUE_MAX_LENGTH];

        uint8_t current_state;
    } ui;
} swap_or_ui_t;

extern swap_or_ui_t G;
