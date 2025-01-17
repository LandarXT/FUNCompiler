#include "codegen/codegen_forward.h"
#include <codegen/intermediate_representation.h>

#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

void ir_insert
(CodegenContext *context,
 IRInstruction *new_instruction
 )
{
  // If block is closed, open a new block.
  if (context->block->branch) {
    //
  }
  ASSERT(context->block->branch == NULL, "Can not insert into a closed IRBlock.");
  // [ ] <-> [ ] <-> [NULL]
  //         [ ] <-> [new] <-> [NULL]
  if (!context->block->instructions) {
    context->block->instructions = new_instruction;
    context->block->last_instruction = new_instruction;
    return;
  }
  context->block->last_instruction->next = new_instruction;
  new_instruction->previous = context->block->last_instruction;
  context->block->last_instruction = new_instruction;
}

#define INSERT(instruction) ir_insert(context, (instruction))

void ir_femit_instruction
(FILE *file,
 IRInstruction *instruction
 )
{
  ASSERT(instruction, "Can not insert NULL instruction");

# define ID_FORMAT "%%%zu | ", instruction->id
  const size_t id_width = 10;
  size_t id_length = snprintf(NULL, 0, ID_FORMAT);
  int64_t difference = id_width - id_length;
  while (difference--) {
    fputc(' ', file);
  }
  fprintf(file, ID_FORMAT);
# undef ID_FORMAT

  switch (instruction->type) {
  case IR_IMMEDIATE:
    fprintf(file, "%"PRId64, instruction->value.immediate);
    break;
  case IR_CALL:
    switch (instruction->value.call.type) {
    case IR_CALLTYPE_DIRECT:
      fprintf(file, "%s", instruction->value.call.value.name);
      break;
    case IR_CALLTYPE_INDIRECT:
      fprintf(file, "%%%zu", instruction->value.call.value.callee->id);
      break;
    default:
      TODO("Handle %d IRCallType.", instruction->value.call.type);
      break;
    }
    fputc('(', file);
    IRCallArgument *argument = instruction->value.call.arguments;
    if (argument) {
      fprintf(file, "%%%zu", argument->value->id);
      argument = argument->next;
    }
    for (; argument; argument = argument->next) {
      fprintf(file, ", %%%zu", argument->value->id);
    }
    fputc(')', file);
    break;
  case IR_RETURN:
    fprintf(file, "return");
    break;
  case IR_ADD:
    fprintf(file, "add %%%zu, %%%zu",
            instruction->value.pair.car->id,
            instruction->value.pair.cdr->id);
    break;
  case IR_SUBTRACT:
    fprintf(file, "subtract %%%zu, %%%zu",
            instruction->value.pair.car->id,
            instruction->value.pair.cdr->id);
    break;
  case IR_GLOBAL_LOAD:
    fprintf(file, "g.load %s", instruction->value.name);
    break;
  case IR_GLOBAL_STORE:
    fprintf(file, "g.store %%%zu, %s",
            instruction->value.global_assignment.new_value->id,
            instruction->value.global_assignment.name);
    break;
  case IR_GLOBAL_ADDRESS:
    fprintf(file, "g.address %s", instruction->value.name);
    break;
  case IR_LOCAL_LOAD:
    fprintf(file, "l.load %%%zu", instruction->value.reference->id);
    break;
  case IR_PARAMETER_REFERENCE:
    fprintf(file, "parameter.reference %%%"PRId64,
            instruction->value.immediate);
    break;
  case IR_COMPARISON:
    switch (instruction->value.comparison.type) {
    case COMPARE_EQ:
      fprintf(file, "eq");
      break;
    case COMPARE_GE:
      fprintf(file, "ge");
      break;
    case COMPARE_LE:
      fprintf(file, "le");
      break;
    case COMPARE_GT:
      fprintf(file, "gt");
      break;
    case COMPARE_LT:
      fprintf(file, "lt");
      break;
    case COMPARE_NE:
      fprintf(file, "ne");
      break;
    default:
      PANIC("Unhandled comparison type: %d", instruction->value.comparison.type);
      break;
    }
    fprintf(file, " %%%zu, %%%zu",
            instruction->value.comparison.pair.car->id,
            instruction->value.comparison.pair.cdr->id);
    break;
  case IR_BRANCH:
    fprintf(file, "branch bb%zu", instruction->value.block->id);
    break;
  case IR_BRANCH_CONDITIONAL:
    fprintf(file, "branch.conditional %%%zu, bb%zu, bb%zu",
            instruction->value.conditional_branch.condition->id,
            instruction->value.conditional_branch.true_branch->id,
            instruction->value.conditional_branch.false_branch->id);
    break;
  case IR_PHI:
    fprintf(file, "phi");
    IRPhiArgument *arg = instruction->value.phi_argument;
    if (arg) {
      fprintf(file, " [bb%zu : %%%zu]",
              arg->block->id, arg->value->id);
      arg = arg->next;
    }
    for (; arg; arg = arg->next) {
      fprintf(file, ", [bb%zu : %%%zu]",
              arg->block->id, arg->value->id);
    }
    break;
  default:
    TODO("Handle IRType %d\n", instruction->type);
    break;
  }
  fputc('\n', file);
}

