#include <stdio.h>
#include <string.h>
#include <time.h>
#include "base32.h"
#include "format.h"

#include "stellar/printer.h"

#define INT256_LENGTH                     32
#define MUXED_ACCOUNT_MED_25519_SIZE      43
#define BINARY_MAX_SIZE                   36
#define AMOUNT_WITH_COMMAS_MAX_LENGTH     24   // 922,337,203,685.4775807
#define ED25519_SIGNED_PAYLOAD_MAX_LENGTH 166  // include the null terminator
#define NUMBER_WITH_COMMAS_MAX_LENGTH     105

uint16_t crc16(const uint8_t *input_str, int num_bytes) {
    uint16_t crc;
    crc = 0;
    while (--num_bytes >= 0) {
        crc = crc ^ (uint32_t) *input_str++ << 8;
        int i = 8;
        do {
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
        } while (--i);
    }
    return crc;
}

bool encode_key(const uint8_t *in, uint8_t version_byte, char *out, uint8_t out_len) {
    if (in == NULL || out_len < 56 + 1) {
        return false;
    }
    uint8_t buffer[35] = {0};
    buffer[0] = version_byte;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i + 1] = in[i];
    }
    uint16_t crc = crc16(buffer, 33);  // checksum
    buffer[33] = crc;
    buffer[34] = crc >> 8;
    if (base32_encode(buffer, 35, (uint8_t *) out, 56) == -1) {
        return false;
    }
    out[56] = '\0';
    return true;
}

bool encode_ed25519_public_key(const uint8_t raw_public_key[static RAW_ED25519_PUBLIC_KEY_SIZE],
                               char *out,
                               size_t out_len) {
    return encode_key(raw_public_key, VERSION_BYTE_ED25519_PUBLIC_KEY, out, out_len);
}

bool encode_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                       char *out,
                       size_t out_len) {
    return encode_key(raw_hash_x, VERSION_BYTE_HASH_X, out, out_len);
}

bool encode_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                           char *out,
                           size_t out_len) {
    return encode_key(raw_pre_auth_tx, VERSION_BYTE_PRE_AUTH_TX_KEY, out, out_len);
}

bool encode_contract(const uint8_t raw_contract[static RAW_CONTRACT_KEY_SIZE],
                     char *out,
                     size_t out_len) {
    return encode_key(raw_contract, VERSION_BYTE_CONTRACT, out, out_len);
}

bool encode_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                   char *out,
                                   size_t out_len) {
    if (out_len < ED25519_SIGNED_PAYLOAD_MAX_LENGTH) {  // (103 * 8 + 4) / 5
        return false;
    }
    if (signed_payload->payload_len > 64 || signed_payload->payload_len <= 0) {
        return false;
    }
    uint8_t data_len = RAW_ED25519_PUBLIC_KEY_SIZE + 4 + signed_payload->payload_len +
                       ((4 - signed_payload->payload_len % 4) % 4);
    uint8_t buffer[1 + 32 + 4 + 64 + 2] = {0};
    buffer[0] = VERSION_BYTE_ED25519_SIGNED_PAYLOAD;
    for (uint8_t i = 0; i < 32; i++) {
        buffer[i + 1] = signed_payload->ed25519[i];
    }
    buffer[36] = signed_payload->payload_len;
    for (uint8_t i = 0; i < signed_payload->payload_len; i++) {
        buffer[i + 37] = signed_payload->payload[i];
    }
    uint16_t crc = crc16(buffer, data_len + 1);  // checksum
    buffer[1 + data_len] = crc;
    buffer[1 + data_len + 1] = crc >> 8;
    int ret = base32_encode(buffer, data_len + 3, (uint8_t *) out, out_len);
    if (ret == -1) {
        return false;
    }
    out[ret] = '\0';
    return true;
}

