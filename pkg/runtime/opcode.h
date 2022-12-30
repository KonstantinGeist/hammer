/* *****************************************************************************
*
*   Copyright (c) Konstantin Geist. All rights reserved.
*
*   The use and distribution terms for this software are contained in the file
*   named License.txt, which can be found in the root of this distribution.
*   By using this software in any fashion, you are agreeing to be bound by the
*   terms of this license.
*
*   You must not remove this notice, or any other, from this software.
*
* ******************************************************************************/

#ifndef HM_OPCODE_H
#define HM_OPCODE_H

#include <core/common.h>

/* A method's body consists of a sequence of opcodes (bytecode). The interpreter reads opcodes and executes them.
   Many opcodes are followed by additional encodings: what argument to load, what constant to push etc. */
typedef hm_uint8 hmOpcode;
#define HM_OPCODE_NOP    ((hmOpcode)0)  /* nop Do nothing (No operation). */
#define HM_OPCODE_STLOC  ((hmOpcode)1)  /* stloc <uint16(N)> Pop a value from the stack into the variable space at index N. */
#define HM_OPCODE_LDARG  ((hmOpcode)2)  /* ldarg <uint16(N)> Load an argument at index N in the argument space onto the stack. */
#define HM_OPCODE_LDLOC  ((hmOpcode)3)  /* ldloc <uint16(N)> Load a value at index N in the variable space onto the stack. */
#define HM_OPCODE_STARG  ((hmOpcode)4)  /* starg <uint16(N)> Store a value from the stack to the argument at index N in the argument space. */
#define HM_OPCODE_LDC32  ((hmOpcode)5)  /* ldc.32 <any32(N)> Push a 32-bit constant onto the stack. */
#define HM_OPCODE_LDC64  ((hmOpcode)6)  /* ldc.64 <any64(N)> Push a 64-bit constant onto the stack. */
#define HM_OPCODE_DUP    ((hmOpcode)7)  /* dup Duplicate a value on the top of the stack. */
#define HM_OPCODE_POP    ((hmOpcode)8)  /* pop Pop a value from the stack. */
#define HM_OPCODE_CALL   ((hmOpcode)9)  /* call <uint32(N)> Call a method with ID = N. */

#endif /* HM_OPCODE_H */
