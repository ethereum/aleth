#include <stdio.h>
#include <stdint.h>
int main() {
	struct link {
		struct link* next;
		long long slot[8];
	} stack[2], *sp;
	for (uint64_t i = 0; i < 8388608; ++i) {
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
		stack[0] = stack[1]; sp = stack + 1;
	}
}
