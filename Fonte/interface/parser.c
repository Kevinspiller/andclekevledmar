#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef FMACROS
   #include "../macros.h"
#endif
#ifndef FTYPES
   #include "../types.h"
#endif
#ifndef FMISC
   #include "../misc.h"
#endif
#ifndef FDATABASE
   #include "../database.h"
#endif
#ifndef FSQLCOMMANDS
   #include "../sqlcommands.h"
#endif
#ifndef FPARSER
   #include "parser.h"
#endif

/* Estrutura global que guarda as informações obtidas pelo yacc
 * na identificação dos tokens
 */
rc_insert GLOBAL_DATA;

/* Estrutura auxiliar do reconhecedor.
 */
rc_parser GLOBAL_PARSER;

/********   SELECT *******/
rc_select GLOBAL_SELECT;


void connect(char *nome) {
    int r;
    r = connectDB(nome);
	if (r == SUCCESS) {
        connected.db_name = malloc(sizeof(char)*((strlen(nome)+1)));

        strcpylower(connected.db_name, nome);

        connected.conn_active = 1;
        printf("You are now connected to database \"%s\" as user \"uffsdb\".\n", nome);
    } else {
    	printf("ERROR: Failed to establish connection with database named \"%s\". (Error code: %d)\n", nome, r);
    }
}

void invalidCommand(char *command) {
    printf("ERROR: Invalid command '%s'. Type \"help\" for help.\n", command);
}

void notConnected() {
    printf("ERROR: you are not connected to any database.\n");
}

void setObjName(char **nome) {
    if (GLOBAL_PARSER.mode != 0) {
        GLOBAL_DATA.objName = malloc(sizeof(char)*((strlen(*nome)+1)));

        strcpylower(GLOBAL_DATA.objName, *nome);
        GLOBAL_DATA.objName[strlen(*nome)] = '\0';
        GLOBAL_PARSER.step++;
    } else
        return;
}

void setColumnInsert(char **nome) {
    GLOBAL_DATA.columnName = realloc(GLOBAL_DATA.columnName, (GLOBAL_PARSER.col_count+1)*sizeof(char *));

    GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count] = malloc(sizeof(char)*(strlen(*nome)+1));
    strcpylower(GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count], *nome);
    GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count][strlen(*nome)] = '\0';

    GLOBAL_PARSER.col_count++;
}

void setValueInsert(char *nome, char type) {
    int i;
    GLOBAL_DATA.values  = realloc(GLOBAL_DATA.values, (GLOBAL_PARSER.val_count+1)*sizeof(char *));
    GLOBAL_DATA.type    = realloc(GLOBAL_DATA.type, (GLOBAL_PARSER.val_count+1)*sizeof(char));

    // Adiciona o valor no vetor de strings
    GLOBAL_DATA.values[GLOBAL_PARSER.val_count] = malloc(sizeof(char)*(strlen(nome)+1));
    if (type == 'I' || type == 'D') {
        strcpy(GLOBAL_DATA.values[GLOBAL_PARSER.val_count], nome);
        GLOBAL_DATA.values[GLOBAL_PARSER.val_count][strlen(nome)] = '\0';
    } else if (type == 'S') {
        for (i = 1; i < strlen(nome)-1; i++) {
            GLOBAL_DATA.values[GLOBAL_PARSER.val_count][i-1] = nome[i];
        }
        GLOBAL_DATA.values[GLOBAL_PARSER.val_count][strlen(nome)-2] = '\0';
    }

    GLOBAL_DATA.type[GLOBAL_PARSER.val_count] = type;

    GLOBAL_PARSER.val_count++;
}

