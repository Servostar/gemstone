#include <complex.h>
#include <stdio.h>
#include <ast/ast.h>
#include <set/types.h>
#include <stdlib.h>
#include <sys/log.h>
#include <glib.h>


GHashTable *declaredComposites;//pointer to composites with names, 
GHashTable *declaredBoxes;//pointer to typeboxes
GArray *Scope;//list of hashtables. last Hashtable is current depth of program. hashtable key: ident, value: Variable* to var

const Type ShortShortUnsingedIntType = {
    .kind = TypeKindComposite,
    .impl = {
        .composite = {
            .sign = Unsigned,
            .scale = 0.25,
            .primitive = Int
        }
    },
    .nodePtr = NULL,
};

const Type StringLiteralType = {
    .kind = TypeKindReference,
    .impl = {
        .reference = (ReferenceType) &ShortShortUnsingedIntType,
    },
    .nodePtr = NULL,
};


Type *findType(AST_NODE_PTR currentNode){

    const char *typekind = currentNode->children[currentNode->child_count -1]->value;
    if (0 == strcmp(typekind, "int")||0 == strcmp(typekind, "float")){
        
        Type *type =  malloc(sizeof(Type));
        type->nodePtr =  currentNode;
        if(AST_Typekind != currentNode->children[0]->kind){
            DEBUG("Type is a Composite");
            type->kind = TypeKindComposite;

     
            CompositeType composite;
            composite.nodePtr = currentNode;


            if(0 == strcmp(typekind, "int")){
                composite.primitive = Int;
            }else{
                composite.primitive = Float;
            }
            composite.sign = Signed;

            size_t scalelist = 0;
            if(AST_Sign == currentNode->children[0]->kind){
            if(0 == strcmp(currentNode->children[0]->value, "unsigned")){
                composite.sign = Unsigned;
                }
                scalelist = 1;
            }

            composite.scale = 1.0;
            if(AST_List == currentNode->children[scalelist]->kind){
                for (size_t i = 0; i < currentNode->children[scalelist]->child_count; i++){
                    if (0 == strcmp(currentNode->children[scalelist]->children[i]->value, "short") || 0 == strcmp(currentNode->children[scalelist]->children[i]->value, "half")){
                        composite.scale /= 2;
                    }else{
                        composite.scale *= 2; 
                    }
                    if (0.25 > composite.scale  || 8 > composite.scale) {
                        //TODO scale not right
                    }
                }
            }

            type->impl.composite = composite;


        }else{
            type->kind = TypeKindPrimitive;
            if(0 == strcmp(typekind, "int")){
                type->impl.primitive = Int;
            }else{
                type->impl.primitive = Float;
            }
            return type;
        }
    }else if(g_hash_table_contains(declaredBoxes, typekind)){
        if(AST_Typekind != currentNode->children[0]->kind){
            //TODO composite Box try
        }
        return (Type *) g_hash_table_lookup(declaredBoxes, typekind);
    }else if(g_hash_table_contains(declaredComposites, typekind)){
        if(AST_Typekind != currentNode->children[0]->kind){
            Type *composite = malloc(sizeof(Type));
            
            *composite =  *(Type*) g_hash_table_lookup(declaredComposites, typekind);
            


            size_t scalelist = 0;
            if(AST_Sign == currentNode->children[0]->kind){
                if(0 == strcmp(currentNode->children[0]->value, "unsigned")){
                    composite->impl.composite.sign = Unsigned;
                }else if(0 == strcmp(currentNode->children[0]->value, "unsigned")){
                    composite->impl.composite.sign = Signed;
                }
                scalelist = 1;
            }


            if(AST_List == currentNode->children[scalelist]->kind){
                for (size_t i = 0; i < currentNode->children[scalelist]->child_count; i++){
                    if (0 == strcmp(currentNode->children[scalelist]->children[i]->value, "short") || 0 == strcmp(currentNode->children[scalelist]->children[i]->value, "half")){
                        composite->impl.composite.scale /= 2;
                    }else{
                        composite->impl.composite.scale *= 2; 
                    }
                    if (0.25 > composite->impl.composite.scale  || 8 > composite->impl.composite.scale) {
                        //TODO scale not right
                        return NULL;
                    }
                }
            }
            return composite;
        }
        return (Type *) g_hash_table_lookup(declaredComposites, typekind);
    }else{
        //TODO doesnt know typekind 
        return NULL;
    }
    return NULL;
}



