//
// Created by servostar on 6/10/24.
//

#include <stdio.h>

extern void fib(int*, int);

int main(int argc, char* argv[]) {

    for (int i = 0; i < 7; i++) {
        int r = 0;
        fib(&r, i);
        printf("%d\n", r);
    }

    return 0;
}
