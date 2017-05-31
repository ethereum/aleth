#include <stdio.h>
#include <stdint.h>
int main() {
	uint64_t r;	
	for (uint64_t i = 0; i < 2000000; ++i) {
		uint64_t z = 0x54f0003fc5;
		uint64_t y = 3;
		uint64_t x = 0x54f0003fc5;				
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x = z;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y; x *= y;
		r = x;
		x = z;
	}
	printf(r == 0xd9ee6bc48e6ea405 ? "success\n\n" : "failure\n");
}