void setColumnCreate(char **nome) {
    GLOBAL_DATA.columnName  = realloc(GLOBAL_DATA.columnName, (GLOBAL_PARSER.col_count+1)*sizeof(char *));
    GLOBAL_DATA.attribute   = realloc(GLOBAL_DATA.attribute, (GLOBAL_PARSER.col_count+1)*sizeof(int));
    GLOBAL_DATA.fkColumn    = realloc(GLOBAL_DATA.fkColumn, (GLOBAL_PARSER.col_count+1)*sizeof(char *));
    GLOBAL_DATA.fkTable     = realloc(GLOBAL_DATA.fkTable, (GLOBAL_PARSER.col_count+1)*sizeof(char *));
    GLOBAL_DATA.values      = realloc(GLOBAL_DATA.values, (GLOBAL_PARSER.col_count+1)*sizeof(char *));
    GLOBAL_DATA.type        = realloc(GLOBAL_DATA.type, (GLOBAL_PARSER.col_count+1)*sizeof(char *));

    GLOBAL_DATA.values[GLOBAL_PARSER.col_count] = malloc(sizeof(char));
    GLOBAL_DATA.fkTable[GLOBAL_PARSER.col_count] = malloc(sizeof(char));
    GLOBAL_DATA.fkColumn[GLOBAL_PARSER.col_count] = malloc(sizeof(char));
    GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count] = malloc(sizeof(char)*(strlen(*nome)+1));

    strcpylower(GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count], *nome);

    GLOBAL_DATA.columnName[GLOBAL_PARSER.col_count][strlen(*nome)] = '\0';
    GLOBAL_DATA.type[GLOBAL_PARSER.col_count] = 0;
    GLOBAL_DATA.attribute[GLOBAL_PARSER.col_count] = NPK;

    GLOBAL_PARSER.col_count++;
    GLOBAL_PARSER.step = 2;
}

void setColumnTypeCreate(char type) {
    GLOBAL_DATA.type[GLOBAL_PARSER.col_count-1] = type;
    GLOBAL_PARSER.step++;
}

void setColumnSizeCreate(char *size) {
    GLOBAL_DATA.values[GLOBAL_PARSER.col_count-1] = realloc(GLOBAL_DATA.values[GLOBAL_PARSER.col_count-1], sizeof(char)*(strlen(size)+1));
    strcpy(GLOBAL_DATA.values[GLOBAL_PARSER.col_count-1], size);
    GLOBAL_DATA.values[GLOBAL_PARSER.col_count-1][strlen(size)-1] = '\0';
}

void setColumnPKCreate() {
    GLOBAL_DATA.attribute[GLOBAL_PARSER.col_count-1] = PK;
}

void setColumnFKTableCreate(char **nome) {
    GLOBAL_DATA.fkTable[GLOBAL_PARSER.col_count-1] = realloc(GLOBAL_DATA.fkTable[GLOBAL_PARSER.col_count-1], sizeof(char)*(strlen(*nome)+1));
    strcpylower(GLOBAL_DATA.fkTable[GLOBAL_PARSER.col_count-1], *nome);
    GLOBAL_DATA.fkTable[GLOBAL_PARSER.col_count-1][strlen(*nome)] = '\0';
    GLOBAL_DATA.attribute[GLOBAL_PARSER.col_count-1] = FK;
    GLOBAL_PARSER.step++;
}

void setColumnFKColumnCreate(char **nome) {
    GLOBAL_DATA.fkColumn[GLOBAL_PARSER.col_count-1] = realloc(GLOBAL_DATA.fkColumn[GLOBAL_PARSER.col_count-1], sizeof(char)*(strlen(*nome)+1));
    strcpylower(GLOBAL_DATA.fkColumn[GLOBAL_PARSER.col_count-1], *nome);
    GLOBAL_DATA.fkColumn[GLOBAL_PARSER.col_count-1][strlen(*nome)] = '\0';
    GLOBAL_PARSER.step++;
}


void clearGlobalStructs() {
    int i;

    if (GLOBAL_DATA.objName) {
        free(GLOBAL_DATA.objName);
        GLOBAL_DATA.objName = NULL;
    }

    for (i = 0; i < GLOBAL_DATA.N; i++ ) {
        if (GLOBAL_DATA.columnName)
            free(GLOBAL_DATA.columnName[i]);
        if (GLOBAL_DATA.values)
            free(GLOBAL_DATA.values[i]);
        if (GLOBAL_DATA.fkTable)
            free(GLOBAL_DATA.fkTable[i]);
        if (GLOBAL_DATA.fkColumn)
            free(GLOBAL_DATA.fkColumn[i]);
    }

    free(GLOBAL_DATA.columnName);
    GLOBAL_DATA.columnName = NULL;

    free(GLOBAL_DATA.values);
    GLOBAL_DATA.values = NULL;

    free(GLOBAL_DATA.fkTable);
    GLOBAL_DATA.fkTable = NULL;

    free(GLOBAL_DATA.fkColumn);
    GLOBAL_DATA.fkColumn = NULL;

    free(GLOBAL_DATA.type);
    GLOBAL_DATA.type = (char *)malloc(sizeof(char));

    free(GLOBAL_DATA.attribute);
    GLOBAL_DATA.attribute = (int *)malloc(sizeof(int));

    yylex_destroy();

    GLOBAL_DATA.N = 0;

    GLOBAL_PARSER.mode              = 0;
    GLOBAL_PARSER.parentesis        = 0;
    GLOBAL_PARSER.noerror           = 1;
    GLOBAL_PARSER.col_count         = 0;
    GLOBAL_PARSER.val_count         = 0;
    GLOBAL_PARSER.step              = 0;

    free(GLOBAL_SELECT.where);
    GLOBAL_SELECT.where = NULL;
}

