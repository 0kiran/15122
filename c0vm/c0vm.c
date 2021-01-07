#include <assert.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>

#include "lib/xalloc.h"
#include "lib/stack.h"
#include "lib/contracts.h"
#include "lib/c0v_stack.h"
#include "lib/c0vm.h"
#include "lib/c0vm_c0ffi.h"
#include "lib/c0vm_abort.h"

/* call stack frames */
typedef struct frame_info frame;
struct frame_info {
  c0v_stack_t S; /* Operand stack of C0 values */
  ubyte *P;      /* Function body */
  size_t pc;     /* Program counter */
  c0_value *V;   /* The local variables */
};

int execute(struct bc0_file *bc0) {
  REQUIRES(bc0 != NULL);

  /* Variables */
  c0v_stack_t S = c0v_stack_new(); /* Operand stack of C0 values */
  ubyte *P = bc0->function_pool->code;      /* Array of bytes that make up the current function */
  size_t pc = 0;     /* Current location within the current byte array P */
  c0_value *V = xcalloc(255, sizeof(c0_value));   /* Local variables (you won't need this till Task 2) */

  /* The call stack, a generic stack that should contain pointers to frames */
  /* You won't need this until you implement functions. */
  gstack_t callStack;
  callStack = stack_new();
  while (true) {

#ifdef DEBUG
    /* You can add extra debugging information here */
    fprintf(stderr, "Opcode %x -- Stack size: %zu -- PC: %zu\n",
            P[pc], c0v_stack_size(S), pc);
#endif

    switch (P[pc]) {

    /* Additional stack operation: */

    case POP: {
      pc++;
      c0v_pop(S);
      break;
    }

    case DUP: {
      pc++;
      c0_value v = c0v_pop(S);
      c0v_push(S,v);
      c0v_push(S,v);
      break;
    }

    case SWAP:{
      pc++;
      c0_value v = c0v_pop(S);
      c0_value v2 = c0v_pop(S);
      c0v_push(S, v);
      c0v_push(S, v2);
      break;
    }

    /* Returning from a function.
     * This currently has a memory leak! You will need to make a slight
     * change for the initial tasks to avoid leaking memory.  You will
     * need to be revise it further when you write INVOKESTATIC. */

    case RETURN: {
      // Free everything before returning from the execute function!
      if(stack_empty(callStack)){
        pc++;
        c0_value r = c0v_pop(S);
	int32_t retval = val2int(r);
#ifdef DEBUG
      fprintf(stderr, "Returning %d from execute()\n", retval);
#endif
	stack_free(callStack, NULL);
	free(V);
	c0v_stack_free(S);
	return retval;
      }
      else{
        c0_value fpop = c0v_pop(S);
	free(V);
	c0v_stack_free(S);
        frame *f = pop(callStack);
	V = f->V;
	pc = f->pc;
	S = f->S;
	c0v_push(S, fpop);
	P = f->P;
	free(f);
      }
      break;
    }


    /* Arithmetic and Logical operations */

    case IADD:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = x+y;
      c0v_push(S, int2val(f));
      break;
    }
    case ISUB:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = y - x;
      c0v_push(S, int2val(f));
      break;
    }
    case IMUL:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = y*x;
      c0v_push(S, int2val(f));
      break;
    }
    case IDIV:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      if(x == 0) c0_arith_error("div by 0");
      if(x == -1 && y == -2147483648) c0_arith_error("div int_min by -1");
      int32_t f = y / x;
      c0v_push(S, int2val(f));
      break;
    }
    case IREM:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      if(x == 0) c0_arith_error("mod 0");
      if(x == -1 && y == -2147483648) c0_arith_error("int_min mod -1");
      int32_t f = y%x;
      c0v_push(S, int2val(f));
      break;
    }
    case IAND:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = y&x;
      c0v_push(S, int2val(f));
      break;
    }
    case IOR:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = y | x;
      c0v_push(S, int2val(f));
      break;
    }
    case IXOR:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      int32_t f = y ^ x;
      c0v_push(S, int2val(f));
      break;
    }
    case ISHL:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      if(x < 0 || x > 31) c0_arith_error("bad left bitshift");
      int32_t f = y << x;
      c0v_push(S, int2val(f));
      break;
    }
    case ISHR:{
      pc++;
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t x = val2int(c0v_pop(S));
      if(c0v_stack_empty(S)) c0_arith_error("not enough operands");
      int32_t y = val2int(c0v_pop(S));
      if(x < 0 || x > 31) c0_arith_error("bad right bitshift");
      int32_t f = y >> x;
      c0v_push(S, int2val(f));
      break;
    }


    /* Pushing constants */

    case BIPUSH:{
      pc++;
      byte y = P[pc];
      pc++;
      int32_t x = y;
      c0v_push(S, int2val(x));
      break;
    }

    case ILDC:{
      pc++;
      ubyte y1 = P[pc];
      pc++;
      ubyte y2 = P[pc];
      pc++;
      int32_t i = (y1<<8) | y2;
      if(i >= bc0->int_count) c0_arith_error("bad index for intpool");
      int32_t x = bc0->int_pool[i];
      c0v_push(S, int2val(x));
      break;
    }
    case ALDC:{
      pc++;
      ubyte y1 = P[pc];
      pc++;
      ubyte y2 = P[pc];
      pc++;
      int32_t i = (y1<<8)|y2;
      if(i >= bc0->string_count) c0_arith_error("bad index for strpool");
      void *x = &(bc0->string_pool[i]);
      c0v_push(S, ptr2val(x));
      break;
    }

    case ACONST_NULL:{
      pc++;
      void *k = NULL;
      c0v_push(S, ptr2val(k));
      break;
    }

    /* Operations on local variables */

    case VLOAD:{
      pc++;
      ubyte b = P[pc];
      pc++;
      c0v_push(S, V[b]);
      break;
    }
    case VSTORE:{
      pc++;
      c0_value x = c0v_pop(S);
      ubyte b = P[pc];
      pc++;
      V[b] = x;
      break;
    }
    /* Assertions and errors */

    case ATHROW:{
      pc++;
      char *a = (char*)val2ptr(c0v_pop(S));
      c0_user_error(a);
      break;
    }
    case ASSERT:{
      pc++;
      char *a = (char*)val2ptr(c0v_pop(S));
      int32_t i = val2int(c0v_pop(S));
      if(i == 0) c0_assertion_failure(a);
      break;
    }

    /* Control flow operations */

    case NOP:{
      pc++;
      break;
    }

    case IF_CMPEQ:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      c0_value v1 = c0v_pop(S);
      c0_value v2 = c0v_pop(S);
      if(val_equal(v1,v2)) pc = (pc-3)+x;
      break;
    }
    case IF_CMPNE:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      c0_value v1 = c0v_pop(S);
      c0_value v2 = c0v_pop(S);
      if(!val_equal(v1,v2)) pc = (pc-3)+x;
      break;
    }
    case IF_ICMPLT:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      int32_t y = val2int(c0v_pop(S));
      int32_t x1 = val2int(c0v_pop(S));
      if(x1 < y) pc = (pc-3)+x;
      break;
    }
    case IF_ICMPGE:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      int32_t y = val2int(c0v_pop(S));
      int32_t x1 = val2int(c0v_pop(S));
      if(x1 >= y) pc = (pc-3)+x;
      break;
    }
    case IF_ICMPGT:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      int32_t y = val2int(c0v_pop(S));
      int32_t x1 = val2int(c0v_pop(S));
      if(x1 > y) pc = (pc-3)+x;
      break;
    }
    case IF_ICMPLE:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      int32_t y = val2int(c0v_pop(S));
      int32_t x1 = val2int(c0v_pop(S));
      if(x1 <= y) pc = (pc-3)+x;
      break;
    }
    case GOTO:{
      pc++;
      ubyte o1 = P[pc];
      pc++;
      ubyte o2 = P[pc];
      pc++;
      int16_t x = (o1<<8)|o2;
      pc = (pc-3)+x;
      break;
    }

    /* Function call operations: */

    case INVOKESTATIC:{
      
      frame *fstack = xmalloc(sizeof(struct frame_info));
      fstack->S = S;
      fstack->P = P;
      fstack->pc = pc+3;
      fstack->V = V;
      push(callStack, fstack); 

      ubyte c1 = P[pc+1];
      ubyte c2 = P[pc+2];
      uint16_t x = (c1<<8)|c2;

      //printf("pushed callstack\n");

      struct function_info nf = bc0->function_pool[x];
      V = xcalloc(255, sizeof(c0_value));
      for(int i = nf.num_args-1; i >= 0; i--){
        V[i] = c0v_pop(fstack->S);
      }
      P = nf.code;
      pc = 0;
      S = c0v_stack_new();
      break;
    }

    case INVOKENATIVE:{
      pc++;
      ubyte c1 = P[pc];
      pc++;
      ubyte c2 = P[pc];
      pc++;
      uint16_t x = (c1<<8)|c2;

      struct native_info n = bc0->native_pool[x];
      c0_value* nativv = xcalloc(n.num_args, sizeof(c0_value));

      for(int i = n.num_args-1; i >= 0; i--){
        nativv[i] = c0v_pop(S);
      }
      //get index of function (function_table_index)
      unsigned int in = n.function_table_index;
   
      c0_value vr = (*native_function_table[in])(nativv);
      free(nativv);
      c0v_push(S,vr);
      break;
    }

    /* Memory allocation operations: */

    case NEW:{
      pc++;
      ubyte s = P[pc];
      pc++;
      c0_value a = ptr2val(xmalloc(s));
      c0v_push(S, a);
      break;
    }

    case NEWARRAY:{
      pc++;
      ubyte s = P[pc];
      pc++;

      int32_t n = val2int(c0v_pop(S));
      if(n < 0) c0_arith_error("negative size array");
      c0_array* arr = xmalloc(sizeof(c0_array));
      void* a = xcalloc(n,s);
    
      arr->count = n;
      arr->elt_size = s;
      arr->elems = a;
      c0v_push(S, ptr2val(arr));
      break;
    }

    case ARRAYLENGTH:{
      pc++;
      c0_array* arr = val2ptr(c0v_pop(S));
      if(arr == NULL){
        c0v_push(S, int2val(0));
      }
      else c0v_push(S, int2val(arr->count));
      break;
    }


    /* Memory access operations: */

    case AADDF:{
      pc++;
      ubyte f = P[pc];
      pc++;

      void** a = (void**)val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("tf ??");
      char* atemp = *a; 
      c0v_push(S, ptr2val(atemp+f));
      break;
    }

    case AADDS:{
      pc++;
      int32_t i = val2int(c0v_pop(S));
      c0_array* arr = val2ptr(c0v_pop(S));

      if(arr == NULL) c0_memory_error("empty array");
      if(0 > i || i >= arr->count) c0_memory_error("bad index");
      char* b = arr->elems; 
      c0v_push(S, ptr2val(&b[i*arr->elt_size]));
      break;
    }

    case IMLOAD:{
      pc++;
      int* a = val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("IMLOAD");
      c0v_push(S, int2val(*a));
      break;
    }

    case IMSTORE:{
      pc++;
      int x = val2int(c0v_pop(S));
      int* a = val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("IMSTORE");

      *a = x;
      break;
    }
    case AMLOAD:{
      pc++;
      void** a = (void**)val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("AMLOAD");
      c0v_push(S, ptr2val(*a));
      break;
    }
    case AMSTORE:{
      pc++;
      void** x = val2ptr(c0v_pop(S));
      void** a = val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("AMSTORE");
      *a = x;
      break;
    }
    case CMLOAD:{
      pc++;
      char* x = val2ptr(c0v_pop(S));
      if(x == NULL) c0_memory_error("CMLOAD");
      c0v_push(S, int2val((uint32_t)*x));
      break;
    }
    case CMSTORE:{
      pc++;
      char x = val2int(c0v_pop(S));
      char* a = val2ptr(c0v_pop(S));
      if(a == NULL) c0_memory_error("CMSTORE");
      *a = x & 0x7f;
      break;
    }
    default:
      fprintf(stderr, "invalid opcode: 0x%02x\n", P[pc]);
      abort();
    }
  }

  /* cannot get here from infinite loop */
  assert(false);
}
