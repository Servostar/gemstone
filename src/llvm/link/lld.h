//
// Created by servostar on 6/4/24.
//

#ifndef LLVM_BACKEND_LLD_H
#define LLVM_BACKEND_LLD_H

#include <codegen/backend.h>
#include <llvm/backend.h>

/**
 * @brief Link the target by its configuration to the final output.
 * @param target
 * @param config
 * @return
 */
BackendError link_target(const Target* target, const TargetConfig* config);

#endif // LLVM_BACKEND_LLD_H