bool encode_muxed_account(const muxed_account_t *raw_muxed_account, char *out, size_t out_len) {
    if (raw_muxed_account == NULL) {
        return false;
    }
    if (raw_muxed_account->type == KEY_TYPE_ED25519) {
        return encode_ed25519_public_key(raw_muxed_account->ed25519, out, out_len);
    } else {
        if (out_len < ENCODED_MUXED_ACCOUNT_KEY_LENGTH) {
            return false;
        }
        uint8_t buffer[MUXED_ACCOUNT_MED_25519_SIZE] = {0};
        buffer[0] = VERSION_BYTE_MUXED_ACCOUNT;
        memcpy(buffer + 1, raw_muxed_account->med25519.ed25519, RAW_ED25519_PUBLIC_KEY_SIZE);
        for (int i = 0; i < 8; i++) {
            buffer[33 + i] = raw_muxed_account->med25519.id >> 8 * (7 - i);
        }
        uint16_t crc = crc16(buffer, MUXED_ACCOUNT_MED_25519_SIZE - 2);  // checksum
        buffer[41] = crc;
        buffer[42] = crc >> 8;
        if (base32_encode(buffer,
                          MUXED_ACCOUNT_MED_25519_SIZE,
                          (uint8_t *) out,
                          ENCODED_MUXED_ACCOUNT_KEY_LENGTH) == -1) {
            return false;
        }
        out[69] = '\0';
        return true;
    }
}

bool print_summary(const char *in,
                   char *out,
                   size_t out_len,
                   uint8_t num_chars_l,
                   uint8_t num_chars_r) {
    uint8_t result_len = num_chars_l + num_chars_r + 2;
    if (out_len < result_len + 1) {
        return false;
    }
    uint16_t in_len = strlen(in);
    if (in_len > result_len) {
        memcpy(out, in, num_chars_l);
        out[num_chars_l] = '.';
        out[num_chars_l + 1] = '.';
        memcpy(out + num_chars_l + 2, in + in_len - num_chars_r, num_chars_r);
        out[result_len] = '\0';
    } else {
        memcpy(out, in, in_len);
        out[in_len] = '\0';
    }
    return true;
}

