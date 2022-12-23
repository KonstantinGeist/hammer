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
#define HM_OPCODE_NOP      ((hmOpcode)0)  /* nop Do nothing (No operation). */
#define HM_OPCODE_STLOC32  ((hmOpcode)1)  /* stloc.32 <uint16(N)> Pop a 32-bit value from the stack into the variable space at offset N. */
#define HM_OPCODE_STLOC64  ((hmOpcode)2)  /* stloc.64 <uint16(N)> Pop a 64-bit value from the stack into the variable space at offset N. */
#define HM_OPCODE_LDARG32  ((hmOpcode)3)  /* ldarg.32 <uint16(N)> Load a 32-bit argument at offset N in the argument space onto the stack. */
#define HM_OPCODE_LDARG64  ((hmOpcode)4)  /* ldarg.64 <uint16(N)> Load a 64-bit argument at offset N in the argument space onto the stack. */
#define HM_OPCODE_LDLOC32  ((hmOpcode)5)  /* ldloc.32 <uint16(N)> Load a 32-bit value at offset N in the variable space onto the stack. */
#define HM_OPCODE_LDLOC64  ((hmOpcode)6)  /* ldloc.64 <uint16(N)> Load a 64-bit value at offset N in the variable space onto the stack. */
#define HM_OPCODE_LDARGA   ((hmOpcode)7)  /* ldarga <uint16(N)> Fetch the address of an argument at offset N in the argument space and push it to the stack. */
#define HM_OPCODE_STARG32  ((hmOpcode)8)  /* starg.32 <uint16(N)> Store a 32-bit value from the stack to the argument at offset N in the argument space. */
#define HM_OPCODE_STARG64  ((hmOpcode)9)  /* starg.64 <uint16(N)> Store a 64-bit value from the stack to the argument at offset N in the argument space. */
#define HM_OPCODE_LDLOCA   ((hmOpcode)10) /* ldloca <uint16(N)> Fetch the address of a variable at offset N in the variable space and push it to the stack. */
#define HM_OPCODE_LDC32    ((hmOpcode)11) /* ldc.32 <any32(N)> Push a 32-bit constant onto the stack. */
#define HM_OPCODE_LDC64    ((hmOpcode)12) /* ldc.32 <any64(N)> Push a 64-bit constant onto the stack. */
#define HM_OPCODE_DUP32    ((hmOpcode)13) /* dup.32 Duplicate a 32-bit value on the top of the stack. */
#define HM_OPCODE_DUP64    ((hmOpcode)14) /* dup.64 Duplicate a 64-bit value on the top of the stack. */
#define HM_OPCODE_POP32    ((hmOpcode)15) /* pop.32 Pop a 32-bit value from the stack. */
#define HM_OPCODE_POP64    ((hmOpcode)16) /* pop.64 Pop a 64-bit value from the stack. */
#define HM_OPCODE_CALL     ((hmOpcode)17) /* call <uint32(N)> Call a method with ID = N. */

// TODO push string constant (or, rather, a blob)
// TODO finish the opcode list

#endif /* HM_OPCODE_H */
