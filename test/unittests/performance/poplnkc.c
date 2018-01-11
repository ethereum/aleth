#include <stdio.h>
#include <stdint.h>
int main() {
	struct link {
		struct link* next;
		long long slot[8];
	} stack;
	struct link* sp = &stack;
	struct link* free = &stack;
	struct link* tmp = 0;
	stack.next = &stack;
	for (uint64_t i = 0; i < 8388608; ++i) {

		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
		tmp = sp->next, sp->next = free, free = sp, sp = tmp;
	}
}