void ir_femit_block
(FILE *file,
 IRBlock *block
 )
{
  fprintf(file, "  bb%zu\n", block->id);
  for (IRInstruction *instruction = block->instructions;
       instruction;
       instruction = instruction->next
       ) {
    ir_femit_instruction(file, instruction);
  }
  ir_femit_instruction(file, block->branch);
}

void ir_femit_function
(FILE *file,
 IRFunction *function
 )
{
  fprintf(file, "f%zu\n", function->id);
  for (IRBlock *block = function->first;
       block;
       block = block->next
       ) {
    ir_femit_block(file, block);
  }

  fprintf(file, "return value: %%%zu\n", function->return_value->id);
}

void ir_femit
(FILE *file,
 CodegenContext *context
 )
{
  for (IRFunction *function = context->function;
       function;
       function = function->next
       ) {
    ir_femit_function(file, function);
  }
  putchar('\n');
}

void ir_set_ids(CodegenContext *context) {
  size_t function_id = 0;
  size_t block_id = 0;
  size_t instruction_id = 0;
  for (IRFunction *function = context->function;
       function;
       function = function->next
       ) {
    for (IRBlock *block = function->first;
         block;
         block = block->next
         ) {
      for (IRInstruction *instruction = block->instructions;
           instruction;
           instruction = instruction->next
           ) {
        instruction->id = ++instruction_id;
      }
      block->branch->id = ++instruction_id;
      block->id = ++block_id;
    }
    function->id = ++function_id;
  }
}

void ir_add_function_call_argument
(CodegenContext *context,
 IRInstruction *call,
 IRInstruction *argument
 )
{
  IRCallArgument *new_argument = calloc(1, sizeof(IRCallArgument));
  ASSERT(new_argument, "Could not allocate memory for new_argument.");

  new_argument->value = argument;

  if (!call->value.call.arguments) {
    call->value.call.arguments = new_argument;
  } else {
    IRCallArgument *arguments = call->value.call.arguments;
    for (; arguments->next; arguments = arguments->next);
    arguments->next = new_argument;
  }
}

IRBlock *ir_block_create() {
  IRBlock *block = calloc(1, sizeof(IRBlock));
  ASSERT(block, "Could not allocate memory for first IRBlock of new IRFunction.");
  return block;
}

void ir_phi_argument
(IRInstruction *phi,
 IRBlock *phi_predecessor,
 IRInstruction *argument
 )
{
  IRPhiArgument *phi_argument = calloc(1, sizeof(IRPhiArgument));
  phi_argument->block = phi_predecessor;
  phi_argument->value = argument;

  phi_argument->next = phi->value.phi_argument;
  phi->value.phi_argument = phi_argument;
}

IRInstruction *ir_phi(CodegenContext *context) {
  INSTRUCTION(phi, IR_PHI);
  INSERT(phi);
  return phi;
}

void ir_block_attach
(CodegenContext *context,
 IRBlock *new_block
 )
{
  IRFunction *function = context->function;
  ASSERT(function->last);
  function->last->next = new_block;
  new_block->previous = function->last;
  function->last = new_block;
  context->block = new_block;
}

IRFunction *ir_function_create() {
  IRFunction *function = calloc(1, sizeof(IRFunction));
  ASSERT(function, "Could not allocate memory for new IRFunction.");
  return function;
}

IRFunction *ir_function(CodegenContext *context) {
  IRFunction *function = ir_function_create();
  // A function *must* contain at least one block, so we start new
  // functions out with an empty block.
  IRBlock *block = ir_block_create();

  if (context->function) {
    IRFunction *last_function = context->function;
    for (; last_function->next; last_function = last_function->next);
    last_function->next = function;
  }
  context->function = function;

  function->first = block;
  function->last = block;
  context->block = block;

  return function;
}

IRInstruction *ir_immediate
(CodegenContext *context,
 int64_t immediate
 )
{
  INSTRUCTION(imm, IR_IMMEDIATE);
  imm->value.immediate = immediate;
  INSERT(imm);
  return imm;
}

IRInstruction *ir_load
(CodegenContext *context,
 IRInstruction *address
 )
{
  INSTRUCTION(load, IR_LOAD);
  load->value.reference = address;
  INSERT(load);
  return load;
}

