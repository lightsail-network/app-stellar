#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "stellar/parser.h"
#include "stellar/formatter.h"

/*
 * Captions don't scroll so there is no use in having more capacity than can fit on screen at once.
 */
#define DETAIL_CAPTION_MAX_LENGTH 20

/*
 * DETAIL_VALUE_MAX_LENGTH value of 89 is due to the maximum length of managed data value which can
 * be 64 bytes long. Managed data values are displayed as base64 encoded strings, which are
 * 4*((len+2)/3) characters long. (An additional slot is required for the end-of-string character of
 * course)
 */
#define DETAIL_VALUE_MAX_LENGTH 89

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    envelope_t envelope;
    memset(&envelope, 0, sizeof(envelope_t));
    if (!parse_transaction_envelope(data, size, &envelope)) {
        return 0;
    }

    // char detail_caption[DETAIL_CAPTION_MAX_LENGTH];
    // char detail_value[DETAIL_VALUE_MAX_LENGTH];

    // uint8_t signing_key[] = {0xe9, 0x33, 0x88, 0xbb, 0xfd, 0x2f, 0xbd, 0x11, 0x80, 0x6d, 0xd0,
    //                          0xbd, 0x59, 0xce, 0xa9, 0x7,  0x9e, 0x7c, 0xc7, 0xc,  0xe7, 0xb1,
    //                          0xe1, 0x54, 0xf1, 0x14, 0xcd, 0xfe, 0x4e, 0x46, 0x6e, 0xcd};

    // formatter_data_t fdata = {
    //     .raw_data = data,
    //     .raw_data_len = size,
    //     .envelope = &envelope,
    //     .caption = detail_caption,
    //     .value = detail_value,
    //     .signing_key = signing_key,
    //     .caption_len = DETAIL_CAPTION_MAX_LENGTH,
    //     .value_len = DETAIL_VALUE_MAX_LENGTH,
    //     .display_sequence = true,
    // };

    // reset_formatter();

    // while (true) {
    //     bool data_exists = true;
    //     bool is_op_header = false;
    //     if (!get_next_data(&fdata, true, &data_exists, &is_op_header)) {
    //         return 0;
    //     }

    //     if (!data_exists) {
    //         break;
    //     }
    // }
    return 0;
}
