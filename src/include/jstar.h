// MIT License

// Copyright (c) 2018 Fabrizio Pietrucci

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef JSTAR_H
#define JSTAR_H

#include "jstarconf.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * =========================================================
 *  J* VM entry points
 * =========================================================
 */

// The J* virtual machine
typedef struct JStarVM JStarVM;

typedef enum {
    VM_EVAL_SUCCESS, // The VM successfully executed the code
    VM_SYNTAX_ERR,    // A syntax error has been encountered in parsing
    VM_COMPILE_ERR,   // An error has been encountered during compilation
    VM_RUNTIME_ERR,   // An unhandled exception has reached the top of the stack
} EvalResult;

// Allocate a new VM with all the state needed for code execution
JSTAR_API JStarVM *jsrNewVM();
// Free a previously obtained VM along with all the state
JSTAR_API void jsrFreeVM(JStarVM *vm);

// Evaluate J* code in the context of module (or __main__ in blEvaluate)
// as top level <main> function.
// VM_EVAL_SUCCSESS will be returned if the execution completed normally
// In case of errors, either VM_SYNTAX_ERR, VM_COMPILE_ERR or VM_RUNTIME_ERR
// will be returned, and all the errors will be printed to stderr.
JSTAR_API EvalResult jsrEvaluate(JStarVM *vm, const char *fpath, const char *src);
JSTAR_API EvalResult jsrEvaluateModule(JStarVM *vm, const char *fpath, const char *name, const char *src);

// Call a function (or method with name "name") that sits on the top of the stack
// along with its arguments. The state of the stack when calling should be:
//  ... [callee][arg1][arg2]...[argn] $top
//         |       |______________|
//         |                |
// function/instance  the args of the function/method
//
// In case of success VM_EVAL_SUCCSESS will be returned, and the result will be placed
// on the top of the stack in the place of "callee", popping all arguments:
//  ... [result] $top [arg1][arg2] ... [argn] popped
//
// If an exception has been raised by the code, VM_RUNTIME_ERR will be returned and
// The exception will be placed on top of the stack as a result.
JSTAR_API EvalResult jsrCall(JStarVM *vm, uint8_t argc);
JSTAR_API EvalResult jsrCallMethod(JStarVM *vm, const char *name, uint8_t argc);

// Prints the the stack trace of the exception on the top of the stack
JSTAR_API void jsrPrintStackTrace(JStarVM *vm);
// Init the sys.args list with a list of arguments (usually main arguments)
JSTAR_API void jsrInitCommandLineArgs(int argc, const char **argv);
// Add a path to be searched during module imports
JSTAR_API void jsrAddImportPath(JStarVM *vm, const char *path);

/**
 * =========================================================
 *  Native function API
 * =========================================================
 */

// ---- Utility functions and definitions ----

// The minimum reserved space for the stack when calling a native function
#define JSTAR_MIN_NATIVE_STACK_SZ 20

// Utility macro for declaring/defining a native function
#define JSR_NATIVE(name) bool name(JStarVM *vm)

