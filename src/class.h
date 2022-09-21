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

#ifndef HM_CLASS_H
#define HM_CLASS_H

#define "common.h"

/* Although methods are usually defined by their bytecode, it's also possible to implement certain
   built-in methods entirely in C code. */
typedef hmError (*hmMethodFunc)(); /* TODO specify actual parameters */

/* A method of execution consists of: its signature (parameter types, return type), its method body (bytecode)
   and other auxiliary data. */
typedef struct {
    const char*  name;            /* The name of the method. The name should be unique in a given class. */
    hmTypeRef*   param_type_refs; /* Typeref list for the parameters, indexed from 0 to param_count-1. */
    hmOpcode*    opcodes;         /* Bytecode of the method to be interpreted. Can be null only if method_func is non-null.
                                     The size is determined by opcode_size. */
    hmMethodFunc method_func;     /* If the method is implemented as a C function instead of bytecode, this pointer
                                     contains a pointer to the function implementation. */
    hmTypeRef    param_typ_ref;   /* The typeref of the returned value. Can be HM_TYPEKIND_VOID as well. */
    hm_nint      param_count;     /* Number of parameters. Can be 0 if there are no parameters. */
    hm_nint      opcode_size;     /* The size of the bytecode. Can be 0 only if method_func is non-null. */
} hmMethod;

/* A class is a collection of methods and properties bundled together. */
typedef struct _hmClass {
    const char* name;          /* The name of the class (NOT fully qualified, for example: "StringBuilder"). The name
                                  should be unique in a given module. */
    hmMethod*   methods;       /* List of methods this class has. The size of the array is determined by method_count. */
    hm_nint     method_count;  /* The number of methods in this class. */
} hmClass;

#endif /* HM_CLASS_H */
