//
// Created by servostar on 6/4/24.
//

#ifndef LLVM_BACKEND_LLD_H
#define LLVM_BACKEND_LLD_H

#include <codegen/backend.h>
#include <llvm/backend.h>

TargetLinkConfig* lld_create_link_config(__attribute__((unused))
                                         const Target* target,
                                         const TargetConfig* target_config,
                                         const Module* module);

BackendError lld_link_target(TargetLinkConfig* config);

void lld_delete_link_config(TargetLinkConfig* config);

#endif // LLVM_BACKEND_LLD_H