// Utility macro for raising an exception from a native function.
// It raises the exception and exits signaling the error.
#define JSR_RAISE(vm, cls, err, ...)           \
    do {                                       \
        jsrRaise(vm, cls, err, ##__VA_ARGS__); \
        return false;                          \
    } while(0)

// Main module and core module names
#define JSR_MAIN_MODULE "__main__"
#define JSR_CORE_MODULE "__core__"

// Ensure `needed` slots are available on the stack
JSTAR_API void jsrEnsureStack(JStarVM *vm, size_t needed);

// A C function callable from J*
typedef bool (*JStarNative)(JStarVM *vm);

// ---- Registry to register native functions ----

// J* native registry, used to associate names to native pointers in native c extension modules.
typedef struct JStarNativeReg {
    enum {REG_METHOD, REG_FUNCTION, REG_SENTINEL} type;
    union {
        struct { const char *cls; const char *name; JStarNative meth; } method;
        struct { const char *name; JStarNative fun; } function;
    } as;
} JStarNativeReg;

#define JSR_REGFUNC(name, func)      { REG_FUNCTION, { .function = { #name, func } } },
#define JSR_REGMETH(cls, name, meth) { REG_METHOD, { .method = { #cls, #name, meth } } },
#define JSR_REGEND                   { REG_SENTINEL, { .function = { NULL, NULL } } }

// ---- Overloadable operator functions ----

// Instantiate an exception from "cls" with "err" as an error string and raises
// it, leaving it on top of the stack.
// If "cls" cannot be found in current module a NameException is raised instead.
JSTAR_API void jsrRaise(JStarVM *vm, const char *cls, const char *err, ...);

// Check if two J* values are equal.
// As this function may call the __eq__ method, it behaves like
// blCall, i.e. the two values should be on the top of the stack
// when calling, and the result will be left on the top of the
// stack popping the two values.
// This function will return true if the execution was successful,
// And false if an exception was raised, leaving the result or
// the exception on top of the stack repectively.
JSTAR_API bool jsrEquals(JStarVM *vm);

// Check if a value is of a certain class.
JSTAR_API bool jsrIs(JStarVM *vm, int slot, int classSlot);

// ---- Iterable protocol functions ----

// `iterable` is the slot in which the iterable object is sitting and `res` is the slot of the
// result of the last blIter call or, if first time calling blIter, a slot containing null.
// blNext is called to obtain the next element in the iteration. The element will be placed
// on the top of the stack.
JSTAR_API bool jsrIter(JStarVM *vm, int iterable, int res, bool *err);
JSTAR_API bool jsrNext(JStarVM *vm, int iterable, int res);

// Macro that automatically configures the loop to iterate over a J* iterable using blIter and
// blNext.
// `iter` is the slot of the iterable we want to iterate over and `code` a block used as the body.
// Beware that the macro pushes a new value on top of the stack to store the result of blIter, so
// negative slot indeces to access previously pushed elements should be offset by one
#define JSR_FOREACH(iter, code, cleanup)         \
    {                                            \
        bool _err = false;                       \
        jsrPushNull(vm);                         \
        while(jsrIter(vm, iter, -1, &_err)) {    \
            if(_err || !jsrNext(vm, iter, -1)) { \
                cleanup;                         \
                return false;                    \
            }                                    \
            code                                 \
        }                                        \
        jsrPop(vm);                              \
    }

// ---- C to J* values converting functions ----

// The converted value is left on the top of the stack

JSTAR_API void jsrPushNumber(JStarVM *vm, double number);
JSTAR_API void jsrPushBoolean(JStarVM *vm, bool boolean);
JSTAR_API void jsrPushStringSz(JStarVM *vm, const char *string, size_t size);
JSTAR_API void jsrPushString(JStarVM *vm, const char *string);
JSTAR_API void jsrPushBoolean(JStarVM *vm, bool b);
JSTAR_API void jsrPushHandle(JStarVM *vm, void *handle);
JSTAR_API void jsrPushNull(JStarVM *vm);
JSTAR_API void jsrPushList(JStarVM *vm);
JSTAR_API void jsrPushTuple(JStarVM *vm, size_t size);
JSTAR_API void jsrPushValue(JStarVM *vm, int slot);
#define jsrDup() jsrPushValue(vm, -1)

// ---- J* to C values converter functions ----

JSTAR_API double jsrGetNumber(JStarVM *vm, int slot);
JSTAR_API bool jsrGetBoolean(JStarVM *vm, int slot);
JSTAR_API void *jsrGetHandle(JStarVM *vm, int slot);
JSTAR_API size_t jsrGetStringSz(JStarVM *vm, int slot);

// BEWARE: The returned string is owned by J*
// and thus is garbage collected. Never use this
// buffer outside the native where it was retrieved.
// Also be careful when popping the original ObjString
// from the stack  while retaining this buffer, because 
// if a GC occurs and the string is not found to be 
// reachable it'll be collected.
JSTAR_API const char *jsrGetString(JStarVM *vm, int slot);

// ---- List manipulation functions ----

// These functions do not perfrom bounds checking,
// use blCeckIndex first if needed.

JSTAR_API void jsrListAppend(JStarVM *vm, int slot);
JSTAR_API void jsrListInsert(JStarVM *vm, size_t i, int slot);
JSTAR_API void jsrListRemove(JStarVM *vm, size_t i, int slot);
JSTAR_API void jsrListGetLength(JStarVM *vm, int slot);
JSTAR_API void jsrListGet(JStarVM *vm, size_t i, int slot);

// ---- Tuple manipulation functions ----

// These functions do not perfrom bounds checking,
// use blCeckIndex first if needed.

JSTAR_API void jsrTupleGetLength(JStarVM *vm, int slot);
JSTAR_API void jsrTupleGet(JStarVM *vm, size_t i, int slot);

// ---- Object instances manipulation functions ----

// Set the field "name" of the value at "slot" with the value
// on top of the stack. the value is not popped.
JSTAR_API void jsrSetField(JStarVM *vm, int slot, const char *name);

// Get the field "name" of the value at "slot".
// Returns true in case of success leaving the result on
// top of the stack, false otherwise leaving an exception
// on top of the stack.
JSTAR_API bool jsrGetField(JStarVM *vm, int slot, const char *name);

// ---- Modules manipulation functions ----

// Set the global "name" of the module "mname" with the value
// on top of the stack. the value is not popped.
// If calling from inside a native mname can be NULL, and the
// used module will be the current one.
JSTAR_API void jsrSetGlobal(JStarVM *vm, const char *mname, const char *name);

// Get the global "name" of the module "mname".
// Returns true in case of success leaving the result on the
// top of the stack, false otherwise leaving an exception on
// top of the stack.
// If calling from inside a native mname can be NULL, and the
// used module will be the current one.
JSTAR_API bool jsrGetGlobal(JStarVM *vm, const char *mname, const char *name);

// ---- J* type checking functions ----

// These functions return true if the slot is of the given type, false otherwise
JSTAR_API bool jsrIsNumber(JStarVM *vm, int slot);
JSTAR_API bool jsrIsInteger(JStarVM *vm, int slot);
JSTAR_API bool jsrIsString(JStarVM *vm, int slot);
JSTAR_API bool jsrIsList(JStarVM *vm, int slot);
JSTAR_API bool jsrIsTuple(JStarVM *vm, int slot);
JSTAR_API bool jsrIsBoolean(JStarVM *vm, int slot);
JSTAR_API bool jsrIsHandle(JStarVM *vm, int slot);
JSTAR_API bool jsrIsNull(JStarVM *vm, int slot);
JSTAR_API bool jsrIsInstance(JStarVM *vm, int slot);

// These functions return true if the slot is of the given type, false otherwise leaving a
// TypeException on top of the stack with a message customized with 'name'
JSTAR_API bool jsrCheckNum(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckInt(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckStr(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckList(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckTuple(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckBool(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckInstance(JStarVM *vm, int slot, const char *name);
JSTAR_API bool jsrCheckHandle(JStarVM *vm, int slot, const char *name);

// Check if the value at slot "slot" is an integer >= 0 and < max.
// Returns the number casted to size_t if true, SIZE_MAX if false
// leaving an exception on top of the stack.
JSTAR_API size_t jsrCheckIndex(JStarVM *vm, int slot, size_t max, const char *name);

// Pop a value from the top of the stack
JSTAR_API void jsrPop(JStarVM *vm);

/**
 * =========================================================
 *  Buffer creation and manipulation functions
 * =========================================================
 */

// Dynamic Buffer that holds memory allocated by the J* garbage collector.
// This memory is owned by J*, but cannot be collected until the buffer 
// is pushed on the stack using the blBufferPush method.
// Used for efficient creation of Strings in the native API.
typedef struct JStarBuffer {
    JStarVM *vm;
    size_t size;
    size_t len;
    char *data;
} JStarBuffer;

JSTAR_API void jsrBufferInit(JStarVM *vm, JStarBuffer *b);
JSTAR_API void jsrBufferInitSz(JStarVM *vm, JStarBuffer *b, size_t size);
JSTAR_API void jsrBufferAppend(JStarBuffer *b, const char *str, size_t len);
JSTAR_API void jsrBufferAppendstr(JStarBuffer *b, const char *str);
JSTAR_API void jsrBufferTrunc(JStarBuffer *b, size_t len);
JSTAR_API void jsrBufferCut(JStarBuffer *b, size_t len);
JSTAR_API void jsrBufferReplaceChar(JStarBuffer *b, size_t start, char c, char r);
JSTAR_API void jsrBufferPrepend(JStarBuffer *b, const char *str, size_t len);
JSTAR_API void jsrBufferPrependstr(JStarBuffer *b, const char *str);
JSTAR_API void jsrBufferAppendChar(JStarBuffer *b, char c);
JSTAR_API void jsrBufferClear(JStarBuffer *b);

// Once the buffer is pushed on the J* stack it becomes a String and can't be modified further
// One can reuse the BlBuffer struct by re-initializing it using the blBufferInit method.
JSTAR_API void jsrBufferPush(JStarBuffer *b);
JSTAR_API void jsrBufferFree(JStarBuffer *b);

#endif