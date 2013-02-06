#include <string.h>
#define BUF_SIZE 5

void reverse(char* buf, int size) {
    int i;
    for (i = 0; i < size / 2; i++) {
        char tmp = buf[i];
        buf[i] = buf[size - i - 1];
        buf[size - i - 1] = tmp;
    }
}

int main() {
    char buf[BUF_SIZE];
    int used = 0;
    char skip = 0;
    while(1) {
        int n = read(0, buf + used, BUF_SIZE - used);
        if (n < 0) return n;
        used += n;

        int pos;
        for (pos = 0; pos < used && buf[pos] != '\n'; pos++);

        if (pos < used) {
            if (skip) {
                skip = 0;
            } else {
                reverse(buf, pos);
                write(1, buf, pos + 1);
            }
            used -= (pos + 1);
            memmove(buf, buf + pos + 1, used);
        } else if (pos == BUF_SIZE) {
            skip = 1;
            used = 0;
        } else if (n == 0) {
            return 0;
        }
    }
}