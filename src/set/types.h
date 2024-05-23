
#ifndef SET_TYPES_H_
#define SET_TYPES_H_

#include <glib.h>
#include <ast/ast.h>

/**
 * @brief Primitive types form the basis of all other types.
 * 
 */
typedef enum PrimitiveType_t {
    // 4 byte signed integer in two's complement
    Int,
    // 4 byte IEEE-754 single precision
    Float
} PrimitiveType;

/**
 * @brief Represents the sign of a composite type.
 * 
 */
typedef enum Sign_t {
    // type has a sign bit
    Signed,
    // type has no sign bit
    Unsigned
} Sign;

/**
 * @brief Represents the scale of composite type which is multiplied
 *        with the base size in order to retrieve the the composites size.
 * @attention Valid value are: { 1/8, 1/4, 1/2, 1, 2, 4, 8 }
 * 
 */
typedef double Scale;

/**
 * @brief A composite type is an extended definiton of a primitive type.
 * 
 */
typedef struct CompositeType_t {
    // sign of composite
    Sign sign;
    Scale scale;
    PrimitiveType primitive;
} CompositeType;

/**
 * @brief Specifies the specific type of the generic type struct.
 * 
 */
typedef enum TypeKind_t {
    TypeKindPrimitive,
    TypeKindComposite,
    TypeKindBox,
    TypeKindReference
} TypeKind;

typedef struct Type_t Type;

/**
 * @brief Reference points to a type.
 * @attention Can be nested. A reference can point to another reference: REF -> REF -> REF -> Primitive
 * 
 */
typedef Type* ReferenceType;

typedef struct BoxType_t BoxType;

typedef struct BoxMember_t {
    const char* name;
    Type* type;
    BoxType* box;
} BoxMember;

/**
 * @brief Essentially a glorified struct
 * 
 */
typedef struct BoxType_t {
    // hashtable of members.
    // Associates the memebers name (const char*) with its type (BoxMember) 
    GHashTable* member;
} BoxType;

typedef struct Variable_t Variable;

typedef struct BoxAccess_t {
    // list of recursive box accesses
    // contains a list of BoxMembers (each specifying their own type, name and box type)
    GArray* member;
} BoxAccess;

typedef struct Type_t {
    // specifiy the kind of this type
    // used to determine which implementation to choose
    TypeKind kind;
    // actual implementation of the type
    union TypeImplementation_t {
        PrimitiveType primitive;
        CompositeType composite;
        BoxType box;
        ReferenceType reference;
    } impl;
} Type;

typedef struct Typedefine_t {
    const char* name;
    Type type;
} Typedefine;

const Type ShortShortUnsingedIntType = {
    .kind = TypeKindComposite,
    .impl = {
        .composite = {
            .sign = Unsigned,
            .scale = 0.25,
            .primitive = Int
        }
    }
};

const Type StringLiteralType = {
    .kind = TypeKindReference,
    .impl = {
        .reference = (ReferenceType) &ShortShortUnsingedIntType,
    }
};

/**
 * @brief Reprents the value of type. Can be used to definitons, initialization and for expressions contants.
 * 
 */
typedef struct TypeValue_t {
    // the type
    Type type;
    // UTF-8 representation of the type's value
    const char* value;
} TypeValue;

// .------------------------------------------------.
// |                 Functions                      |
// '------------------------------------------------'

/**
 * @brief Specifies a parameters I/O properties
 * 
 */
typedef enum IO_Qualifier_t {
    // Can be read from but not written to.
    // Function local only.
    In,
    // Can be written to but not read from.
    // Passed back to the functions callee.
    Out,
    // Can be read from and written to.
    // Passed back to the functions callee.
    InOut,
} IO_Qualifier;

/**
 * @brief A functions parameter declaration.
 * 
 */
typedef struct ParameterDeclaration_t {
    Type type;
    IO_Qualifier qualifier;
} ParameterDeclaration;

/**
 * @brief A functions parameter.
 * 
 */
typedef struct ParameterDefinition_t {
    ParameterDeclaration declaration;
    // value to initalize the declaration with
    // NOTE: type of initializer and declaration MUST be equal
    TypeValue initializer;
} ParameterDefinition;

typedef enum ParameterKind_t {
    ParameterDeclarationKind,
    ParameterDefinitionKind
} ParameterKind;

/**
 * @brief A parameter can either be a declaration or a definiton
 * 
 */
typedef struct Parameter_t {
    const char* name;
    ParameterKind kind;
    union ParameterImplementation {
        ParameterDeclaration declaration;
        ParameterDefinition definiton;
    } impl;
} Paramer; 

typedef struct FunctionDefinition_t {
    // hashtable of parameters
    // associates a parameters name (const char*) with its parameter declaration (ParameterDeclaration)
    GArray* parameter;
} FunctionDefinition;

// .------------------------------------------------.
// |                 Variables                      |
// '------------------------------------------------'

typedef enum StorageQualifier_t {
    Local,
    Static,
    Global
} StorageQualifier;

typedef struct VariableDeclaration_t {
    StorageQualifier qualifier;
    Type type;
} VariableDeclaration;

/**
 * @brief Definiton of a variable
 * 
 * @attention NOTE: The types of the initializer and the declaration must be equal
 * 
 */
typedef struct VariableDefiniton_t {
    VariableDeclaration declaration;
    TypeValue initializer;
} VariableDefiniton;

