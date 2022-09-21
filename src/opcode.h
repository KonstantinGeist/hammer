// *****************************************************************************
//
//  Copyright (c) Konstantin Geist. All rights reserved.
//
//  The use and distribution terms for this software are contained in the file
//  named License.txt, which can be found in the root of this distribution.
//  By using this software in any fashion, you are agreeing to be bound by the
//  terms of this license.
//
//  You must not remove this notice, or any other, from this software.
//
// *****************************************************************************

#ifndef HM_OPCODE_H
#define HM_OPCODE_H

#include "common.h"

/* A method's "meat" is its sequence of opcodes (bytecode). The interpreter reads opcodes and executes them.
   Many opcodes are followed by additional encodings: what argument to load, what constant to push etc.
   The encodings are explained in the comments below next to every opcode. */
typedef hm_uint8 hmOpcode;
#define HM_OPCODE_LDARG_I32 ((hmOpcode)0) /* Loads an int32 argument at position N (8-bit) and pushes it to the stack. */
#define HM_OPCODE_LDARG_OBJ ((hmOpcode)1) /* Loads an object argument at position N (8-bit) and pushes it to the stack. */
#define HM_OPCODE_LDC_I32   ((hmOpcode)2) /* Loads an int32 constant which is encoded by an int32 value right after the opcode. */
#define HM_OPCODE_ADD_I32   ((hmOpcode)3) /* Adds two integers on the stack and pushes the result back to the stack.
                                             The integers' positions in the stack are encoded at positions N and N+1 right after the code (8-bit). */

#endif /* HM_OPCODE_H */