StorageQualifier Qualifier_from_string(const char *str) {
    if (!strncmp(str, "local", 5)) return Local;
    if (!strncmp(str, "static", 6)) return Static;
    if (!strncmp(str, "global", 6)) return Global;
    PANIC("Provided string is not a storagequalifier: %s", str);
}

Variable **createDecl(AST_NODE_PTR currentNode){
    DEBUG("create declaration");
    Variable **variables = malloc(currentNode->children[currentNode->child_count -1]->child_count * sizeof(Variable*));

    VariableDeclaration decl;
    decl.nodePtr = currentNode;

    DEBUG("Child Count: %i", currentNode->child_count);
    for (size_t i = 0; i < currentNode->child_count; i++){
    
        switch(currentNode->children[i]->kind){
        case AST_Storage:
            DEBUG("fill Qualifier");
            decl.qualifier = Qualifier_from_string(currentNode->children[i]->value);
            break;
        case AST_Type:
            DEBUG("fill Type");
            decl.type = findType(currentNode->children[i]);
            break;
        case AST_IdentList:
            for(size_t i = 0; i < currentNode->children[currentNode->child_count -1]->child_count; i++){
                Variable *variable = malloc(sizeof(Variable));
                variable->kind = VariableKindDeclaration;
                variable->nodePtr = currentNode;
                variable->name = currentNode->children[currentNode->child_count -1]->children[i]->value;
                variable->impl.declaration = decl;
                variables[i] = variable;
            }   
            break;
        default:
        //TODO PANIC maybe
            break;
        }
    }
    
    return variables;
}

Variable* getVariableFromScope(const char* name){
    for(size_t i = 0; i < Scope->len; i++){
        if(g_hash_table_contains(((GHashTable **) Scope->data)[i], name))
        {
            return g_hash_table_lookup(((GHashTable**)Scope->data)[i], name);
        }
    }
    return NULL;
}



int fillTablesWithVars(GHashTable *variableTable,GHashTable *currentScope , Variable** content, size_t amount){
    DEBUG("filling vars in scope and table");
    for(size_t i = 0; i < amount; i++){
        if(!(NULL == getVariableFromScope(content[i]->name))){
            DEBUG("this var already exist: ",content[i]->name);
            return 1;
        }
        g_hash_table_insert(variableTable, (gpointer) content[i]->name, content[i] );   
        g_hash_table_insert(currentScope, (gpointer) content[i]->name, content[i] );
    }
    return 0;
}

Variable **createDef(AST_NODE_PTR currentNode){

}



TypeValue createTypeValue(AST_NODE_PTR currentNode){
    TypeValue value;
    Type *type = malloc(sizeof(Type));
    value.type = type;
    type->kind = TypeKindPrimitive;
    type->nodePtr = currentNode;

    switch (currentNode->kind) {
   
        case AST_Int:
            type->impl.primitive = Int;
        case AST_Float:
            type->impl.primitive = Int;
        default:
        PANIC("Node is not an expression but from kind: %i", currentNode->kind);
            break;
        }
    
    value.nodePtr = currentNode;
    value.value = currentNode->value;
    return value;
}

TypeValue createString(AST_NODE_PTR currentNode){
    TypeValue value;
    Type *type =(Type*) &StringLiteralType;
    value.type = type;
    value.nodePtr = currentNode;
    value.value = currentNode->value; 
    return value;
}

Expression *createExpression(AST_NODE_PTR currentNode);


