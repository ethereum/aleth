#include <stdio.h>
#include <stdint.h>
int main() {
	uint64_t r;	
	for (uint64_t i = 0; i < 1048576; ++i) {
		uint64_t z = 0;
		uint64_t y = 64;
		uint64_t x = 0;				
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x = z;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		x += y; x += y; x += y; x += y; x += y; x += y; x += y; x += y;
		r = x;
	}
	printf(r == 1024 ? "success\n" : "failure\n");
}
