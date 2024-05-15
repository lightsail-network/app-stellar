/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2024 Ledger SAS.
 *   (c) 2024 overcat.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *****************************************************************************/
#ifdef HAVE_NBGL
#include <stdbool.h>  // bool
#include <string.h>   // memset
#include "format.h"

#include "nbgl_use_case.h"

#include "display.h"
#include "globals.h"
#include "sw.h"
#include "stellar/printer.h"
#include "action/validate.h"

// Macros
#define TAG_VAL_LST_PAIR_NB 2

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_menu_main();
}

// Globals
static char str_values[TAG_VAL_LST_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static nbgl_pageInfoLongPress_t info_long_press;
static nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_PAIR_NB];
static nbgl_layoutTagValueList_t pair_list;

// Static functions declarations
static void review_start(void);
static void review_warning(void);
static void review_continue(void);
static void reject_confirmation(void);
static void reject_choice(void);

// Functions definitions
static void prepare_page(void) {
    explicit_bzero(caption_value_pairs, sizeof(caption_value_pairs));
    explicit_bzero(str_values, sizeof(str_values));

    // Address caption/value preparation.
    caption_value_pairs[0].item = "Address";
    if (!print_account_id(G_context.raw_public_key, str_values[0], DETAIL_VALUE_MAX_LENGTH, 0, 0)) {
        io_send_sw(SW_DISPLAY_ADDRESS_FAIL);
        return;
    }
    caption_value_pairs[0].value = str_values[0];

    // Hash caption/value preparation.
    caption_value_pairs[1].item = "Hash";
    if (!format_hex(G_context.hash, HASH_SIZE, str_values[1], DETAIL_VALUE_MAX_LENGTH)) {
        io_send_sw(SW_DISPLAY_TRANSACTION_HASH_FAIL);
        return;
    }
    caption_value_pairs[1].value = str_values[1];
}

static void reject_confirmation(void) {
    ui_action_validate_transaction(false);
    nbgl_useCaseStatus("Hash Rejected", false, ui_menu_main);
}

static void reject_choice(void) {
    nbgl_useCaseConfirm("Reject hash?",
                        NULL,
                        "Yes, Reject",
                        "Go back to hash",
                        reject_confirmation);
}

static void review_choice(bool confirm) {
    if (confirm) {
        ui_action_validate_transaction(true);
        nbgl_useCaseStatus("HASH SIGNED", true, ui_menu_main);
    } else {
        reject_choice();
    }
}

static void review_start(void) {
    nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                            "Review hash signing",
                            "",
                            "Reject hash",
                            review_warning,
                            reject_choice);
}

static void review_warning(void) {
    nbgl_useCaseReviewStart(NULL,
                            "WARNING",
                            "Dangerous Operation",
                            "Reject hash",
                            review_continue,
                            reject_choice);
}

static void review_continue(void) {
    pair_list.pairs = caption_value_pairs;
    pair_list.nbPairs = TAG_VAL_LST_PAIR_NB;

    info_long_press.text = "Sign hash?";
    info_long_press.icon = &C_icon_stellar_64px;
    info_long_press.longPressText = "Hold to sign";
    info_long_press.longPressToken = 0;
    info_long_press.tuneId = TUNE_TAP_CASUAL;

    nbgl_useCaseStaticReview(&pair_list, &info_long_press, "Reject hash", review_choice);
}

int ui_display_hash() {
    if (G_context.req_type != CONFIRM_HASH || G_context.state != STATE_NONE) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    prepare_page();
    review_start();
    return 0;
}
#endif  // HAVE_NBGL
