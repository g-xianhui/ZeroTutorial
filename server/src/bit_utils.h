#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

size_t write_7bit_encoded_int(uint32_t value, char bytes[5]);
uint32_t read_7bit_encoded_int(const char* bytes, int* out_pos=nullptr);