void setMode(char mode) {
    GLOBAL_PARSER.mode = mode;
    GLOBAL_PARSER.step++;
}


int interface() {
    pthread_t pth;

    pthread_create(&pth, NULL, (void*)clearGlobalStructs, NULL);
    pthread_join(pth, NULL);

    connect("uffsdb"); // conecta automaticamente no banco padrão

    while(1){
        if (!connected.conn_active) {
            printf(">");
        } else {
            printf("%s=# ", connected.db_name);
        }

        pthread_create(&pth, NULL, (void*)yyparse, &GLOBAL_PARSER);
        pthread_join(pth, NULL);

        if (GLOBAL_PARSER.noerror) {
            if (GLOBAL_PARSER.mode != 0) {
                if (!connected.conn_active) {
                    notConnected();
                } else {
                    switch(GLOBAL_PARSER.mode) {
                        case OP_INSERT:
                            if (GLOBAL_DATA.N > 0) {
                                insert(&GLOBAL_DATA);
                            }
                            else
                                printf("WARNING: Nothing to be inserted. Command ignored.\n");
                            break;
                        case OP_SELECT_ALL:
                            imprime(&GLOBAL_SELECT);
                            break;
                        case OP_CREATE_TABLE:
                            createTable(&GLOBAL_DATA);
                            break;
                        case OP_CREATE_DATABASE:
                            createDB(GLOBAL_DATA.objName);
                            break;
                        case OP_DROP_TABLE:
                            excluirTabela(GLOBAL_DATA.objName);
                            break;
                        case OP_DROP_DATABASE:
                            dropDatabase(GLOBAL_DATA.objName);
                            break;
                        default: break;
                    }

                }
            }
        } else {
            GLOBAL_PARSER.consoleFlag = 1;
            switch(GLOBAL_PARSER.mode) {
                case OP_CREATE_DATABASE:
                case OP_DROP_DATABASE:
                case OP_CREATE_TABLE:
                case OP_DROP_TABLE:
                case OP_SELECT_ALL:
                case OP_INSERT:
                    if (GLOBAL_PARSER.step == 1) {
                        GLOBAL_PARSER.consoleFlag = 0;
                        printf("Expected object name.\n");
                    }
                break;

                default: break;
            }

            if (GLOBAL_PARSER.mode == OP_CREATE_TABLE) {
                if (GLOBAL_PARSER.step == 2) {
                    printf("Column not specified correctly.\n");
                    GLOBAL_PARSER.consoleFlag = 0;
                }
            } else if (GLOBAL_PARSER.mode == OP_INSERT) {
                if (GLOBAL_PARSER.step == 2) {
                    printf("Expected token \"VALUES\" after object name.\n");
                    GLOBAL_PARSER.consoleFlag = 0;
                }
            }

            printf("ERROR: syntax error.\n");
            GLOBAL_PARSER.noerror = 1;
        }

        if (GLOBAL_PARSER.mode != 0) {
            pthread_create(&pth, NULL, (void*)clearGlobalStructs, NULL);
            pthread_join(pth, NULL);
        }
    }
    return 0;
}

void yyerror(char *s, ...) {
    GLOBAL_PARSER.noerror = 0;
 }

/********* select *******/

 void setObjNameSelect(char **name) {
    if (GLOBAL_PARSER.mode == 0)
        return;
    GLOBAL_SELECT.objName = malloc(sizeof(char)*((strlen(*name)+1)));
    strcpylower(GLOBAL_SELECT.objName, *name);
    GLOBAL_SELECT.objName[strlen(*name)] = '\0';
    GLOBAL_PARSER.step++;
}

