//
// Created by servostar on 01.08.24.
//

#include <math/api.h>
#include <capi.h>

void mod(u32 a, u32 b, u32* c) {
    c[0] = a % b;
}
