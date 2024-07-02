//
// Created by servostar on 6/10/24.
//

#include <stdio.h>

extern void swap(float*, float*);

int main(int argc, char* argv[]) {
    float a = 2.0;
    float b = 7.0;

    swap(&a, &b);

    printf("%f, %f\n", a, b);

    return 0;
}