Type* createTypeFromOperands(Type* LeftOperandType, Type* RightOperandType, AST_NODE_PTR currentNode){
    Type *result = malloc(sizeof(Type));
    result->nodePtr = currentNode;

    
    if(LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindComposite)
    {
        result->kind = TypeKindComposite;
        CompositeType resultImpl;

        resultImpl.nodePtr = currentNode;
        resultImpl.sign = MAX(LeftOperandType->impl.composite.sign, RightOperandType->impl.composite.sign);
        resultImpl.scale = MAX(LeftOperandType->impl.composite.scale, RightOperandType->impl.composite.scale);
        resultImpl.primitive = MAX(LeftOperandType->impl.composite.primitive , RightOperandType->impl.composite.primitive);
        
        result->impl.composite = resultImpl;

        
    } else if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindPrimitive){
        result->kind = TypeKindPrimitive;
        
        result->impl.primitive = MAX(LeftOperandType->impl.primitive , RightOperandType->impl.primitive);

    } else if(LeftOperandType->kind == TypeKindPrimitive && RightOperandType->kind == TypeKindComposite){
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX( 1.0, RightOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, RightOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;

    } else if(LeftOperandType->kind == TypeKindComposite && RightOperandType->kind == TypeKindPrimitive){
        result->kind = TypeKindComposite;

        result->impl.composite.sign = Signed;
        result->impl.composite.scale = MAX( 1.0, LeftOperandType->impl.composite.scale);
        result->impl.composite.primitive = MAX(Int, LeftOperandType->impl.composite.primitive);
        result->impl.composite.nodePtr = currentNode;
    }else{
        return NULL;
    }
    return result;
}

int createArithOperation(Expression* ParentExpression, AST_NODE_PTR currentNode, size_t expectedChildCount){
    
    ParentExpression->impl.operation.kind = Arithmetic;
    ParentExpression->impl.operation.nodePtr = currentNode;
    if (expectedChildCount > currentNode->child_count){
        PANIC("Operation has to many children");
    }
    for (size_t i = 0; i < currentNode->child_count; i++){
        Expression* expression = createExpression(currentNode->children[i]);
        if(NULL == expression){
            return 1;
        }
        g_array_append_val(ParentExpression->impl.operation.operands , expression);

    }

    switch (currentNode->kind){
        case AST_Add:
            ParentExpression->impl.operation.impl.arithmetic = Add;
        case AST_Sub:
            ParentExpression->impl.operation.impl.arithmetic = Sub;
        case AST_Mul:
            ParentExpression->impl.operation.impl.arithmetic = Mul;
        case AST_Div:
            ParentExpression->impl.operation.impl.arithmetic = Div;
        case AST_Negate:
            ParentExpression->impl.operation.impl.arithmetic = Negate;
        default:
            PANIC("Current node is not an arithmetic operater");
        break;
    }


    if(ParentExpression->impl.operation.impl.arithmetic == Negate){
        Type* result = malloc(sizeof(Type));
        result = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        result->nodePtr = currentNode;
        if (result->kind == TypeKindReference || result->kind == TypeKindBox){
            return 1;
        }else if(result->kind == TypeKindComposite){
            result->impl.composite.sign = Signed;
        }
        ParentExpression->result = result;
        
    }else{

        Type* LeftOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[0]->result;
        Type* RightOperandType = ((Expression**) ParentExpression->impl.operation.operands->data)[1]->result;

        ParentExpression->result = createTypeFromOperands(LeftOperandType, RightOperandType, currentNode);
    }



    return 0;
}