IRInstruction *ir_direct_call
(CodegenContext *context,
 char* function_name
 )
{
  INSTRUCTION(call, IR_CALL);
  call->value.call.type = IR_CALLTYPE_DIRECT;
  call->value.call.value.name = function_name;
  INSERT(call);
  return call;
}

IRInstruction *ir_indirect_call
(CodegenContext *context,
 IRInstruction *function
 )
{
  INSTRUCTION(call, IR_CALL);
  call->value.call.type = IR_CALLTYPE_INDIRECT;
  call->value.call.value.callee = function;
  INSERT(call);
  return call;
}

IRInstruction *ir_load_global_address
(CodegenContext *context,
 char *name
 )
{
  INSTRUCTION(global_address, IR_GLOBAL_ADDRESS);
  global_address->value.name = name;
  INSERT(global_address);
  return global_address;
}

IRInstruction *ir_load_local_address
(CodegenContext *context,
 IRInstruction *local
 )
{
  INSTRUCTION(local_address, IR_LOCAL_ADDRESS);
  local_address->value.reference = local;
  INSERT(local_address);
  return local_address;
}

IRInstruction *ir_load_global
(CodegenContext *context,
 char *name
 )
{
  INSTRUCTION(global_load, IR_GLOBAL_LOAD);
  global_load->value.name = name;
  INSERT(global_load);
  return global_load;
}

IRInstruction *ir_load_local
(CodegenContext *context,
 IRInstruction *local
 )
{
  INSTRUCTION(local_load, IR_LOCAL_LOAD);
  local_load->value.reference = local;
  INSERT(local_load);
  return local_load;
}

IRInstruction *ir_store_global
(CodegenContext *context,
 IRInstruction *source,
 char *name
 )
{
  INSTRUCTION(global_store, IR_GLOBAL_STORE);
  global_store->value.global_assignment.new_value = source;
  global_store->value.global_assignment.name = name;
  INSERT(global_store);
  return global_store;
}

IRInstruction *ir_store_local
(CodegenContext *context,
 IRInstruction *source,
 IRInstruction *local
 )
{
  INSTRUCTION(local_store, IR_LOCAL_STORE);
  local_store->value.pair.car = local;
  local_store->value.pair.cdr = source;
  INSERT(local_store);
  return local_store;
}

IRInstruction *ir_store
(CodegenContext *context,
 IRInstruction *data,
 IRInstruction *address
 )
{
  TODO();
}

IRInstruction *ir_branch_conditional
(CodegenContext *context,
 IRInstruction *condition,
 IRBlock *then_block,
 IRBlock *otherwise_block
 )
{
  INSTRUCTION(branch, IR_BRANCH_CONDITIONAL);
  branch->value.conditional_branch.condition = condition;
  branch->value.conditional_branch.true_branch = then_block;
  branch->value.conditional_branch.false_branch = otherwise_block;
  context->block->branch = branch;
  return branch;
}

IRInstruction *ir_branch
(CodegenContext *context,
 IRBlock *destination
 )
{
  INSTRUCTION(branch, IR_BRANCH);
  branch->value.block = destination;
  context->block->branch = branch;
  return branch;
}

IRInstruction *ir_return(CodegenContext *context) {
  INSTRUCTION(branch, IR_RETURN);
  context->block->branch = branch;
  return branch;
}

IRInstruction *ir_copy
(CodegenContext *context,
 IRInstruction *source
 )
{
  TODO();
}

IRInstruction *ir_comparison
(CodegenContext *context,
 enum ComparisonType type,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  INSTRUCTION(cc, IR_COMPARISON);
  cc->value.comparison.type = type;
  cc->value.comparison.pair.car = lhs;
  cc->value.comparison.pair.cdr = rhs;
  INSERT(cc);
  return cc;
}

IRInstruction *ir_add
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  INSTRUCTION(add, IR_ADD);
  add->value.pair.car = lhs;
  add->value.pair.cdr = rhs;
  INSERT(add);
  return add;
}

IRInstruction *ir_subtract
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  INSTRUCTION(sub, IR_SUBTRACT);
  sub->value.pair.car = lhs;
  sub->value.pair.cdr = rhs;
  INSERT(sub);
  return sub;
}

IRInstruction *ir_multiply
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  TODO();
}

IRInstruction *ir_divide
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  TODO();
}

IRInstruction *ir_modulo
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  TODO();
}

IRInstruction *ir_shift_left
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  TODO();
}

IRInstruction *ir_shift_right_arithmetic
(CodegenContext *context,
 IRInstruction *lhs,
 IRInstruction *rhs
 )
{
  TODO();
}

IRInstruction *ir_stack_allocate
(CodegenContext *context,
 int64_t size
 )
{
  TODO();
}

#undef INSERT

