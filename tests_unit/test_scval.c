#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <cmocka.h>

#include "stellar/parser.h"

void test_parse_int64() {
    uint8_t data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6};
    buffer_t buffer = {data, sizeof(data)};
    int64_t n = 0;
    assert_true(parse_int64(&buffer, &n));
    assert_int_equal(n, -10);

    uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6};
    buffer_t buffer2 = {data2, sizeof(data2)};
    int64_t n2 = 0;
    assert_true(parse_int64(&buffer2, &n2));
    assert_int_equal(n2, 246);
}

void test_parse_uint64() {
    uint8_t data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6};
    buffer_t buffer = {data, sizeof(data)};

    uint64_t n = 0;
    assert_true(parse_uint64(&buffer, &n));
    assert_int_equal(n, 18446744073709551606);

    uint8_t data2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf6};
    buffer_t buffer2 = {data2, sizeof(data2)};
    uint64_t n2 = 0;
    assert_true(parse_uint64(&buffer2, &n2));
    assert_int_equal(n2, 246);
}

void test_scv_i128() {
    uint8_t data[] = {0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00,
                      0x00};
    buffer_t buffer = {data, sizeof(data)};
    scv_i128_t i128;
    assert_true(parse_scv_i128(&buffer, &i128));
    assert_int_equal(i128.hi, 0);
    assert_int_equal(i128.lo, 0);
    assert_int_equal(buffer.size, buffer.offset);

    uint8_t data2[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01};
    buffer_t buffer2 = {data2, sizeof(data2)};
    scv_i128_t i128_2;
    assert_true(parse_scv_i128(&buffer2, &i128_2));
    assert_int_equal(i128_2.hi, 0);
    assert_int_equal(i128_2.lo, 1);
    assert_int_equal(buffer2.size, buffer2.offset);

    uint8_t data3[] = {0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff};
    buffer_t buffer3 = {data3, sizeof(data3)};
    scv_i128_t i128_3;
    assert_true(parse_scv_i128(&buffer3, &i128_3));
    assert_int_equal(i128_3.hi, -1);
    assert_int_equal(i128_3.lo, 18446744073709551615);
    assert_int_equal(buffer3.size, buffer3.offset);

    uint8_t data4[] = {0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x01,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    buffer_t buffer4 = {data4, sizeof(data4)};
    scv_i128_t i128_4;
    assert_true(parse_scv_i128(&buffer4, &i128_4));
    assert_int_equal(i128_4.hi, 1);
    assert_int_equal(i128_4.lo, 0);
    assert_int_equal(buffer4.size, buffer4.offset);

    uint8_t data5[] = {0x7f,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff,
                       0xff};
    buffer_t buffer5 = {data5, sizeof(data5)};
    scv_i128_t i128_5;
    assert_true(parse_scv_i128(&buffer5, &i128_5));
    assert_int_equal(i128_5.hi, 9223372036854775807);
    assert_int_equal(i128_5.lo, 18446744073709551615);
    assert_int_equal(buffer5.size, buffer5.offset);

    uint8_t data6[] = {0x80,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00,
                       0x00};
    buffer_t buffer6 = {data6, sizeof(data6)};
    scv_i128_t i128_6;
    assert_true(parse_scv_i128(&buffer6, &i128_6));
    assert_int_equal(i128_6.hi, -9223372036854775808);
    assert_int_equal(i128_6.lo, 0);
    assert_int_equal(buffer6.size, buffer6.offset);
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_int64),
        cmocka_unit_test(test_parse_uint64),
        cmocka_unit_test(test_scv_i128),

    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}