#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool base64_encode(const uint8_t *data, size_t in_len, char *out, size_t out_len);
