#ifndef INTERMEDIATE_REPRESENTATION_H
#define INTERMEDIATE_REPRESENTATION_H

#include <codegen.h>
#include <codegen/codegen_forward.h>
#include <inttypes.h>

#define INSTRUCTION(name, given_type)                       \
  IRInstruction *(name) = calloc(1, sizeof(IRInstruction)); \
  ASSERT((name), "Could not allocate new IRInstruction.");  \
  (name)->type = (given_type);

typedef enum IRType {
  IR_IMMEDIATE,
  IR_CALL,
  IR_RETURN,
  IR_LOAD,
  IR_BRANCH,
  IR_BRANCH_CONDITIONAL,
  IR_PHI,

  IR_ADD,
  IR_SUBTRACT,

  IR_LOCAL_LOAD,
  IR_LOCAL_STORE,
  IR_LOCAL_ADDRESS,

  IR_GLOBAL_LOAD,
  IR_GLOBAL_STORE,
  IR_GLOBAL_ADDRESS,

  IR_COMPARISON,

  IR_PARAMETER_REFERENCE,

  IR_COUNT
} IRType;

typedef struct IRPhiArgument {
  /// The value of the argument itself.
  IRInstruction *value;
  /// Stores the predecessor to the Phi node in the direction of the
  /// argument assignment.
  ///    [a]
  ///  [t] [o]
  ///    \ [b]
  ///    [j]
  /// For example, if arg->value->block == o, then arg->block == b.
  IRBlock *block;
  // A linked list of arguments.
  struct IRPhiArgument *next;
} IRPhiArgument;

typedef struct IRPair {
  IRInstruction *car;
  IRInstruction *cdr;
} IRPair;

typedef struct IRCallArgument {
  IRInstruction *value;
  struct IRCallArgument *next;
} IRCallArgument;

typedef enum IRCallType {
  IR_CALLTYPE_DIRECT,
  IR_CALLTYPE_INDIRECT,
  IR_CALLTYPE_COUNT
} IRCallType;

typedef union IRCallValue {
  char *name;
  IRInstruction *callee;
} IRCallValue;

typedef struct IRCall {
  IRCallType type;
  IRCallValue value;
  IRCallArgument *arguments;
} IRCall;

typedef struct IRBranchConditional {
  IRInstruction *condition;
  IRBlock *true_branch;
  IRBlock *false_branch;
} IRBranchConditional;

typedef struct IRComparison {
  enum ComparisonType type;
  IRPair pair;
} IRComparison;

typedef struct IRGlobalAssignment {
  IRInstruction *new_value;
  char *name;
} IRGlobalAssignment;

typedef union IRValue {
  IRBlock *block;
  IRInstruction *reference;
  int64_t immediate;
  IRCall call;
  IRPhiArgument *phi_argument;
  IRBranchConditional conditional_branch;
  IRPair pair;
  IRComparison comparison;
  char *name;
  IRGlobalAssignment global_assignment;
} IRValue;

typedef struct IRInstruction {
  int type;
  IRValue value;

  /// A unique identifier (mainly for debug purposes).
  size_t id;

  // Register allocation.
  size_t index;

  // Doubly linked list.
  struct IRInstruction *previous;
  struct IRInstruction *next;
} IRInstruction;

typedef struct IRBlockPredecessor {
  IRBlock *block;
  struct IRBlockPredecessor *next;
} IRBlockPredecessor;

/// A block is a list of instructions that have control flow enter at
/// the beginning and leave at the end.
typedef struct IRBlock {
  IRInstruction *instructions;
  IRInstruction *last_instruction;

  IRInstruction *branch;

  IRBlockPredecessor *predecessor;

  // Doubly linked list.
  struct IRBlock *previous;
  struct IRBlock *next;

  // Unique ID (among blocks)
  size_t id;
} IRBlock;

typedef struct IRFunction {
  IRBlock *first;
  IRBlock *last;

  IRInstruction *return_value;

  // Linked list.
  struct IRFunction *next;

  // Unique ID (among functions)
  size_t id;
} IRFunction;

void ir_set_ids(CodegenContext *context);

void ir_femit_instruction
(FILE *file,
 IRInstruction *instruction);
void ir_femit_block
(FILE *file,
 IRBlock *block);
void ir_femit_function
(FILE *file,
 IRFunction *function);
void ir_femit
(FILE *file,
 CodegenContext *context);

void ir_add_function_call_argument
(CodegenContext *context,
 IRInstruction *call,
 IRInstruction *argument);

IRBlock *ir_block_create();

void ir_block_attach
(CodegenContext *context,
 IRBlock *new_block);

IRFunction *ir_function_create();
IRFunction *ir_function(CodegenContext *context);

void ir_insert
(CodegenContext *context,
 IRInstruction *new_instruction);

void ir_phi_argument
(IRInstruction *phi,
 IRBlock *phi_predecessor,
 IRInstruction *argument);

IRInstruction *ir_phi
(CodegenContext *context);

IRInstruction *ir_direct_call
(CodegenContext *context,
 char* function_name);

IRInstruction *ir_indirect_call
(CodegenContext *context,
 IRInstruction *function);

IRInstruction *ir_load_global_address
(CodegenContext *context,
 char *name);

IRInstruction *ir_immediate
(CodegenContext *context,
 int64_t immediate);

IRInstruction *ir_load
(CodegenContext *context,
 IRInstruction *address);

IRInstruction *ir_load_local_address
(CodegenContext *context,
 IRInstruction *local);

IRInstruction *ir_load_global
(CodegenContext *context,
 char *name);

IRInstruction *ir_load_local
(CodegenContext *context,
 IRInstruction *local);

IRInstruction *ir_store_global
(CodegenContext *context,
 IRInstruction *source,
 char *name);

IRInstruction *ir_store_local
(CodegenContext *context,
 IRInstruction *source,
 IRInstruction *local);

IRInstruction *ir_store
(CodegenContext *context,
 IRInstruction *data,
 IRInstruction *address);

IRInstruction *ir_branch_conditional
(CodegenContext *context,
 IRInstruction *condition,
 IRBlock *then_block,
 IRBlock *otherwise_block);

IRInstruction *ir_branch
(CodegenContext *context,
 IRBlock *destination);

IRInstruction *ir_return
(CodegenContext *context);

IRInstruction *ir_comparison
(CodegenContext *context,
 enum ComparisonType type,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_add
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_subtract
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_multiply
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_divide
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_modulo
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_shift_left
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_shift_right_arithmetic
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs);

IRInstruction *ir_stack_allocate
(CodegenContext *context,
 int64_t size);

#endif /* INTERMEDIATE_REPRESENTATION_H */
