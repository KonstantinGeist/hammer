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

/* A method's body consists of a sequence of high-level opcodes (bytecode).
   Many opcodes are followed by additional encodings: what argument to load, what constant to push etc.
   High-level bytecode is verified and compiled to low-level bytecode on the fly (see hmLLOpcode). */
typedef hm_uint8 hmHLOpcode;
#define HM_HLOPCODE_NOP    ((hmOpcode)0)  /* nop Do nothing (No operation). */
#define HM_HLOPCODE_STLOC  ((hmOpcode)1)  /* stloc <uint16(N)> Pop a value from the stack into the variable space at index N. */
#define HM_HLOPCODE_LDARG  ((hmOpcode)2)  /* ldarg <uint16(N)> Load an argument at index N in the argument space onto the stack. */
#define HM_HLOPCODE_LDLOC  ((hmOpcode)3)  /* ldloc <uint16(N)> Load a value at index N in the variable space onto the stack. */
#define HM_HLOPCODE_STARG  ((hmOpcode)4)  /* starg <uint16(N)> Store a value from the stack to the argument at index N in the argument space. */
#define HM_HLOPCODE_LDC32  ((hmOpcode)5)  /* ldc.32 <any32(N)> Push a 32-bit constant onto the stack. */
#define HM_HLOPCODE_LDC64  ((hmOpcode)6)  /* ldc.64 <any64(N)> Push a 64-bit constant onto the stack. */
#define HM_HLOPCODE_DUP    ((hmOpcode)7)  /* dup Duplicate a value on the top of the stack. */
#define HM_HLOPCODE_POP    ((hmOpcode)8)  /* pop Pop a value from the stack. */
#define HM_HLOPCODE_CALL   ((hmOpcode)9)  /* call <uint32(N)> Call a method with ID = N. */

typedef hm_uint8 hmLLOpcode;

#endif /* HM_OPCODE_H */