typedef enum VariableKind_t {
    VariableKindDeclaration,
    VariableKindDefinition,
    VariableKindBoxMember
} VariableKind;

typedef struct Variable_t {
    VariableKind kind;
    const char* name;
    union VariableImplementation {
        VariableDeclaration declaration;
        VariableDefiniton definiton;
        BoxMember member;
    } impl;
} Variable;

// .------------------------------------------------.
// |                 Casts                          |
// '------------------------------------------------'

/**
 * @brief Perform a type cast, converting a value to different type whilest preserving as much of the original
 *        values information. 
 *
 * @attention NOTE: Must check wether the given value's type can be parsed into
 *       the target type without loss.
 *       Lossy mean possibly loosing information such when casting a float into an int (no fraction anymore). 
 *
 */
typedef struct TypeCast_t {
    Type targetType;
} TypeCast;

/**
 * @brief Perform a reinterpret cast.
 *
 * @attention NOTE: The given value's type must have the size in bytes as the target type.
 *                  Transmuting a short int into a float should yield an error.
 * 
 */
typedef struct Transmute_t {
    Type targetType;
} Transmute;

// .------------------------------------------------.
// |                 Arithmetic                     |
// '------------------------------------------------'

/**
 * @brief Represents the arithmetic operator.
 * 
 */
typedef enum ArithmeticOperator_t {
    Add,
    Sub,
    Mul,
    Div
} ArithmeticOperator;

// .------------------------------------------------.
// |                 Relational                     |
// '------------------------------------------------'

/**
 * @brief Represents the relational operator.
 * 
 */
typedef enum RelationalOperator_t {
    Equal,
    Greater,
    Less
} RelationalOperator;

// .------------------------------------------------.
// |                 Boolean                        |
// '------------------------------------------------'

typedef enum BooleanOperator_t {
    BooleanAnd,
    BooleanOr,
    BooleanNot,
    BooleanXor,
} BooleanOperator;

// .------------------------------------------------.
// |                 Logical                        |
// '------------------------------------------------'

typedef enum LogicalOperator_t {
    LogicalAnd,
    LogicalOr,
    LogicalNot,
    LogicalXor,
} LogicalOperator;

// .------------------------------------------------.
// |                 Logical                        |
// '------------------------------------------------'

typedef enum BitwiseOperator_t {
    BitwiseAnd,
    BitwiseOr,
    BitwiseNot,
    BitwiseXor,
} BitwiseOperator;

// .------------------------------------------------.
// |                 Operations                     |
// '------------------------------------------------'

typedef enum OperationKind_t {
    Arithmetic,
    Relational,
    Boolean,
    Logical,
    Bitwise
} OperationKind;

typedef struct Operation_t {
    // mode of operation
    OperationKind kind;
    // specific implementation
    union OperationImplementation {
        ArithmeticOperator arithmetic;
        RelationalOperator relational;
        BooleanOperator boolean;
        LogicalOperator logical;
        BitwiseOperator bitwise;
    } impl;
} Operation;

// .------------------------------------------------.
// |                 Expression                     |
// '------------------------------------------------'

typedef enum ExpressionKind_t {
    ExpressionKindOperation,
    ExpressionKindTypeCast,
    ExpressionKindTransmute,
    ExpressionKindConstant
} ExpressionKind;

typedef struct Expression_t {
    ExpressionKind kind;
    union ExpressionImplementation_t {
        Operation operation;
        TypeCast typecast;
        Transmute transmute;
        TypeValue constant;
        Variable variable;
    } impl;
} Expression;

// .------------------------------------------------.
// |                 Function call                  |
// '------------------------------------------------'

typedef struct FunctionCall_t {
    // function to call
    FunctionDefinition* function;
    // list of expression arguments
    GArray* expressions;
} FunctionCall;

typedef struct FunctionBoxCall_t {
    // function to call
    FunctionDefinition* function;
    // list of expression arguments
    GArray* expressions;
    // box which has the function defined for it
    // NOTE: must be of TypeKind: Box
    Variable selfArgument;
} FunctionBoxCall;

typedef struct Block_t {
    // array of statements
    GArray* statemnts;
} Block;

// .------------------------------------------------.
// |                 While                          |
// '------------------------------------------------'

typedef struct While_t {
    Expression conditon;
    Block block;
} While;

// .------------------------------------------------.
// |                 If/Else                        |
// '------------------------------------------------'

typedef struct If_t {
    Expression conditon;
    Block block;
} If;

typedef struct ElseIf_t {
    Expression conditon;
    Block block;
} ElseIf;

typedef struct Else_t {
    Block block;
} Else;

typedef struct Branch_t {
    If ifBranch;
    // list of else-ifs (can be empty/NULL)
    GArray* elseIfBranches;
    Else elseBranch;
} Branch;

// .------------------------------------------------.
// |                 Statements                     |
// '------------------------------------------------'

typedef struct Assignment_t {
    Variable* variable;
    Expression value;
} Assignment;

typedef enum StatementKind_t {
    StatementKindFunctionCall,
    StatementKindFunctionBoxCall,
    StatementKindWhile,
    StatementKindBranch,
    StatementKindAssignment
} StatementKind;

typedef struct Statement_t {
    union StatementImplementation {
        FunctionCall call;
        FunctionBoxCall boxCall;
        While whileLoop;
        Branch branch;
        Assignment assignment;
    } impl;
} Statement;

#endif // SET_TYPES_H_
