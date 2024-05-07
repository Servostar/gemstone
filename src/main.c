#include <ast/ast.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/log.h>

#define LOG_LEVEL LOG_LEVEL_DEBUG

extern FILE* yyin;

/**
 * @brief Log a debug message to inform about beginning exit procedures
 * 
 */
void notify_exit(void)
{
    DEBUG("Exiting gemstone...");
}

/**
 * @brief Closes File after compiling.
 * 
 */

void close_file(void)
{
    if (NULL != yyin)
    {
        fclose(yyin);
    }
}

/**
 * @brief Run compiler setup here
 * 
 */
void setup(void)
{
    // setup preample

    log_init();
    DEBUG("starting gemstone...");

    #if LOG_LEVEL <= LOG_LEVEL_DEBUG
    atexit(&notify_exit);
    #endif

    // actual setup
    
    DEBUG("finished starting up gemstone...");
}

int main(int argc, char *argv[]) {

    setup();
    atexit(close_file);
    
    // Check for file input as argument
    if (2 != argc)
    {
        INFO("Usage: %s <filename>\n", argv[0]);
        PANIC("No File could be found");
    }
    
    // filename as first argument
    char *filename = argv[1];

    FILE *file = fopen(filename, "r");

    struct AST_Node_t* node = AST_new_node(AST_Branch, NULL);

    struct AST_Node_t* child = AST_new_node(AST_OperatorAdd, NULL);
    AST_push_node(child, AST_new_node(AST_IntegerLiteral, "43"));
    AST_push_node(child, AST_new_node(AST_IntegerLiteral, "9"));

    AST_push_node(node, child);
    AST_push_node(node, AST_new_node(AST_Expression, NULL));
    AST_push_node(node, AST_new_node(AST_Expression, NULL));

    FILE* out = fopen("ast.gv", "w+");
    // convert this file        ^^^^^^
    // to an svg with: `dot -Tsvg ast.gv > graph.svg`

    AST_fprint_graphviz(out, node);

    AST_delete_node(node);

    fflush(out);
    fclose(out);

    return 0;
}