bool print_binary(const uint8_t *in,
                  size_t in_len,
                  char *out,
                  size_t out_len,
                  uint8_t num_chars_l,
                  uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[BINARY_MAX_SIZE * 2 + 1];
        if (!format_hex(in, in_len, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return format_hex(in, in_len, out, out_len);
}

bool print_account_id(const account_id_t account_id,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_ED25519_PUBLIC_KEY_LENGTH];
        if (!encode_ed25519_public_key(account_id, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_ed25519_public_key(account_id, out, out_len);
}

bool print_contract_id(const uint8_t *contract_id,
                       char *out,
                       size_t out_len,
                       uint8_t num_chars_l,
                       uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_CONTRACT_KEY_LENGTH];
        if (!encode_contract(contract_id, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_contract(contract_id, out, out_len);
}

bool print_hash_x_key(const uint8_t raw_hash_x[static RAW_HASH_X_KEY_SIZE],
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_HASH_X_KEY_LENGTH];
        if (!encode_hash_x_key(raw_hash_x, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_hash_x_key(raw_hash_x, out, out_len);
}

bool print_pre_auth_x_key(const uint8_t raw_pre_auth_tx[static RAW_PRE_AUTH_TX_KEY_SIZE],
                          char *out,
                          size_t out_len,
                          uint8_t num_chars_l,
                          uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_PRE_AUTH_TX_KEY_LENGTH];
        if (!encode_pre_auth_x_key(raw_pre_auth_tx, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_pre_auth_x_key(raw_pre_auth_tx, out, out_len);
}

bool print_ed25519_signed_payload(const ed25519_signed_payload_t *signed_payload,
                                  char *out,
                                  size_t out_len,
                                  uint8_t num_chars_l,
                                  uint8_t num_chars_r) {
    // TODO: calculate the exact length
    char tmp[ED25519_SIGNED_PAYLOAD_MAX_LENGTH];
    if (!encode_ed25519_signed_payload(signed_payload, tmp, sizeof(tmp))) {
        return false;
    };

    if (num_chars_l > 0) {
        if (out_len < num_chars_l + num_chars_r + 2) {
            return false;
        }
        return print_summary(tmp, out, out_len, num_chars_l, num_chars_r);
    } else {
        if (out_len < ED25519_SIGNED_PAYLOAD_MAX_LENGTH) {
            return false;
        }
        if (strlcpy(out, tmp, out_len) >= out_len) {
            return false;
        }
    }
    return true;
}

bool print_sc_address(const sc_address_t *sc_address,
                      char *out,
                      size_t out_len,
                      uint8_t num_chars_l,
                      uint8_t num_chars_r) {
    if (sc_address->type == SC_ADDRESS_TYPE_ACCOUNT) {
        return print_account_id(sc_address->address, out, out_len, num_chars_l, num_chars_r);
    } else {
        return print_contract_id(sc_address->address, out, out_len, num_chars_l, num_chars_r);
    }
    return true;
}

bool print_muxed_account(const muxed_account_t *muxed_account,
                         char *out,
                         size_t out_len,
                         uint8_t num_chars_l,
                         uint8_t num_chars_r) {
    if (num_chars_l > 0) {
        char buffer[ENCODED_MUXED_ACCOUNT_KEY_LENGTH];
        if (!encode_muxed_account(muxed_account, buffer, sizeof(buffer))) {
            return false;
        }
        return print_summary(buffer, out, out_len, num_chars_l, num_chars_r);
    }
    return encode_muxed_account(muxed_account, out, out_len);
}

bool print_claimable_balance_id(const claimable_balance_id_t *claimable_balance_id_t,
                                char *out,
                                size_t out_len,
                                uint8_t num_chars_l,
                                uint8_t num_chars_r) {
    if (out_len < 36 * 2 + 1) {
        return false;
    }
    uint8_t data[36];
    // enum is 1 byte
    data[0] = '\0';
    data[1] = '\0';
    data[2] = '\0';
    data[3] = claimable_balance_id_t->type;
    memcpy(data + 4, claimable_balance_id_t->v0, CLAIMABLE_BALANCE_ID_SIZE);
    return print_binary(data, 36, out, out_len, num_chars_l, num_chars_r);
}

bool print_uint64_num(uint64_t num, char *out, size_t out_len) {
    uint8_t data[8] = {0};
    for (int i = 0; i < 8; i++) {
        data[i] = num >> (8 * (7 - i));
    }
    return print_uint64(data, 0, out, out_len, false);
}

bool print_int64_num(int64_t num, char *out, size_t out_len) {
    uint8_t data[8] = {0};
    for (int i = 0; i < 8; i++) {
        data[i] = num >> (8 * (7 - i));
    }
    return print_int64(data, 0, out, out_len, false);
}

bool print_time(uint64_t seconds, char *out, size_t out_len) {
    if (seconds > 253402300799) {
        // valid range 1970-01-01 00:00:00 - 9999-12-31 23:59:59
        return false;
    }
    char time_str[20] = {0};  // 1970-01-01 00:00:00

    if (out_len < sizeof(time_str)) {
        return false;
    }
    struct tm tm;
    if (!gmtime_r((time_t *) &seconds, &tm)) {
        return false;
    };

    if (snprintf(time_str,
                 sizeof(time_str),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 tm.tm_year + 1900,
                 tm.tm_mon + 1,
                 tm.tm_mday,
                 tm.tm_hour,
                 tm.tm_min,
                 tm.tm_sec) < 0) {
        return false;
    };
    if (strlcpy(out, time_str, out_len) >= out_len) {
        return false;
    }
    return true;
}

bool print_asset_name(const asset_t *asset, uint8_t network_id, char *out, size_t out_len) {
    switch (asset->type) {
        case ASSET_TYPE_NATIVE:
            if (network_id == NETWORK_TYPE_UNKNOWN) {
                if (strlcpy(out, "native", out_len) >= out_len) {
                    return false;
                }
            } else {
                if (strlcpy(out, "XLM", out_len) >= out_len) {
                    return false;
                }
            }
            return true;
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            for (int i = 0; i < 4; i++) {
                out[i] = asset->alpha_num4.asset_code[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[4] = 0;
            return true;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            for (int i = 0; i < 12; i++) {
                out[i] = asset->alpha_num12.asset_code[i];
                if (out[i] == 0) {
                    break;
                }
            }
            out[12] = 0;
            return true;
        default:
            return false;
    }
}

bool print_asset(const asset_t *asset, uint8_t network_id, char *out, size_t out_len) {
    char asset_code[12 + 1];
    char asset_issuer[3 + 2 + 4 + 1];
    print_asset_name(asset, network_id, asset_code, sizeof(asset_code));

    switch (asset->type) {
        case ASSET_TYPE_CREDIT_ALPHANUM4:
            print_account_id(asset->alpha_num4.issuer, asset_issuer, sizeof(asset_issuer), 3, 4);
            break;
        case ASSET_TYPE_CREDIT_ALPHANUM12:
            print_account_id(asset->alpha_num12.issuer, asset_issuer, sizeof(asset_issuer), 3, 4);
            break;
        default:
            break;
    }
    if (strlcpy(out, asset_code, out_len) >= out_len) {
        return false;
    }
    if (asset->type != ASSET_TYPE_NATIVE) {
        strlcat(out, "@", out_len);
        strlcat(out, asset_issuer, out_len);
    }
    return true;
}

bool print_flag(const char *flag, char *out, size_t out_len) {
    if (out[0]) {
        if (strlcat(out, ", ", out_len) >= out_len) {
            return false;
        }
    }
    if (strlcat(out, flag, out_len) >= out_len) {
        return false;
    }
    return true;
}

bool print_account_flags(uint32_t flags, char *out, size_t out_len) {
    explicit_bzero(out, out_len);
    if (flags & 0x01u) {
        if (!print_flag("AUTH_REQUIRED", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x02u) {
        if (!print_flag("AUTH_REVOCABLE", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x04u) {
        if (!print_flag("AUTH_IMMUTABLE", out, out_len)) {
            return false;
        }
    }
    if (flags & 0x08u) {
        if (!print_flag("AUTH_CLAWBACK_ENABLED", out, out_len)) {
            return false;
        }
    }
    return true;
}

bool print_trust_line_flags(uint32_t flags, char *out, size_t out_len) {
    explicit_bzero(out, out_len);
    if (flags & AUTHORIZED_FLAG) {
        if (!print_flag("AUTHORIZED", out, out_len)) {
            return false;
        }
    }
    if (flags & AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        if (!print_flag("AUTHORIZED_TO_MAINTAIN_LIABILITIES", out, out_len)) {
            return false;
        }
    }
    if (flags & TRUSTLINE_CLAWBACK_ENABLED_FLAG) {
        if (!print_flag("TRUSTLINE_CLAWBACK_ENABLED", out, out_len)) {
            return false;
        }
    }
    return true;
}

bool print_allow_trust_flags(uint32_t flag, char *out, size_t out_len) {
    explicit_bzero(out, out_len);
    if (flag & AUTHORIZED_FLAG) {
        if (strlcpy(out, "AUTHORIZED", out_len) >= out_len) {
            return false;
        }
    } else if (flag & AUTHORIZED_TO_MAINTAIN_LIABILITIES_FLAG) {
        if (strlcpy(out, "AUTHORIZED_TO_MAINTAIN_LIABILITIES", out_len) >= out_len) {
            return false;
        }
    } else {
        if (strlcpy(out, "UNAUTHORIZED", out_len) >= out_len) {
            return false;
        }
    }
    return true;
}

bool print_amount(uint64_t amount,
                  const asset_t *asset,
                  uint8_t network_id,
                  char *out,
                  size_t out_len) {
    uint8_t data[8] = {0};
    for (int i = 0; i < 8; i++) {
        data[i] = amount >> (8 * (7 - i));
    }

    if (!print_uint64(data, 7, out, out_len, true)) {
        return false;
    }

    char asset_info[23];  // BANANANANANA@GBD..KHK4, 12 + 1 + 3 + 2 + 4 = 22

    if (asset) {
        // qualify amount
        if (!print_asset(asset, network_id, asset_info, 23)) {
            return false;
        };
        strlcat(out, " ", out_len);
        strlcat(out, asset_info, out_len);
    }
    return true;
}

bool is_printable_binary(const uint8_t *str, size_t str_len) {
    for (size_t i = 0; i < str_len; i++) {
        if (str[i] > 0x7e || str[i] < 0x20) {
            return false;
        }
    }
    return true;
}

static int allzeroes(const void *buf, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    for (size_t i = 0; i < n; ++i) {
        if (p[i]) {
            return 0;
        }
    }
    return 1;
}

static bool uint256_to_decimal(const uint8_t *value, size_t value_len, char *out, size_t out_len) {
    if (value_len > INT256_LENGTH) {
        return false;
    }

    uint16_t n[16] = {0};
    // Copy and right-align the number
    memcpy((uint8_t *) n + INT256_LENGTH - value_len, value, value_len);

    // Special case when value is 0
    if (allzeroes(n, INT256_LENGTH)) {
        if (out_len < 2) {
            // Not enough space to hold "0" and \0.
            return false;
        }
        if (strlcpy(out, "0", out_len) >= out_len) {
            return false;
        }
        return true;
    }

    uint16_t *p = n;
    for (int i = 0; i < 16; i++) {
        n[i] = __builtin_bswap16(*p++);
    }
    int pos = out_len;
    size_t result_len = 0;
    while (!allzeroes(n, sizeof(n))) {
        if (pos == 0) {
            return false;
        }
        pos -= 1;
        result_len += 1;
        unsigned int carry = 0;
        for (int i = 0; i < 16; i++) {
            int rem = ((carry << 16) | n[i]) % 10;
            n[i] = ((carry << 16) | n[i]) / 10;
            carry = rem;
        }
        out[pos] = '0' + carry;
    }
    if (out_len < result_len + 1) {
        // Not enough space to hold the result and \0.
        return false;
    }
    memmove(out, out + pos, out_len - pos);
    out[out_len - pos] = 0;
    return true;
}

static bool int256_to_decimal(const uint8_t *value, size_t value_len, char *out, size_t out_len) {
    if (value_len > INT256_LENGTH) {
        // value len is bigger than INT256_LENGTH
        return false;
    }

    bool is_negative = (value[0] & 0x80) != 0;
    uint8_t n[INT256_LENGTH] = {0};
    if (is_negative) {
        // Compute the absolute value
        bool carry = true;
        for (int i = value_len - 1; i >= 0; --i) {
            n[INT256_LENGTH - value_len + i] = ~value[i] + (carry ? 1 : 0);
            carry = carry && (value[i] == 0);
        }
    } else {
        memcpy(n + INT256_LENGTH - value_len, value, value_len);
    }

    size_t result_len = 0;
    char *p = out + out_len - 1;
    *p = '\0';
    do {
        uint32_t remainder = 0;
        for (int i = 0; i < INT256_LENGTH; ++i) {
            uint32_t temp = (remainder << 8) | n[i];
            n[i] = temp / 10;
            remainder = temp % 10;
        }
        --p;
        result_len++;
        if (p < out) {
            // Not enough space in the output buffer
            return false;
        }
        *p = '0' + remainder;
    } while (!allzeroes(n, INT256_LENGTH));

    if (is_negative) {
        --p;
        result_len++;
        if (p < out) {
            // Not enough space in the output buffer
            return false;
        }
        *p = '-';
    }

    if (out_len < result_len + 1) {
        // Not enough space to hold the result and \0.
        return false;
    }

    memmove(out, p, out_len - (p - out));
    return true;
}

bool add_decimal_point(char *out, size_t out_len, uint8_t decimals) {
    // if (out == NULL || out_len == 0) {
    //     return false;
    // }
    // if (decimals == 0) {
    //     return true;
    // }

    // bool is_negative = out[0] == '-';
    // if (is_negative) {
    //     if (decimals >= out_len - 2) {
    //         // Not enough space to add decimal point and leading zero.
    //         return false;
    //     }
    // } else {
    //     if (decimals >= out_len - 1) {
    //         // Not enough space to add decimal point.
    //         return false;
    //     }
    // }

    // char *start = is_negative ? out + 1 : out;

    // size_t len = strlen(start);
    // if (len == 0) {
    //     return true;
    // }

    // if (len <= decimals) {
    //     memmove(start + decimals - len + 2, start, len + 1);
    //     start[0] = '0';
    //     start[1] = '.';
    //     for (size_t i = 2; i < decimals - len + 2; ++i) {
    //         start[i] = '0';
    //     }
    // } else {
    //     memmove(start + len - decimals + 1, start + len - decimals, decimals + 1);
    //     start[len - decimals] = '.';
    // }

    // // Remove trailing zeros after decimal point
    // char *p = start + strlen(start) - 1;
    // while (p > start && *p == '0') {
    //     *p-- = '\0';
    // }

    // // Remove decimal point if it's the last character
    // if (p > start && *p == '.') {
    //     *p = '\0';
    // }

    // if (is_negative && out[0] != '-') {
    //     memmove(out + 1, out, strlen(out) + 1);
    //     out[0] = '-';
    // }

    return true;
}

bool add_separator_to_number(char *out, size_t out_len) {
    // if (out == NULL || out_len == 0) {
    //     return false;
    // }

    // size_t length = strlen(out);
    // uint8_t negative = (out[0] == '-') ? 1 : 0;  // Check if the number is negative

    // // Find the position of the decimal point
    // char *decimal_point = strchr(out, '.');
    // size_t decimal_index = decimal_point ? decimal_point - out : length;

    // // Calculate the new length of the string with the commas
    // size_t new_length = 0;
    // if (negative) {
    //     if (decimal_index < 2) {
    //         // The string is too short to have a negative number
    //         return false;
    //     }
    //     new_length = decimal_index + (decimal_index - 2) / 3;
    // } else {
    //     if (decimal_index < 1) {
    //         // The string is too short to have a positive number
    //         return false;
    //     }
    //     new_length = decimal_index + (decimal_index - 1) / 3;
    // }

    // // If the new length is greater than the maximum length, return false
    // if (new_length >= out_len || new_length >= NUMBER_WITH_COMMAS_MAX_LENGTH) {
    //     return false;
    // }

    // char temp[NUMBER_WITH_COMMAS_MAX_LENGTH];
    // if (strlcpy(temp, out, NUMBER_WITH_COMMAS_MAX_LENGTH) >= NUMBER_WITH_COMMAS_MAX_LENGTH) {
    //     return false;
    // }

    // temp[new_length] = '\0';  // Set the end of the new string

    // // Start from the end of the string and move the digits to their new positions
    // for (int i = decimal_index - 1, j = new_length - 1; i >= 0; i--, j--) {
    //     temp[j] = out[i];

    //     // If the current position is a multiple of 3 and it's not the first digit, add a comma
    //     if ((decimal_index - i) % 3 == 0 && i != negative && j > 0) {
    //         temp[--j] = ',';
    //     }
    // }

    // // If there is a decimal point, append the part after the decimal point
    // if (decimal_point) {
    //     strlcpy(temp + new_length, decimal_point, NUMBER_WITH_COMMAS_MAX_LENGTH - new_length);
    // }

    // if (strlcpy(out, temp, out_len) >= out_len) {
    //     return false;
    // }

    return true;
}

bool print_int32(const uint8_t *value,
                 uint8_t decimals,
                 char *out,
                 size_t out_len,
                 bool add_separator) {
    return int256_to_decimal(value, 4, out, out_len) && add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_uint32(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator) {
    return uint256_to_decimal(value, 4, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_int64(const uint8_t *value,
                 uint8_t decimals,
                 char *out,
                 size_t out_len,
                 bool add_separator) {
    return int256_to_decimal(value, 8, out, out_len) && add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_uint64(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator) {
    return uint256_to_decimal(value, 8, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_int128(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator) {
    return int256_to_decimal(value, 16, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_uint128(const uint8_t *value,
                   uint8_t decimals,
                   char *out,
                   size_t out_len,
                   bool add_separator) {
    return uint256_to_decimal(value, 16, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_int256(const uint8_t *value,
                  uint8_t decimals,
                  char *out,
                  size_t out_len,
                  bool add_separator) {
    return int256_to_decimal(value, 32, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_uint256(const uint8_t *value,
                   uint8_t decimals,
                   char *out,
                   size_t out_len,
                   bool add_separator) {
    return uint256_to_decimal(value, 32, out, out_len) &&
           add_decimal_point(out, out_len, decimals) &&
           (!add_separator || add_separator_to_number(out, out_len));
}

bool print_scv_symbol(const scv_symbol_t *scv_symbol, char *out, size_t out_len) {
    if (scv_symbol == NULL || out == NULL) {
        return false;
    }
    if (scv_symbol->size > SCV_SYMBOL_MAX_SIZE || scv_symbol->size > out_len - 1) {
        return false;
    }
    if (scv_symbol->size == 0) {
        // print empty symbol
        if (strlcpy(out, "[empty symbol]", out_len) >= out_len) {
            return false;
        }
        return true;
    }
    if (!is_printable_binary(scv_symbol->symbol, scv_symbol->size)) {
        return false;
    }
    memcpy(out, scv_symbol->symbol, scv_symbol->size);
    out[scv_symbol->size] = '\0';
    return true;
}

bool print_scv_string(const scv_string_t *scv_string, char *out, size_t out_len) {
    if (scv_string == NULL || out == NULL) {
        return false;
    }

    if (scv_string->size == 0) {
        // print empty symbol
        if (strlcpy(out, "[empty string]", out_len) >= out_len) {
            return false;
        }
        return true;
    }

    // if the string is not printable, print [unprintable string]
    if (!is_printable_binary(scv_string->string, scv_string->size)) {
        if (strlcpy(out, "[unprintable string]", out_len) >= out_len) {
            return false;
        }
        return true;
    }

    size_t copy_len = scv_string->size;

    // If the output buffer is large enough to hold the entire scv_string, copy it directly and
    // append a null terminator.
    if (out_len > copy_len) {
        memcpy(out, scv_string->string, copy_len);
        out[copy_len] = '\0';  // Ensure the string is null-terminated.
    } else {
        if (out_len < 3) {
            return false;
        }
        // Calculate the lengths of the beginning and end parts that can be displayed.
        size_t dots_len = 2;                 // The length of two dots.
        size_t available_len = out_len - 1;  // Subtract 1 to reserve space for the null terminator.
        size_t start_copy_len = available_len / 2;
        size_t end_copy_len = available_len - start_copy_len - dots_len;

        // Copy the beginning part of the string.
        memcpy(out, scv_string->string, start_copy_len);
        out[start_copy_len] = '.';
        out[start_copy_len + 1] = '.';

        // Copy the end part of the string if there is space for it after the dots.
        if (end_copy_len > 0) {
            memcpy(out + start_copy_len + dots_len,
                   scv_string->string + copy_len - end_copy_len,
                   end_copy_len);
        }

        // Ensure the output string is null-terminated by placing a null character at the end of the
        // buffer.
        out[out_len - 1] = '\0';
    }

    return true;
}