Expression *createExpression(AST_NODE_PTR currentNode){
    Expression *expression = malloc(sizeof(Expression));
    expression->nodePtr = currentNode;
    switch(currentNode->kind){
    
    case AST_Int:
    case AST_Float:
        expression->kind = ExpressionKindConstant;
        expression->impl.constant = createTypeValue(currentNode);
        expression->result = expression->impl.constant.type;

    case AST_String:
        expression->kind = ExpressionKindConstant;
        expression->impl.constant = createString(currentNode);
        expression->result = expression->impl.constant.type;
    case AST_Ident:
        expression->kind = ExpressionKindVariable;
        expression->impl.variable = getVariableFromScope(currentNode->value);
        if(NULL == expression->impl.variable){
            DEBUG("Identifier is not in current scope");
            return NULL;
        }
        switch (expression->impl.variable->kind) {
            case VariableKindDeclaration:
                expression->result = expression->impl.variable->impl.declaration.type;
            case VariableKindDefinition:
                expression->result = expression->impl.variable->impl.definiton.declaration.type;
            default:
                PANIC("current Variable should not be an BoxMember");
                break;
        }

    case AST_Add:
    case AST_Sub:
    case AST_Mul:
    case AST_Div:
        expression->kind = ExpressionKindOperation;
         if(createArithOperation(expression, currentNode, 2)){
           return NULL; 
        } 
    case AST_Negate:
        expression->kind = ExpressionKindOperation;
        if(createArithOperation(expression,currentNode, 1)){
            return NULL;
        }


    case AST_Eq:
    case AST_Less:
    case AST_Greater:

    case AST_BoolAnd:
    case AST_BoolNot:
    case AST_BoolOr:
    case AST_BoolXor:

    case AST_BitAnd:
    case AST_BitOr:
    case AST_BitXor:
    case AST_BitNot:

    case AST_IdentList:
    //Box Accsess
    case AST_List:
    // Box Self Access
    case AST_Typekind:
    case AST_Transmute:


    default:
    PANIC("Node is not an expression but from kind: %i", currentNode->kind);
        break;
    }
}

Module *create_set(AST_NODE_PTR currentNode){
    DEBUG("create root Module");
    //create tables for types 
    declaredComposites = g_hash_table_new(g_str_hash,g_str_equal);
    declaredBoxes = g_hash_table_new(g_str_hash,g_str_equal);

    //create scope
    Scope = g_array_new(FALSE, FALSE, sizeof(GHashTable*));


    //building current scope for module
    GHashTable *globalscope = malloc(sizeof(GHashTable*));
    globalscope = g_hash_table_new(g_str_hash,g_str_equal);
    g_array_append_val(Scope, globalscope);

    Module *rootModule = malloc(sizeof(Module));

    GHashTable *boxes = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *types = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *functions = g_hash_table_new(g_str_hash,g_str_equal);
    GHashTable *variables = g_hash_table_new(g_str_hash,g_str_equal);
    GArray *imports = g_array_new(FALSE, FALSE, sizeof(const char*));
    
    rootModule->boxes = boxes;
    rootModule->types = types;
    rootModule->functions = functions;
    rootModule->variables = variables;
    rootModule->imports = imports;

    DEBUG("created Module struct");


    for (size_t i = 0; i < currentNode->child_count; i++){
        DEBUG("created Child: %i" ,currentNode->children[i]->kind);
        switch(currentNode->children[i]->kind){
            
        case AST_Decl:
            if (1 == fillTablesWithVars(variables,globalscope,createDecl(currentNode->children[i]) ,currentNode->children[i]->children[currentNode->children[i]->child_count -1]->child_count)){
                //TODO behandlung, wenn var schon existiert
                DEBUG("var already exists");
                break;
            }
            DEBUG("filled successfull the module and scope with vars");
            break;
        case AST_Def:
            if (1 == fillTablesWithVars(variables,globalscope,createDef(currentNode->children[i]) ,currentNode->children[i]->children[currentNode->children[i]->child_count -1]->child_count)){
                //TODO behandlung, wenn var schon existiert
                DEBUG("var already exists");
                break;
            }
            DEBUG("filled successfull the module and scope with vars");
        case AST_Box:
        case AST_Fun:
        case AST_Import:
            DEBUG("create Import");
            g_array_append_val(imports, currentNode->children[i]->value);
            break;
        default:
            INFO("Provided source file could not be parsed beecause of semantic error.");
            break;

        }
    }

    return rootModule;
}



