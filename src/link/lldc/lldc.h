//
// Created by servostar on 8/11/24.
//

#ifndef LLDC_H
#define LLDC_H

#include <link/driver.h>

bool lldc_link(TargetLinkConfig* config);

BinaryDriver* lldc_get_driver();

#endif //LLDC_H