void setColumnProjection(char **name) {
    GLOBAL_SELECT.columnName = realloc(GLOBAL_SELECT.columnName, (GLOBAL_SELECT.nColumn + 1)*sizeof(char *));
    GLOBAL_SELECT.columnName[GLOBAL_SELECT.nColumn] = malloc(sizeof(char)*(strlen(*name) + 1));
    strcpylower(GLOBAL_SELECT.columnName[GLOBAL_SELECT.nColumn], *name);
    GLOBAL_SELECT.columnName[GLOBAL_SELECT.nColumn][strlen(*name)] = '\0';
    GLOBAL_SELECT.nColumn++;
}

/*------------------------------------------------
resetSelect: resetar os ponteiros para a projeção
-------------------------------------------------*/
void resetSelect(){
    free(GLOBAL_SELECT.objName);
    GLOBAL_SELECT.objName = NULL;
    free(GLOBAL_SELECT.columnName);
    GLOBAL_SELECT.columnName = NULL;
    GLOBAL_SELECT.nColumn = 0;
}

/********** where ********/

/* ----------------------------------------------------------------------------------------------
    setWhere: Cria uma nova condição where na lista.
              Caso for a primeira condição where ele aloca o nodo no inicio da lista
              se não ele percorre a lista até o final e aloca um novo nodo no final da lista.
              Se for a primeira condição ele aloca o nodo com a condição logica NONE se não
              ele aloca com a operação que o usuario digitar podendo ser AND ou OR que é passado
              como parametro pelo yacc.y.
------------------------------------------------------------------------------------------------*/
void setWhere(op_logic logic){
    rc_where * new, *loop;

    new = (rc_where*)malloc(sizeof(rc_where));
    new->next = NULL;

    if(GLOBAL_SELECT.where == NULL)
        GLOBAL_SELECT.where = new;
    else{
        for(loop = GLOBAL_SELECT.where; loop->next != NULL; loop = loop->next);

        loop->next = new;
    }

    new->left_logic = logic;
}
/* -----------------------------------------------------------------------------------------------------------
    setCompWhereLeft: Aloca um novo nodo com valores e tipo das condições do where.
                      Pega como parametro o que o usuario digitou a esquerda da condição 
                      e guarda no nodo juntamente com o tipo (Coluna, String, Valor inteiro ou Valor quebrado)
                      que é passado como parametro. 
-------------------------------------------------------------------------------------------------------------*/
void setCompWhereLeft(char **op_left, char type){ 
    rc_where * aux;
    bool isString = type == 'S';

    for(aux = GLOBAL_SELECT.where; aux->next != NULL; aux = aux->next);

    aux->comp = (rc_where_comp*)malloc(sizeof(rc_where_comp)); 
    aux->comp->left.value = malloc(sizeof(char)*(strlen(*op_left) + 1));
    strcpy(aux->comp->left.value, &(*op_left)[isString ? 1 : 0 ]); // verifica se é uma string e ja copia para o value
    if(isString) 
        aux->comp->left.value[strlen(*op_left)-2] = '\0'; // tratamento de '' nas strings
    else    
        aux->comp->left.value += '\0';

    aux->comp->left.type = type;
}
/* ----------------------------------------------------------------------------------
    setCompWhereOp: recebe o tipo da operação que o usuario digitou e guarda no nodo
------------------------------------------------------------------------------------*/
void setCompWhereOp(operation op){ 
    rc_where * aux;

    for(aux = GLOBAL_SELECT.where; aux->next != NULL; aux = aux->next);

    aux->comp->op = op;
}
/* -----------------------------------------------------------------------------------------------------------
    setCompWhereRight: Aloca um novo nodo com valores e tipo das condições do where.
                      Pega como parametro o que o usuario digitou a direita da condição 
                      e guarda no nodo juntamente com o tipo (Coluna, String, Valor inteiro ou Valor quebrado)
                      que é passado como parametro. 
-------------------------------------------------------------------------------------------------------------*/
void setCompWhereRight(char **op_left, char type){ 
    rc_where * aux;
    bool isString = (type == 'S');

    for(aux = GLOBAL_SELECT.where; aux->next != NULL; aux = aux->next);

    aux->comp->right.value = malloc(sizeof(char)*(strlen(*op_left) + 1));
    strcpy(aux->comp->right.value, &(*op_left)[isString ? 1 : 0 ]); // verifica se é uma string e ja copia para o value
    if(isString)
        aux->comp->right.value[strlen(*op_left)-2] = '\0'; // tratamento de '' nas strings
    else    
        aux->comp->right.value += '\0';

    aux->comp->right.type = type;
}

