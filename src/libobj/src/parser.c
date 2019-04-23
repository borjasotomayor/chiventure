#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Necessary includes to use YAML */
#include <yaml.h>

#include "parser.h"

/* DEBUG is 0 normally, 1 to print debugging statements */
#define DEBUG 0

/* Forward function declaration */
void itoa(int n, char s[]);

/* Helper function: to print debugging statments only when debugging */
int myPrint(char* string){
    if (DEBUG){
        printf("%s\n",string);
    }
    return 0;
}

/* See parser.h */
stack_t* begin_sequence(stack_t* stack, char* key){
    stackobj_t* stacktop = stack -> top;
    char* prefix;
    if (stacktop == NULL){
        prefix = calloc(1,MAX_DEPTH);
    } else{
        prefix = stacktop -> attr_string;
    }
    prefix = extend_prefix(prefix, key);
    stack = push(prefix, 1, stack);
    myPrint(prefix);
    return stack; 
}

/* See parser.h */
stack_t* begin_obj(stack_t* stack, char* key){
    stackobj_t* stacktop = stack -> top;
    char* prefix;

    //In null case, create new attribute prefix string, else take from top
    if (stacktop == NULL){
        prefix = calloc(1,MAX_DEPTH);
        myPrint("stacktop was null");
    } else{
        prefix = stacktop -> attr_string;

        //If this is an element in a list, add list index to prefix
        if (stacktop -> isListHead){
            char* index_string = calloc(1,100);
            itoa((stacktop -> nextListIndex)++, index_string);
            myPrint("index_string");
            myPrint(index_string);
            prefix = extend_prefix(prefix, index_string);
        }
    }
    
    prefix = extend_prefix(prefix, key);
    myPrint(prefix);
    stack = push(prefix, 0, stack);
    return stack; 
}

/* See parser.h */
char* extend_prefix(char* orig_prefix, char* to_add){
    char* prefix = calloc(1, MAX_DEPTH);

    //If there's nothing to add, return original.
    //If the original is empty, the prefix is just to_add
    //If neither is NULL, 
    if (to_add == NULL){
        return orig_prefix;
    } else if (!orig_prefix[0]){ //if original prefix is empty
        prefix = strcpy(prefix, to_add);
    } else {
        strcpy(prefix, orig_prefix);
        prefix = strcat(prefix,".");
        prefix = strcat(prefix,to_add);
    }
    return prefix;
}


/* See parser.h */
int add_attr(stack_t* stack, char* key, char* val, obj_t* obj){
    char* prefix = calloc(1,MAX_DEPTH);
    strcpy(prefix, (stack -> top) -> attr_string);
    if (prefix == NULL){
        myPrint("in add_attr: prefix null");
        return -1;
    }
    prefix = extend_prefix(prefix, key);
    myPrint("in attr:");
    myPrint(prefix);
    myPrint(val);

    int maybe_int = atoi(val); //For use later to see if val is an integer
    if (!strcmp(val,"true")){
        obj_set_bool(obj, prefix, true);
    }
    else if (!strcmp(val,"false")){
        obj_set_bool(obj, prefix, false);
    } else if (!strcmp(val,"0")){
        obj_set_int(obj, prefix, 0);
    } else if (maybe_int != 0){
        obj_set_int(obj, prefix, maybe_int);
    } else if (strlen(val) == 1){
        obj_set_char(obj, prefix, val[0]);
    } else{
        obj_set_str(obj, prefix, val);
    }
    return 0;
}

/* See parser.h */
int parse_game(char* filename, obj_t* docobj)
{

    yaml_parser_t parser; //creates a parser
    yaml_token_t token; //creating a token
    FILE* pfile; //file to be parsed
    printf("Now parsing file %s \n", filename);

    pfile = fopen(filename,"rb"); //opens file for reading
    assert(pfile);
    assert(yaml_parser_initialize(&parser));

    yaml_parser_set_input_file(&parser, pfile); //sets input file

    yaml_parser_scan(&parser, &token);
    tok_type prev_token = 8; //initialized prev_token to something meaningless
    char* key = NULL; 
    stack_t* stack = new_stack();

    while(token.type != YAML_STREAM_END_TOKEN){
        yaml_parser_scan(&parser, &token);
        switch(token.type)
        {
            /* Stream start/end (ignored) */
            case YAML_STREAM_START_TOKEN: 
                myPrint("STREAM START");
                break;
            case YAML_STREAM_END_TOKEN:
                myPrint("STREAM END");
                break;

            /* Token types (read before actual strings) */
            case YAML_KEY_TOKEN:   
                myPrint("(Key token)");
                prev_token = KEY;
                break;
            case YAML_VALUE_TOKEN:   
                myPrint("Value token");
                prev_token = VALUE;
                break;

            /* Data */
            /* Ignored */
            case YAML_BLOCK_MAPPING_START_TOKEN:
                myPrint("[Block mapping]");
                prev_token = BLOCK_START;
                break;

            /* Key or Value strings */
            case YAML_SCALAR_TOKEN: 
                myPrint("scalar ");
                myPrint((char*)token.data.scalar.value);
                switch (prev_token){
                    case KEY:
                        key = (char*)token.data.scalar.value;
                        break;
                    case VALUE:
                        add_attr(stack, key, (char*)token.data.scalar.value, docobj);
                        key = NULL;
                        break;
                    default:
                        myPrint("ERR: scalar immediately following non-value/key token");
                        break;
                }
                break;

            /* Block delimeters */
            /* If it's the start of a sequence, you're about to have a list*/
            case YAML_BLOCK_SEQUENCE_START_TOKEN:
                myPrint("<b>Start Block (Sequence)</b>");
                if ((prev_token != VALUE)||(key == NULL)){
                    myPrint("ERR: odd sequence of events");
                    break;
                }
                stack = begin_sequence(stack, key);
                key = NULL;
                break;

            /* If it's not the start of a sequence, deal accordingly*/
            case YAML_BLOCK_ENTRY_TOKEN:
                myPrint("<b>Start Block (Entry)</b>");
                stack = begin_obj(stack, key);
                key = NULL;
                break;

            case YAML_BLOCK_END_TOKEN:
                myPrint("<b>End block</b>");
                if ((stack -> top) == NULL){
                    myPrint("top was null"); //Should happen only at the end
                    break;
                }
                pop(stack);
                if ((stack -> top) == NULL){
                    myPrint("Reached end of stack");
                    break;
                }
               break;

            /* Others */
            default:
                myPrint("Got token of another type");
        }
    }

    //Delete token
    yaml_token_delete(&token);

    //Delete parser
    yaml_parser_delete(&parser);

    //Close file
    assert(!fclose(pfile));

    return 0;
}



/* Helper functions (taken from K&R) to convert string to int */
void reverse(char s[])
{
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }

void itoa(int n, char s[])
{
     int i, sign;
 
     if ((sign = n) < 0)  /* record sign */
         n = -n;          /* make n positive */
     i = 0;
     do {       /* generate digits in reverse order */
         s[i++] = n % 10 + '0';   /* get next digit */
     } while ((n /= 10) > 0);     /* delete it */
     if (sign < 0)
         s[i++] = '-';
     s[i] = '\0';
     reverse(s);
 }