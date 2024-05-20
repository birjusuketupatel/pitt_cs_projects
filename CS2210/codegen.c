#include "proj2.h"
#include "proj3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define CODEFILE "code.s"

extern void yyparse();
extern void MkST(tree);

tree  SyntaxTree;   // root of syntax tree
FILE *treelst;      // syntax tree print file
FILE *asm_file;     // assembly code file

int global_offset;  // offset of class
int label_id;       // id of next label
int class_offset;   // offset within class

void GenProgram(tree);
void GenClass(tree);
void GenMethod(tree);
void GenDecl(tree);
int ArrayLength(tree);
void GenArray(tree);
int EvalExpression(tree);
void AssignRegs(tree, int);
void GenMethodBody(tree, int *);
void GenMethodDecl(tree, int *);
void GenStmt(tree);
void AllocClassDecl(tree);
void PushClassDecl(tree, int *);
void AssignmentStmt(tree);
void IfElseStmt(tree, int);
void LoopStmt(tree);
void ReturnStmt(tree);
void GenExpression(tree);
void GenVar(tree);
void MethodCall(tree);

int main (int argc, char *argv[]) {
  // construct syntax tree
  SyntaxTree = NULL;
  yyparse();
  if ( SyntaxTree == NULL ) {
      printf( "error: failed to create syntax tree\n" );
      exit(1);
  }

  // construct symbol table
  treelst = stdout;
  STInit();
  MkST(SyntaxTree);


  // create assembly file
  asm_file = fopen(CODEFILE, "wb");
  if (asm_file == NULL) {
    printf("error: failed to create file %s\n", CODEFILE);
  }

  // set offsets
  global_offset = 0;
  label_id = 0;
  class_offset = 0;

  fprintf(asm_file, ".data\n");
  fprintf(asm_file, "Enter:\t.asciiz \"\\n\"\n");
  fprintf(asm_file, "base:\n");
  fprintf(asm_file, ".text\n");
  fprintf(asm_file, ".globl main\n");

  GenProgram(SyntaxTree);

  // print symbol table
  STPrint();
  printtree(SyntaxTree, 0);

  fclose(asm_file);
  return 0;
} //main

void GenProgram (tree curr_node) {
  if (IsNull(curr_node)) {
    return;
  }

  int node_type = NodeOp(curr_node);
  if (node_type == ClassDefOp) {
    // put offset of next class in symbol table
    tree class_id_node = RightChild(curr_node);
    int class_id = IntVal(class_id_node);
    SetAttr(class_id, OFFSET_ATTR, global_offset);
    SetAttr(class_id, TYPE_ATTR, curr_node);

    // generate code for next class
    class_offset = 0;
    tree class_body_node = LeftChild(curr_node);
    GenClass(class_body_node);
    SetAttr(class_id, SIZE_ATTR, class_offset); // store size of class in symbol table
    global_offset += class_offset;
  }
  else {
    // continue left to right DFS
    GenProgram(LeftChild(curr_node));
    GenProgram(RightChild(curr_node));
  }
} //GenProgram

void GenClass (tree curr_node) {
  if (IsNull(curr_node)) {
    return;
  }

  // continue left to right DFS
  GenClass(LeftChild(curr_node));

  tree next_node = RightChild(curr_node);
  int node_type = NodeOp(next_node);

  if (node_type == MethodOp) {
    GenMethod(next_node);
  }
  else if (node_type == DeclOp) {
    fprintf(asm_file, ".data\n");
    GenDecl(next_node);
  }
} //GenClass

void GenMethod (tree curr_node) {
  tree name_node = LeftChild(LeftChild(curr_node));
  tree specop_node = RightChild(LeftChild(curr_node));

  char method_name[20];
  int method_id = IntVal(name_node);
  SetAttr(method_id, TYPE_ATTR, curr_node);
  SetAttr(method_id, OFFSET_ATTR, 0);
  strcpy(method_name, (char *) getname(GetAttr(method_id, NAME_ATTR)));

  fprintf(asm_file, ".text\n");

  if (strcmp(method_name, "main") == 0) {
    fprintf(asm_file, "main:\n");
  }
  else {
    fprintf(asm_file, "%s_%d:\n", method_name, method_id); // generate label for function
  }

  // gets class that defines this method
  while (method_id > 0 && GetAttr(method_id, KIND_ATTR) != CLASS) {
    method_id--;
  }

  int offset = GetAttr(method_id, OFFSET_ATTR);
  fprintf(asm_file,"\tmove\t$fp\t$sp\t\t#init fp pointer\n"); // initialize fp
  fprintf(asm_file, "\tsw\t$ra\t0($sp)\t\t#save return address on stack\n"); // push return address to stack
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase sp\n");

  AssignRegs(LeftChild(specop_node), 0);
  int local_offset = 4; // $ra is already on the stack
  GenMethodBody(RightChild(curr_node), &local_offset);

  fprintf(asm_file, "\tlw\t$ra\t0($fp)\t\t#get back control line\n"); // pop return address from stack
  fprintf(asm_file, "\tmove\t$sp\t$fp\t\t#pop stack to fp\n"); // pop stack down to fp
  fprintf(asm_file, "\tjr\t$ra\t\t\t#return from routine\n");
} //GenMethod

