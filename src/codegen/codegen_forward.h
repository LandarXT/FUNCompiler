#ifndef CODEGEN_FORWARD_H
#define CODEGEN_FORWARD_H

typedef struct IRInstruction IRInstruction;
typedef struct IRBlock IRBlock;
typedef struct IRFunction IRFunction;

typedef int RegisterDescriptor;

typedef struct Register Register;
typedef struct RegisterPool RegisterPool;
typedef struct CodegenContext CodegenContext;

enum CodegenAssemblyDialect {
  CG_ASM_DIALECT_ATT,
  CG_ASM_DIALECT_INTEL,
  CG_ASM_DIALECT_COUNT,

  CG_ASM_DIALECT_DEFAULT = CG_ASM_DIALECT_ATT
};

enum CodegenOutputFormat {
  CG_FMT_x86_64_GAS,
  CG_FMT_COUNT,

  CG_FMT_DEFAULT = CG_FMT_x86_64_GAS,
};

enum CodegenCallingConvention {
  CG_CALL_CONV_MSWIN,
  CG_CALL_CONV_LINUX,
  CG_CALL_CONV_COUNT,

  CG_CALL_CONV_DEFAULT = CG_CALL_CONV_MSWIN,
};

// Types of comparison to be implemented by codegen backend.
enum ComparisonType {
  COMPARE_EQ,
  COMPARE_NE,
  COMPARE_LT,
  COMPARE_LE,
  COMPARE_GT,
  COMPARE_GE,

  COMPARE_COUNT,
};

#endif // CODEGEN_FORWARD_H
