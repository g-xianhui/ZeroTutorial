#include "bit_utils.h"

size_t write_7bit_encoded_int(uint32_t value, char bytes[5])
{
	size_t i = 0;
	while (value > 0x7F)
	{
		bytes[i++] = (char)(value | ~0x7F);
		value >>= 7;
	}
	bytes[i++] = (char)value;
	return i;
}

uint32_t read_7bit_encoded_int(const char* bytes, int* out_pos)
{
	uint32_t value = 0;

	char c;
	int i = 0;
	int s = 0;
	do
	{
		c = bytes[i++];
		uint32_t x = (c & 0x7F);
		x <<= s;
		value += x;
		s += 7;
	} while (c & 0x80);

	if (out_pos)
		*out_pos = i;

	return value;
}