void GenMethodBody (tree curr_node, int *local_offset) {
  if (IsNull(curr_node)) {
    return;
  }

  // left to right DFS
  GenMethodBody(LeftChild(curr_node), local_offset);

  tree target_node = RightChild(curr_node);

  if (NodeOp(target_node) == DeclOp) {
    GenMethodDecl(target_node, local_offset);
  }
  else if (NodeOp(target_node) == StmtOp) {
    GenStmt(target_node);
  }
} //GenMethodBody

/*
 * assigns registers a0, a1, a2, a3 to each argument
 * stores register number in offset attribute of symbol table
 */
void AssignRegs (tree arg_node, int next_reg) {
  if (IsNull(arg_node)) {
    return;
  }

  int var_id = IntVal(LeftChild(LeftChild(arg_node)));
  SetAttr(var_id, OFFSET_ATTR, next_reg);
  AssignRegs(RightChild(arg_node), next_reg + 1);
} //AssignRegs

void GenMethodDecl (tree curr_node, int *local_offset) {
  if (IsNull(curr_node)) {
    return;
  }

  // get to leftmost declaration
  GenMethodDecl(LeftChild(curr_node), local_offset);

  tree var_node = LeftChild(RightChild(curr_node));
  tree type_node = LeftChild(LeftChild(RightChild(RightChild(curr_node))));
  tree val_node = RightChild(RightChild(RightChild(curr_node)));

  int var_id = IntVal(var_node);
  int symbol_type = GetAttr(var_id, KIND_ATTR);

  if (symbol_type == VAR) {
    if (NodeKind(type_node) == INTEGERTNode) {
      // integer declaration, sizeof(integer) = 4 bytes
      SetAttr(var_id, OFFSET_ATTR, (*local_offset));
      (*local_offset) += 4;

      // push variable on stack
      int value = EvalExpression(val_node);
      fprintf(asm_file, "\tli\t$t0\t%d\t\t#load immediate %d to t0\n", value, value);
      fprintf(asm_file, "\tsw\t$t0\t0($sp)\t\t#store t0 to top of stack\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
    }
    else if (NodeKind(type_node) == STNode) {
      // push object on stack
      tree class_decl = GetAttr(IntVal(type_node), TYPE_ATTR);
      SetAttr(var_id, OFFSET_ATTR, (*local_offset));
      PushClassDecl(LeftChild(class_decl), local_offset);
    }
  }
  else if (symbol_type == ARR) {
    SetAttr(var_id, OFFSET_ATTR, (*local_offset));
    int arr_length = ArrayLength(val_node);
    (*local_offset) += arr_length * 4;
    SetAttr(var_id, SIZE_ATTR, arr_length * 4);
    SetAttr(var_id, DIMEN_ATTR, 1);

    PushArray(val_node);
  }
} //GenMethodDecl

void PushClassDecl (tree curr_node, int * local_offset) {
  if (IsNull(curr_node)) {
    return;
  }

  // get to leftmost declaration
  PushClassDecl(LeftChild(curr_node), local_offset);

  tree next_node = RightChild(curr_node);
  int node_type = NodeOp(next_node);

  if (node_type == MethodOp) {
    return;
  }
  else if (node_type == DeclOp) {
    tree var_node = LeftChild(RightChild(next_node));
    tree type_node = LeftChild(LeftChild(RightChild(RightChild(next_node))));
    tree val_node = RightChild(RightChild(RightChild(next_node)));
    int var_id = IntVal(var_node);
    int symbol_type = GetAttr(var_id, KIND_ATTR);

    if (symbol_type == VAR) {
      if (NodeKind(type_node) == INTEGERTNode) {
        // integer declaration, sizeof(integer) = 4 bytes
        (*local_offset) += 4;

        // push variable on stack
        int value = EvalExpression(val_node);
        fprintf(asm_file, "\tli\t$t0\t%d\t\t#load immediate %d to t0\n", value, value);
        fprintf(asm_file, "\tsw\t$t0\t0($sp)\t\t#store t0 to top of stack\n");
        fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
      }
      else if (NodeKind(type_node) == STNode) {
        tree class_decl = (tree)GetAttr(IntVal(type_node), TYPE_ATTR);
        PushClassDecl(LeftChild(class_decl), local_offset);
      }
    }
    else if (symbol_type == ARR) {
      int arr_length = ArrayLength(val_node);
      (*local_offset) += arr_length * 4;
      PushArray(val_node);
    }
  }
} //PushClassDecl

void GenDecl (tree curr_node) {
  if (IsNull(curr_node)) {
    return;
  }

  // get to leftmost declaration
  GenDecl(LeftChild(curr_node));

  tree var_node = LeftChild(RightChild(curr_node));
  tree type_node = LeftChild(LeftChild(RightChild(RightChild(curr_node))));
  tree val_node = RightChild(RightChild(RightChild(curr_node)));

  int var_id = IntVal(var_node);
  int symbol_type = GetAttr(var_id, KIND_ATTR);
  char var_name[20];
  strcpy(var_name, (char *) getname(GetAttr(var_id, NAME_ATTR)));

  if (symbol_type == VAR) {
    if (NodeKind(type_node) == INTEGERTNode) {
      // integer declaration, sizeof(integer) = 4 bytes
      SetAttr(var_id, OFFSET_ATTR, class_offset);
      fprintf(asm_file, "var_%s_%d: .word\t%d\t\t#offset %d\n", var_name, var_id, EvalExpression(val_node), class_offset);
      class_offset += 4;
    }
    else if (NodeKind(type_node) == STNode) {
      // class declaration, sizeof(class) is stored in symbol table
      int class_size = GetAttr(IntVal(type_node), SIZE_ATTR);
      tree class_decl = GetAttr(IntVal(type_node), TYPE_ATTR);
      SetAttr(var_id, OFFSET_ATTR, class_offset);
      fprintf(asm_file, "var_%s_%d:\t\t\t#offset %d\n", var_name, var_id, class_offset);
      AllocClassDecl(LeftChild(class_decl));
      class_offset += class_size;
    }
  }
  else if (symbol_type == ARR) {
    // array declaration, sizeof(array) = sizeof(integer) * length(array)
    fprintf(asm_file, "var_%s_%d:\t\t\t#offset %d\n", var_name, var_id, class_offset);
    SetAttr(var_id, OFFSET_ATTR, class_offset);
    int arr_length = ArrayLength(val_node);
    class_offset += arr_length * 4;
    SetAttr(var_id, SIZE_ATTR, arr_length * 4);
    SetAttr(var_id, DIMEN_ATTR, 1);
    GenArray(val_node);
  }
} //GenDecl

void AllocClassDecl (tree curr_node) {
  if (IsNull(curr_node)) {
    return;
  }

  // get to leftmost declaration
  AllocClassDecl(LeftChild(curr_node));

  tree next_node = RightChild(curr_node);
  int node_type = NodeOp(next_node);

  if (node_type == MethodOp) {
    return;
  }
  else if (node_type == DeclOp) {
    tree var_node = LeftChild(RightChild(next_node));
    tree type_node = LeftChild(LeftChild(RightChild(RightChild(next_node))));
    tree val_node = RightChild(RightChild(RightChild(next_node)));
    int var_id = IntVal(var_node);
    int symbol_type = GetAttr(var_id, KIND_ATTR);

    if (symbol_type == VAR) {
      if (NodeKind(type_node) == INTEGERTNode) {
        fprintf(asm_file, "\t.word\t%d\n", EvalExpression(val_node));
      }
      else if (NodeKind(type_node) == STNode) {
        tree class_decl = (tree)GetAttr(IntVal(type_node), TYPE_ATTR);
        AllocClassDecl(LeftChild(class_decl));
      }
    }
    else if (symbol_type == ARR) {
      GenArray(val_node);
    }
  }
} //AllocClassDecl

void PushArray (tree head) {
  int length = ArrayLength(head);
  if (NodeOp(LeftChild(head)) == BoundOp) {
    // initialize array with zeros
    int i;
    for (i = 0; i < length; i++) {
      fprintf(asm_file, "\tsw\t$0\t0($sp)\t\t#store 0 to top of stack\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
    }
  }
  else if (NodeOp(LeftChild(head)) == CommaOp) {
    // initialize array with given values
    tree curr_node = LeftChild(head);
    int arr_vals[length];
    int i = 0;
    while (NodeOp(curr_node) == CommaOp) {
      arr_vals[i] = EvalExpression(RightChild(curr_node));
      curr_node = LeftChild(curr_node);
      i++;
    }
    i = length - 1;
    while(i >= 0) {
      fprintf(asm_file, "\tli\t$t0\t%d\t\t#load immediate %d to $t0\n", arr_vals[i], arr_vals[i]);
      fprintf(asm_file, "\tsw\t$t0\t0($sp)\t\t#store 0 to top of stack\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
      i--;
    }
  }
} //PushArray

void GenArray (tree head) {
  int length = ArrayLength(head);
  if (NodeOp(LeftChild(head)) == BoundOp) {
    // initialize array with zeros
    int i;
    for (i = 0; i < length; i++) {
      fprintf(asm_file, "\t.word\t0\n");
    }
  }
  else if (NodeOp(LeftChild(head)) == CommaOp) {
    // initialize array with given values
    tree curr_node = LeftChild(head);
    int arr_vals[length];
    int i = 0;
    while (NodeOp(curr_node) == CommaOp) {
      arr_vals[i] = EvalExpression(RightChild(curr_node));
      curr_node = LeftChild(curr_node);
      i++;
    }

    i = length - 1;
    while(i >= 0) {
      fprintf(asm_file, "\t.word\t%d\t\n", arr_vals[i]);
      i--;
    }
  }
} //GenArray

int ArrayLength (tree head) {
  int length = 0;
  if (NodeOp(LeftChild(head)) == BoundOp) {
    length = IntVal(RightChild(LeftChild(head)));
  }
  else if (NodeOp(LeftChild(head)) == CommaOp) {
    length = 1;
    tree curr_node = LeftChild(head);
    while (NodeOp(LeftChild(curr_node)) == CommaOp) {
      length++;
      curr_node = LeftChild(curr_node);
    }
  }

  return length;
} //ArrayLength

int EvalExpression (tree curr_node) {
  if (NodeKind(curr_node) == DUMMYNode) {
    return 0;
  }

  if (NodeKind(curr_node) == NUMNode) {
    return IntVal(curr_node);
  }

  int result = 0;
  int l = EvalExpression(LeftChild(curr_node));
  int r = EvalExpression(RightChild(curr_node));

  switch (NodeOp(curr_node)) {
    case AddOp:
      result = l + r;
      break;
    case SubOp:
      result = l - r;
      break;
    case MultOp:
      result = l * r;
      break;
    case DivOp:
      result = l / r;
      break;
    case UnaryNegOp:
      result = (-1) * l;
      break;
  }
  return result;
} //EvalExpression

void GenStmt (tree curr_node) {
  if (IsNull(curr_node)){
    return;
  }

  // continue left to right DFS
  GenStmt(LeftChild(curr_node));

  tree operation = RightChild(curr_node);

  if (NodeOp(operation) == AssignOp) {
    AssignmentStmt(operation);
  }
  else if (NodeOp(operation) == IfElseOp) {
    int next_label = label_id;
    label_id++;
    IfElseStmt(operation, next_label);
    fprintf(asm_file, "\tL_%d:\n", next_label);
  }
  else if (NodeOp(operation) == LoopOp) {
    LoopStmt(operation);
  }
  else if (NodeOp(operation) == ReturnOp) {
    ReturnStmt(operation);
  }
  else if (NodeOp(operation) == RoutineCallOp) {
    MethodCall(operation);
  }
} //GenStmt

void MethodCall (tree head) {
  // push $fp to stack
  // push $a0, $a1, $a2, $a3 to stack
  fprintf(asm_file, "\tsw\t$fp\t0($sp)\t\t#store $fp to top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
  fprintf(asm_file, "\tsw\t$a0\t0($sp)\t\t#store $a0 to top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
  fprintf(asm_file, "\tsw\t$a1\t0($sp)\t\t#store $a1 to top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
  fprintf(asm_file, "\tsw\t$a2\t0($sp)\t\t#store $a2 to top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
  fprintf(asm_file, "\tsw\t$a3\t0($sp)\t\t#store $a3 to top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");

  // get method name
  tree temp = LeftChild(head);
  while (!IsNull(RightChild(temp))) {
    temp = RightChild(temp);
  }
  while (NodeKind(temp) != STNode) {
    temp = LeftChild(temp);
  }

  tree method_node = temp;
  tree method_decl, arg_def_node, arg_node, name_node, temp_node;
  int arg_num, reg_number, var_id, arg_kind, to_print_id, print_label, num_val;
  int method_id = IntVal(method_node);
  char method_name[20];
  char *p;
  char to_print[100];
  strcpy(method_name, (char *) getname(GetAttr(method_id, NAME_ATTR)));

  if (strcmp(method_name, "println") == 0) {
    // println is a predefined method
    // get value to be printed
    arg_node = LeftChild(RightChild(head));

    if (NodeKind(arg_node) == STRINGNode) {
      // print string
      to_print_id = IntVal(arg_node);
      strcpy(to_print, (char *) getstring(to_print_id));
      p = to_print + 1;
      p[strlen(p) - 1] = 0;

      // put string in global memory
      print_label = label_id;
      label_id++;
      fprintf(asm_file, ".data\n");
      fprintf(asm_file, "S_%d: .asciiz \"%s\\n\"\n", print_label, p);
      fprintf(asm_file, ".text\n");

      // make syscall to print
      fprintf(asm_file, "\tla\t$a0\tS_%d\t\t#load address of S_%d to $a0\n", print_label, print_label);
      fprintf(asm_file, "\tli\t$v0\t4\t\t#load immediate 4 to $v0\n");
      fprintf(asm_file, "\tsyscall\n");
    }
    else if (NodeKind(arg_node) == NUMNode) {
      num_val = IntVal(arg_node);

      // make syscall to print
      fprintf(asm_file, "\tli\t$a0\tS_%d\t\t#load immediate %d to $a0\n", num_val);
      fprintf(asm_file, "\tli\t$v0\t4\t\t#load immediate 4 to $v0\n");
      fprintf(asm_file, "\tsyscall\n");

      // print newline
      fprintf(asm_file, "\tla\t$a0\tEnter\t\t#load address of Enter to $a0\n", print_label, print_label);
      fprintf(asm_file, "\tli\t$v0\t4\t\t#load immediate 4 to $v0\n");
      fprintf(asm_file, "\tsyscall\n");
    }
    else {
      // print variable
      GenExpression(arg_node);

      // get variable from address at top of stack
      // put variable in $a0
      fprintf(asm_file, "\tlw\t$a0\t4($sp)\t\t#load word from top of stack to $t0\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");

      // make syscall to print
      fprintf(asm_file, "\tli\t$v0\t1\t\t#load immediate 5 to $v0\n");
      fprintf(asm_file, "\tsyscall\n");

      // print newline
      fprintf(asm_file, "\tla\t$a0\tEnter\t\t#load address of Enter to $a0\n", print_label, print_label);
      fprintf(asm_file, "\tli\t$v0\t4\t\t#load immediate 4 to $v0\n");
      fprintf(asm_file, "\tsyscall\n");
    }
  }
  else if (strcmp(method_name, "readln") == 0) {
    // readln is a predefined method
    arg_node = LeftChild(RightChild(head));

    GenVar(arg_node);

    // get variable from address at top of stack
    // put variable in $t0
    fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");

    // make syscall to read integer
    fprintf(asm_file, "\tli\t$v0\t5\t\t#load immediate 5 to $v0\n");
    fprintf(asm_file, "\tsyscall\n");

    // put integer in target address
    fprintf(asm_file, "\tsw\t$v0\t($t0)\t\t#store word $v0 at $t0\n");
  }
  else {
    // user defined method

    // TODO: copy object parameters

    // put arguments in registers
    tree method_decl = GetAttr(method_id, TYPE_ATTR);
    arg_def_node = LeftChild(RightChild(LeftChild(method_decl)));
    arg_node = RightChild(head);
    arg_num = 0;
    while (!IsNull(arg_node)) {
      name_node = LeftChild(LeftChild(arg_def_node));
      var_id = IntVal(name_node);
      arg_kind = GetAttr(var_id, KIND_ATTR);
      reg_number = GetAttr(var_id, OFFSET_ATTR);

      if (arg_kind == REF_ARG) {
        GenVar(LeftChild(arg_node));
      }
      else if (arg_kind == VALUE_ARG) {
        GenExpression(LeftChild(arg_node));
      }

      // pop value from top of stack
      fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");

      // put value in argument register
      fprintf(asm_file, "\tmove\t$a%d\t$t0\t\t#$a%d = $t0\n", reg_number, reg_number);

      arg_node = RightChild(arg_node);
      arg_def_node = RightChild(arg_def_node);
    }

    // jump to method
    fprintf(asm_file, "\tjal %s_%d\n", method_name, method_id);
  }

  // restore $fp, $a0, $a1, $a2, $a3
  fprintf(asm_file, "\tlw\t$a3\t4($sp)\t\t#load $a3 from top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
  fprintf(asm_file, "\tlw\t$a2\t4($sp)\t\t#load $a2 from top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
  fprintf(asm_file, "\tlw\t$a1\t4($sp)\t\t#load $a1 from top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
  fprintf(asm_file, "\tlw\t$a0\t4($sp)\t\t#load $a0 from top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
  fprintf(asm_file, "\tlw\t$fp\t4($sp)\t\t#load $fp from top of stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
} //MethodCall

void ReturnStmt (tree head) {
  tree expr = LeftChild(head);

  if (!IsNull(expr)) {
    GenExpression(expr);

    // get result of expression from stack
    fprintf(asm_file, "\tlw\t$v0\t4($sp)\t\t#load word from top of stack to $v0\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");
  }

  // pop return address from stack and reset stack
  fprintf(asm_file, "\tlw\t$ra\t0($fp)\t\t#get back control line\n");
  fprintf(asm_file, "\tmove\t$sp\t$fp\t\t#pop stack to fp\n");
  fprintf(asm_file, "\tjr\t$ra\t\t\t#return from routine\n");
} //ReturnStmt

void LoopStmt (tree head) {
  int main_label = label_id;
  label_id++;
  int true_label = label_id;
  label_id++;
  int false_label = label_id;
  label_id++;

  fprintf(asm_file, "\tL_%d:\n", main_label);

  GenExpression(LeftChild(head));

  // get result of expression from stack
  fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");

  // branch to true_label if $t0 != 0
  fprintf(asm_file, "\tbnez\t$t0\tL_%d\t\t#branch to L_%d if $t0 != 0\n", true_label, true_label);

  // branch to false_label otherwise
  fprintf(asm_file, "\tb\tL_%d\t\t\t#branch to L_%d\n", false_label, false_label);

  fprintf(asm_file, "\tL_%d:\n", true_label);

  GenStmt(RightChild(head));

  // branch to main_label
  fprintf(asm_file, "\tb\tL_%d\t\t\t#branch to L_%d\n", main_label, main_label);

  fprintf(asm_file, "\tL_%d:\n", false_label);
} //LoopStmt

void IfElseStmt (tree curr_node, int next_label) {
  if (IsNull(curr_node)) {
    return;
  }

  int true_label = label_id;
  label_id++;
  int false_label = label_id;
  label_id++;

  // left to right DFS
  IfElseStmt(LeftChild(curr_node), next_label);

  if (NodeOp(RightChild(curr_node)) == CommaOp) {
    // if section, condition present
    tree stmt_node = RightChild(RightChild(curr_node));
    tree expr_node = LeftChild(RightChild(curr_node));
    GenExpression(expr_node);

    // get result of expression from stack
    fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");

    // branch to true_label if $t0 != 0
    fprintf(asm_file, "\tbnez\t$t0\tL_%d\t\t#branch to L_%d if $t0 != 0\n", true_label, true_label);

    // branch to false_label otherwise
    fprintf(asm_file, "\tb\tL_%d\t\t\t#branch to L_%d\n", false_label);

    fprintf(asm_file, "\tL_%d:\n", true_label); // true_label

    GenStmt(stmt_node);

    // branch to next_label
    fprintf(asm_file, "\tb\tL_%d\t\t\t#branch to L_%d\n", next_label, next_label);

    fprintf(asm_file, "\tL_%d:\n", false_label);
  }
  else if (NodeOp(RightChild(curr_node)) == StmtOp) {
    // else section, no condition present
    tree stmt_node = RightChild(curr_node);
    GenStmt(stmt_node);
    fprintf(asm_file, "\tb\tL_%d\t\t\t#branch to L_%d\n", next_label, next_label);
  }
} //IfElseStmt

void GenVar (tree head) {
  tree name_node = LeftChild(head);
  tree select_node, base_node;
  int var_id = IntVal(name_node);
  int nest_level = GetAttr(var_id, NEST_ATTR);
  char var_name[20];
  strcpy(var_name, (char *) getname(GetAttr(var_id, NAME_ATTR)));
  int offset = 0;

  int var_kind = GetAttr(var_id, KIND_ATTR);

  if (var_kind == CLASS) {
    select_node = RightChild(head);
    base_node = LeftChild(LeftChild(select_node));
    var_id = IntVal(base_node);
    strcpy(var_name, (char *) getname(GetAttr(var_id, NAME_ATTR)));
    fprintf(asm_file, "\tla\t$t0\tvar_%s_%d\t\t#load base address to $t0\n", var_name, var_id);
    head = select_node;
  }
  else if (var_kind == REF_ARG) {
    int reg_number = GetAttr(var_id, OFFSET_ATTR);
    fprintf(asm_file, "\tmove\t$t0\t$a%d\t\t#move $a%d to $t0\n", reg_number, reg_number);
  }
  else if (nest_level == 1) {
    // global variable, base = label address
    fprintf(asm_file, "\tla\t$t0\tvar_%s_%d\t\t#load base address to $t0\n", var_name, var_id);
  }
  else if (nest_level == 2) {
    // local variable, base = $fp - offset(var)
    offset = GetAttr(var_id, OFFSET_ATTR);
    fprintf(asm_file, "\tmove\t$t0\t$fp\t\t#move $fp to $t0\n");
    fprintf(asm_file, "\tli\t$t1\t%d\t\t#load immediate %d to $t1\n", offset, offset);
    fprintf(asm_file, "\tsub\t$t0\t$t0\t$t1\t#subtract $t1 from $t0\n", offset, offset);
  }

  // push base to stack
  fprintf(asm_file, "\tsw\t$t0\t0($sp)\t\t#push $t0 to stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");

  // push initial offset (0) to stack
  fprintf(asm_file, "\tsw\t$0\t0($sp)\t\t#push 0 to stack\n");
  fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");

  select_node = RightChild(head);
  while (!IsNull(select_node)) {
    if (NodeOp(LeftChild(select_node)) == FieldOp) {
      // add field's offset to current offset
      int field_id = IntVal(LeftChild(LeftChild(select_node)));
      offset = GetAttr(field_id, OFFSET_ATTR);
      fprintf(asm_file, "\tli\t$t1\t%d\t\t#load immediate %d to $t1\n", offset, offset);
      fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
      fprintf(asm_file, "\tadd\t$t0\t$t0\t$t1\t#$t0 = $t0 + $t1\n");
      fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store word from $t0 to top of stack\n");
    }
    else if (NodeOp(LeftChild(select_node)) == IndexOp) {
      // add index's offset to current offset
      GenExpression(LeftChild(LeftChild(select_node)));
      fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");
      fprintf(asm_file, "\tli\t$t0\t4\t\t#load immediate 4 to $t0\n");
      fprintf(asm_file, "\tmult\t$t0\t$t1\t\t#multiply $t0 and $t1\n");
      fprintf(asm_file, "\tmflo\t$t1\t\t\t#store result in $t1\n");
      fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
      fprintf(asm_file, "\tadd\t$t0\t$t0\t$t1\t#$t0 = $t0 + $t1\n");
      fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store $t0 to top of stack\n");
    }

    select_node = RightChild(select_node);
  }

  if (nest_level == 1 || var_kind == CLASS) {
    // add base and offset, put result on stack
    fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");
    fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
    fprintf(asm_file, "\tadd\t$t0\t$t0\t$t1\t#$t0 = $t0 + $t1\n");
    fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store $t0 to top of stack\n");
  }
  else if (nest_level == 2) {
    // subtract base from offset, put result on stack
    fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#pop stack\n");
    fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
    fprintf(asm_file, "\tsub\t$t0\t$t0\t$t1\t#$t0 = $t0 - $t1\n");
    fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store $t0 to top of stack\n");
  }
} //GenVar

void GenExpression (tree curr_node) {
  int num_val, var_id, var_kind, reg_number;
  tree name_node;

  if (NodeKind(curr_node) == NUMNode) {
    num_val = IntVal(curr_node);
    fprintf(asm_file, "\tli\t$t0\t%d\t\t#load immediate %d to $t0\n", num_val, num_val);
    fprintf(asm_file, "\tsw\t$t0\t0($sp)\t\t#store $t0 to top of stack\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
    return;
  }

  switch (NodeOp(curr_node)) {
    case AddOp:
    case SubOp:
    case MultOp:
    case DivOp:
    case LTOp:
    case GTOp:
    case EQOp:
    case NEOp:
    case LEOp:
    case GEOp:
      GenExpression(LeftChild(curr_node));
      GenExpression(RightChild(curr_node));

      // get two values at the top of the stack
      fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
      fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");

      switch (NodeOp(curr_node)) {
        case AddOp:
          fprintf(asm_file, "\tadd\t$t0\t$t0\t$t1\t#$t0 = $t0 + $t1\n");
          break;
        case SubOp:
          fprintf(asm_file, "\tsub\t$t0\t$t0\t$t1\t#$t0 = $t0 - $t1\n");
          break;
        case MultOp:
          fprintf(asm_file, "\tmult\t$t0\t$t1\t\t#$t0 * $t1\n");
          fprintf(asm_file, "\tmflo\t$t0\t\t\t#store result in $t0\n");
          break;
        case DivOp:
          fprintf(asm_file, "\div\t$t0\t$t1\t\t#$t0 / $t1\n");
          fprintf(asm_file, "\tmflo\t$t0\t\t\t#store result in $t0\n");
          break;
        case LTOp:
          fprintf(asm_file, "\tslt\t$t0\t$t0\t$t1\t\t#$t0 < $t1\n");
          break;
        case GTOp:
          fprintf(asm_file, "\tsgt\t$t0\t$t0\t$t1\t#$t0 > $t1\n");
          break;
        case EQOp:
          fprintf(asm_file, "\tseq\t$t0\t$t0\t$t1\t#$t0 == $t1\n");
          break;
        case NEOp:
          fprintf(asm_file, "\tsne\t$t0\t$t0\t$t1\t#$t0 != $t1\n");
          break;
        case LEOp:
          fprintf(asm_file, "\tsle\t$t0\t$t0\t$t1\t#$t0 <= $t1\n");
          break;
        case GEOp:
          fprintf(asm_file, "\tsge\t$t0\t$t0\t$t1\t#$t0 >= $t1\n");
          break;
      }

      // store result on stack
      fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store word from $t0 to top of stack\n");
      break;
    case UnaryNegOp:
      GenExpression(LeftChild(curr_node));
      // get value from top of stack
      fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");

      //calculate result and store on stack
      fprintf(asm_file, "\tli\t$t1\t-1\t\t#load immediate -1 to $t1\n");
      fprintf(asm_file, "\tmult\t$t0\t$t1\t\t#$t0 * $t1\n");
      fprintf(asm_file, "\tmflo\t$t0\t\t\t#store result in $t0\n");
      fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store word from $t0 to top of stack\n");
      break;
    case VarOp:
      name_node = LeftChild(curr_node);
      var_id = IntVal(name_node);
      var_kind = GetAttr(var_id, KIND_ATTR);
      if (var_kind == VALUE_ARG) {
        reg_number = GetAttr(var_id, OFFSET_ATTR);
        // push value of argument register to stack
        fprintf(asm_file, "\tsw\t$a%d\t0($sp)\t\t#store word from $a%d to top of stack\n", reg_number, reg_number);
        fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
      }
      else {
        GenVar(curr_node);

        // get address from top of stack
        fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
        fprintf(asm_file, "\tlw\t$t0\t($t1)\t\t#load word from $t1 to $t0\n");
        fprintf(asm_file, "\tsw\t$t0\t4($sp)\t\t#store word from $t0 to top of stack\n");
      }

      break;
    case RoutineCallOp:
      MethodCall(curr_node);

      // put $v0 on top of stack
      fprintf(asm_file, "\tsw\t$v0\t0($sp)\t\t#push $v0 to top of stack\n");
      fprintf(asm_file, "\taddi\t$sp\t$sp\t-4\t#increase stack\n");
      break;
  }
} //GenExpression

void AssignmentStmt (tree head) {
  tree expr = RightChild(head);
  tree var_op = RightChild(LeftChild(head));

  tree name_node = LeftChild(var_op);
  int var_id = IntVal(name_node);
  int var_kind = GetAttr(var_id, KIND_ATTR);
  int reg_number;

  if (var_kind == VALUE_ARG) {
    reg_number = GetAttr(var_id, OFFSET_ATTR);
    GenExpression(expr);

    // pop value from top of stack
    fprintf(asm_file, "\tlw\t$a%d\t4($sp)\t\t#load word from top of stack to $a%d\n", reg_number, reg_number);
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");
  }
  else {
    GenVar(var_op);
    GenExpression(expr);

    // pop value from top of stack
    fprintf(asm_file, "\tlw\t$t1\t4($sp)\t\t#load word from top of stack to $t1\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");

    // pop address from top of stack
    fprintf(asm_file, "\tlw\t$t0\t4($sp)\t\t#load word from top of stack to $t0\n");
    fprintf(asm_file, "\taddi\t$sp\t$sp\t4\t#decrease stack\n");

    // store word at target address
    fprintf(asm_file, "\tsw\t$t1\t($t0)\t\t#store word from $t1 to $t0\n");
  }
} //AssignmentStmt
