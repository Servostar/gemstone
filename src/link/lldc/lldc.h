//
// Created by servostar on 8/11/24.
//

#ifndef LLDC_H
#define LLDC_H

#include <link/driver.h>

bool lldc_link(TargetConfig* target_config, TargetLinkConfig* link_config);

BinaryDriver* lldc_get_driver();

#endif //LLDC_H
