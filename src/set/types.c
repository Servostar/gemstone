//
// Created by servostar on 6/7/24.
//

#include <mem/cache.h>
#include <set/types.h>

void delete_box(BoxType *box) {
  g_hash_table_destroy(box->member);
  mem_free(box);
}

static void delete_box_table(GHashTable *box_table) {
  GHashTableIter iter;
  char *name = NULL;
  BoxType *box = NULL;

  g_hash_table_iter_init(&iter, box_table);

  while (g_hash_table_iter_next(&iter, (gpointer)&name, (gpointer)&box)) {
    delete_box(box);
    mem_free(name);
  }

  g_hash_table_destroy(box_table);
}

static void delete_imports(GArray *imports) { g_array_free(imports, TRUE); }

void delete_box_member(BoxMember *member) {
  member->box = NULL;
  delete_expression(member->initalizer);
  delete_type(member->type);
  mem_free((void *)member->name);
}

void delete_box_type(BoxType *box_type) {
  GHashTableIter iter;
  char *name = NULL;
  BoxMember *member = NULL;

  g_hash_table_iter_init(&iter, box_type->member);

  while (g_hash_table_iter_next(&iter, (gpointer)&name, (gpointer)&member)) {
    delete_box_member(member);
    mem_free(name);
  }

  g_hash_table_destroy(box_type->member);
}

void delete_composite([[maybe_unused]] CompositeType *composite) {}

void delete_type(Type *type) {
  switch (type->kind) {
  case TypeKindBox:
    delete_box_type(type->impl.box);
    break;
  case TypeKindReference:
    delete_type(type->impl.reference);
    break;
  case TypeKindPrimitive:
    break;
  case TypeKindComposite:
    delete_composite(&type->impl.composite);
    break;
  }
}

static void delete_type_table(GHashTable *type_table) {

  GHashTableIter iter;
  char *name = NULL;
  Type *type = NULL;

  g_hash_table_iter_init(&iter, type_table);

  while (g_hash_table_iter_next(&iter, (gpointer)&name, (gpointer)&type)) {
    delete_type(type);
    mem_free(name);
  }

  g_hash_table_destroy(type_table);
}

void delete_box_access(BoxAccess *access) {
  delete_variable(access->variable);

  for (guint i = 0; i < access->member->len; i++) {
    delete_box_member(g_array_index(access->member, BoxMember *, i));
  }

  g_array_free(access->member, TRUE);
}

void delete_variable(Variable *variable) {
  switch (variable->kind) {
  case VariableKindBoxMember:
    delete_box_access(&variable->impl.member);
    break;
  case VariableKindDeclaration:
    delete_declaration(&variable->impl.declaration);
    break;
  case VariableKindDefinition:
    delete_definition(&variable->impl.definiton);
    break;
  }
}

void delete_declaration(VariableDeclaration *decl) { delete_type(decl->type); }

void delete_definition(VariableDefiniton *definition) {
  delete_declaration(&definition->declaration);
  delete_expression(definition->initializer);
}

void delete_type_value(TypeValue *value) {
  delete_type(value->type);
  mem_free(value);
}

void delete_operation(Operation *operation) {
  for (guint i = 0; i < operation->operands->len; i++) {
    delete_expression(g_array_index(operation->operands, Expression *, i));
  }

  g_array_free(operation->operands, TRUE);
}

void delete_transmute(Transmute *trans) {
  delete_expression(trans->operand);
  delete_type(trans->targetType);
}

void delete_typecast(TypeCast *cast) {
  delete_expression(cast->operand);
  delete_type(cast->targetType);
}

void delete_expression(Expression *expr) {
  delete_type(expr->result);

  switch (expr->kind) {
  case ExpressionKindConstant:
    delete_type_value(&expr->impl.constant);
    break;
  case ExpressionKindOperation:
    delete_operation(&expr->impl.operation);
    break;
  case ExpressionKindTransmute:
    delete_transmute(&expr->impl.transmute);
    break;
  case ExpressionKindTypeCast:
    delete_typecast(&expr->impl.typecast);
    break;
  case ExpressionKindVariable:
    delete_variable(expr->impl.variable);
  default:
    //TODO free Reference and AddressOf
    break;
  }
}

static void delete_variable_table(GHashTable *variable_table) {

  GHashTableIter iter;
  char *name = NULL;
  Variable *variable = NULL;

  g_hash_table_iter_init(&iter, variable_table);

  while (g_hash_table_iter_next(&iter, (gpointer)&name, (gpointer)&variable)) {
    delete_variable(variable);
    mem_free(name);
  }

  g_hash_table_destroy(variable_table);
}

void delete_module(Module *module) {

  delete_box_table(module->boxes);
  delete_imports(module->imports);
  delete_type_table(module->types);
  delete_variable_table(module->variables);

  mem_free(module);
}
