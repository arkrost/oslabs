#include "rope.h"
#include <stdio.h>
#define BUF_SIZE 5

static void write_reversed_rope(struct rope* rope) {
	if (rope == NULL) return;
	char buf[rope->size + 1];
	rope_dump_reversed(rope, buf);
	buf[rope->size] = buf[0];
	write(1, buf + 1, rope->size);
}


int main() 
{
	struct rope* r = NULL;
	int used = 0;
	int skip = 0;
	char* old = NULL;
	while(1) {
		char* buf = (char*)malloc(BUF_SIZE * sizeof(char));
		int n = read(0, buf, BUF_SIZE);
		if (n < 0) return n;
		if (n == 0) {
			if(r) rope_delete(r);
			free(old);
			free(buf);
			return 0;
		}
		if(r) rope_append(r, buf, n); else	r = rope_new(buf, n);
		int pos = 0, cur = r->size - n;
		while (1) {
			for (; pos < n && buf[pos] != '\n'; pos++);
			// printf("pos %d\n", pos);
			if (pos < n) {
				pos++;
				// printf("cur + pos: %d\n", cur + pos);
				struct rope* tail = rope_split(r, cur + pos);
				if (skip) skip = 0;	else write_reversed_rope(r);
				rope_delete(r);
				r = tail;
				cur = -pos;
			} else {
				if (r && r->size >= BUF_SIZE) {
					rope_delete(r);
					r = NULL;
					skip = 1;
				}
				free(old);
				old = buf;
				break;
			}
		}
	}
}