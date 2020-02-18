# Sinter VM

## Instructions

* nop: no-op
* ldc.i: load constant integer
* ldc.f: load constant float
* ldc.b: load constant boolean
* ldc.u: load constant undefined
* ldc.n: load constant null
* ldc.s: load constant string
* pop: pop from stack
* add: add
* sub: subtract
* mul: multiply
* div: divide
* mod: modulo
* not: not
* neg: negate
* lt: less than
* gt: greater than
* le: less than or equal to
* ge: greater than or equal to
* eq: equal
* new.f: create function
* new.v: create array (vector)
* ld.e: load from environment
* st.e: store in environment
* ld.a: load argument
* st.a: store argument
* ld.v: load from array (vector)
* st.v: store in array (vector)
* br.t: branch if true
* br: branch
* jmp: jump
* call: call function
* call.t: tail call function
* call.p: call primitive function
* call.v: call VM-internal function/native function
* ret: return
* ret.u: return undefined
