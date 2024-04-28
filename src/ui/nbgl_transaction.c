/*****************************************************************************
 *   Ledger Stellar App.
 *   (c) 2022 Ledger SAS.
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

#include "os.h"
#include "nbgl_use_case.h"

#include "display.h"
#include "constants.h"
#include "globals.h"
#include "sw.h"
#include "settings.h"
#include "plugin.h"
#include "action/validate.h"
#include "stellar/printer.h"
#include "stellar/formatter.h"
#include "stellar/printer.h"

// Macros
#define TAG_VAL_LST_MAX_LINES_PER_PAGE     10
#define TAG_VAL_LST_MAX_PAIR_NB            TAG_VAL_LST_MAX_LINES_PER_PAGE / 2
#define TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE 31
#define TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE  17
#define TAG_VAL_LST_VAL_MAX_LEN_PER_PAGE                                 \
    TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE *(TAG_VAL_LST_MAX_LINES_PER_PAGE - \
                                        1)  // -1 because at least one line is used by a tag item.
#define MAX_NUMBER_OF_PAGES 40
// Enums and Structs
typedef struct {
    uint8_t pagePairNb;  // how many data pairs are on the page
    bool centered_info;  // if true, only one caption/value pair is displayed on page, and it is
                         // centered.
    uint8_t data_idx;
} page_infos_t;

// Globals
static uint8_t nbPages;      // (nbPages + 1) = Number of pages to display transaction.
static int16_t currentPage;  // start from 0, eht sign confirmation page is nbPages + 1.
nbgl_layoutTagValue_t caption_value_pairs[TAG_VAL_LST_MAX_PAIR_NB];
static char str_values[TAG_VAL_LST_MAX_PAIR_NB][DETAIL_VALUE_MAX_LENGTH];
static char str_captions[TAG_VAL_LST_MAX_PAIR_NB][DETAIL_CAPTION_MAX_LENGTH];
static page_infos_t pagesInfos[MAX_NUMBER_OF_PAGES];
static formatter_data_t formatter_data;

// Validate/Invalidate transaction and go back to home
static void ui_action_validate_transaction(bool choice) {
    validate_transaction(choice);
    ui_menu_main();
}

static void review_tx_continue(void);
static void review_tx_start(void);
static void reject_tx_confirmation(void);
static void reject_tx_choice(void);

static void review_auth_continue(void);
static void review_auth_start(void);
static void reject_auth_confirmation(void);
static void reject_auth_choice(void);

// Functions definitions
static inline void INCR_AND_CHECK_PAGE_NB(void) {
    nbPages++;
    if (nbPages >= MAX_NUMBER_OF_PAGES) {
        // TODO
        THROW(SW_BAD_STATE);
    }
}

static void prepare_tx_pages_infos(void) {
    PRINTF("prepare_tx_pages_infos\n");
    uint8_t tagLineNb = 0;
    uint8_t tagItemLineNb = 0;
    uint8_t tagValueLineNb = 0;
    uint8_t pageLineNb = 0;
    uint16_t fieldLen = 0;
    uint8_t data_index = 0;
    reset_formatter();

    // Reset globals.
    nbPages = 0;

    explicit_bzero(pagesInfos, sizeof(pagesInfos));
    pagesInfos[0].data_idx = data_index;

    while (true) {  // Execute loop until last tx formatter is reached.
        bool data_exists = true;
        bool is_op_header = false;
        if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
            THROW(SW_FORMATTING_FAIL);
        };

        if (!data_exists) {
            break;
        }
        PRINTF("Page %d - Item : %s - Value : %s\n",
               nbPages,
               G.ui.detail_caption,
               G.ui.detail_value);

        // Compute number of lines filled by tag item string.
        fieldLen = strlen(G.ui.detail_caption);
        tagItemLineNb = fieldLen / TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE;
        tagItemLineNb += (fieldLen % TAG_VAL_LST_ITEM_MAX_CHAR_PER_LINE != 0) ? 1 : 0;
        // Compute number of lines filled by tag value string.
        fieldLen = strlen(G.ui.detail_value);
        tagValueLineNb = fieldLen / TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE;
        tagValueLineNb += (fieldLen % TAG_VAL_LST_VAL_MAX_CHAR_PER_LINE != 0) ? 1 : 0;
        // Add number of screen lines occupied by tag pair to total lines occupied in page.
        tagLineNb = tagValueLineNb + tagItemLineNb;
        pageLineNb += tagLineNb;
        // If there are multiple operations and a new operation is reached, create a
        // special page with only one caption/value pair to display operation number.
        if (is_op_header && G_context.envelope.tx_details.tx.operations_count > 1) {
            INCR_AND_CHECK_PAGE_NB();
            pagesInfos[nbPages].pagePairNb = 1;
            pagesInfos[nbPages].data_idx = data_index;
            pagesInfos[nbPages].centered_info = true;
            INCR_AND_CHECK_PAGE_NB();
            pageLineNb = 0;
            pagesInfos[nbPages].pagePairNb = 0;
            pagesInfos[nbPages].data_idx = data_index + 1;
        }
        // Else if number of lines occupied on page > allowed max number of lines per page,
        // go to next page.
        else if (pageLineNb > TAG_VAL_LST_MAX_LINES_PER_PAGE) {
            INCR_AND_CHECK_PAGE_NB();
            pageLineNb = tagLineNb;
            pagesInfos[nbPages].pagePairNb = 1;
            pagesInfos[nbPages].data_idx = data_index;
        } else
        // Otherwise save number of pairs on current page
        {
            pagesInfos[nbPages].pagePairNb++;
        }
        data_index++;
    }

    INCR_AND_CHECK_PAGE_NB();

    for (uint8_t i = 0; i < nbPages; i++) {
        PRINTF("Page %d - PairNb : %d - DataIdx : %d\n",
               i,
               pagesInfos[i].pagePairNb,
               pagesInfos[i].data_idx);
    }
}

static void prepare_page(uint8_t page) {
    PRINTF("prepare_page, page: %d\n", page);
    reset_formatter();
    uint8_t data_start_index = pagesInfos[page].data_idx;
    bool data_exists = true;
    bool is_op_header = false;

    for (uint8_t i = 0; i < data_start_index; i++) {
        if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
            THROW(SW_FORMATTING_FAIL);
        };
    }

    for (uint8_t i = 0; i < pagesInfos[page].pagePairNb; i++) {
        if (!get_next_data(&formatter_data, true, &data_exists, &is_op_header)) {
            THROW(SW_FORMATTING_FAIL);
        };
        strncpy(str_captions[i], G.ui.detail_caption, sizeof(str_captions[i]));
        strncpy(str_values[i], G.ui.detail_value, sizeof(str_values[i]));
        caption_value_pairs[i].item = str_captions[i];
        caption_value_pairs[i].value = str_values[i];
    }
}

static bool display_transaction_page(uint8_t page, nbgl_pageContent_t *content) {
    PRINTF("display_transaction_page, page: %d\n", page);
    currentPage = page;
    if (page < nbPages) {
        prepare_page(page);
        if (pagesInfos[page].centered_info) {
            content->type = CENTERED_INFO;
            content->centeredInfo.style = LARGE_CASE_INFO;
            content->centeredInfo.text1 = "Please review";
            content->centeredInfo.text2 = caption_value_pairs[0].item;
            content->centeredInfo.text3 = NULL;
            content->centeredInfo.icon = &C_icon_stellar_64px;
            content->centeredInfo.offsetY = 35;
            content->centeredInfo.onTop = false;
        } else {
            content->type = TAG_VALUE_LIST;
            content->tagValueList.nbPairs = pagesInfos[page].pagePairNb;
            content->tagValueList.pairs = (nbgl_layoutTagValue_t *) &caption_value_pairs;
            content->tagValueList.smallCaseForValue = false;
            content->tagValueList.wrapping = true;
            content->tagValueList.nbMaxLinesForValue = 0;
        }
    } else {
        if (formatter_data.envelope->type == ENVELOPE_TYPE_SOROBAN_AUTHORIZATION) {
            content->infoLongPress.text = "Sign Soroban Auth?";
        } else {
            content->infoLongPress.text = "Sign transaction?";
        }
        content->type = INFO_LONG_PRESS, content->infoLongPress.icon = &C_icon_stellar_64px;
        content->infoLongPress.longPressText = "Hold to sign";
    }
    return true;
}

static void reject_tx_confirmation(void) {
    ui_action_validate_transaction(false);
    nbgl_useCaseStatus("Transaction\nRejected", false, ui_menu_main);
}

static void reject_tx_choice(void) {
    nbgl_useCaseConfirm("Reject transaction?",
                        NULL,
                        "Yes, Reject",
                        "Go back to transaction",
                        reject_tx_confirmation);
}

static void review_tx_choice(bool confirm) {
    if (confirm) {
        ui_action_validate_transaction(true);
        nbgl_useCaseStatus("TRANSACTION\nSIGNED", true, ui_menu_main);
    } else {
        reject_tx_choice();
    }
}

static void review_tx_continue(void) {
    nbgl_useCaseRegularReview(currentPage,
                              nbPages + 1,
                              "Reject transaction",
                              NULL,
                              display_transaction_page,
                              review_tx_choice);
}

static void review_tx_start(void) {
    nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                            "Review transaction",
                            NULL,
                            "Reject transaction",
                            review_tx_continue,
                            reject_tx_choice);
}

static void reject_auth_confirmation(void) {
    ui_action_validate_transaction(false);
    nbgl_useCaseStatus("Soroban Auth\nRejected", false, ui_menu_main);
}

static void reject_auth_choice(void) {
    nbgl_useCaseConfirm("Reject Soroban Auth?",
                        NULL,
                        "Yes, Reject",
                        "Go back to Soroban Auth",
                        reject_auth_confirmation);
}

static void review_auth_choice(bool confirm) {
    if (confirm) {
        ui_action_validate_transaction(true);
        nbgl_useCaseStatus("SOROBAN AUTH\nSIGNED", true, ui_menu_main);
    } else {
        reject_auth_choice();
    }
}

static void review_auth_continue(void) {
    nbgl_useCaseRegularReview(currentPage,
                              nbPages + 1,
                              "Reject Soroban Auth",
                              NULL,
                              display_transaction_page,
                              review_auth_choice);
}

static void review_auth_start(void) {
    nbgl_useCaseReviewStart(&C_icon_stellar_64px,
                            "Review Soroban Auth",
                            NULL,
                            "Reject Soroban Auth",
                            review_auth_continue,
                            reject_auth_choice);
}

void prepare_display() {
    formatter_data_t fdata = {
        .raw_data = G_context.raw,
        .raw_data_len = G_context.raw_size,
        .envelope = &G_context.envelope,
        .caption = G.ui.detail_caption,
        .value = G.ui.detail_value,
        .signing_key = G_context.raw_public_key,
        .caption_len = DETAIL_CAPTION_MAX_LENGTH,
        .value_len = DETAIL_VALUE_MAX_LENGTH,
        .display_sequence = HAS_SETTING(S_SEQUENCE_NUMBER_ENABLED),
        .plugin_check_presence = &plugin_check_presence,
        .plugin_init_contract = &plugin_init_contract,
        .plugin_query_data_pair_count = &plugin_query_data_pair_count,
        .plugin_query_data_pair = &plugin_query_data_pair,
    };

    // init formatter_data
    memcpy(&formatter_data, &fdata, sizeof(formatter_data_t));

    currentPage = 0;
    prepare_tx_pages_infos();
}

int ui_display_transaction(void) {
    if (G_context.req_type != CONFIRM_TRANSACTION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }

    prepare_display();
    review_tx_start();
    return 0;
}

int ui_display_auth() {
    if (G_context.req_type != CONFIRM_SOROBAN_AUTHORATION || G_context.state != STATE_PARSED) {
        G_context.state = STATE_NONE;
        return io_send_sw(SW_BAD_STATE);
    }
    prepare_display();
    review_auth_start();
    return 0;
}
#endif  // HAVE_NBGL
