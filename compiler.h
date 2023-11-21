

typedef enum OpCode {
    
    PUSH_VAR_OP,
    POP_VAR_OP,
    GET_VAR_WITH_OFFSET_OP,
    GET_VAR_WITH_CONST_OP,




} OpCode;




/* Final Runtime ready ByteCode */

typedef enum RtType {
    NUMBER_TYPE,
    STRING_TYPE,
    OBJECT_TYPE,
    FUNCTION_TYPE,
    LIST_TYPE,
    HASHMAP_TYPE,
    HASHSET_TYPE,
} RtType;

// ALL variables are 
typedef struct RtObject {


    union data {
        struct Number {

        } Number;

        struct String {

        } String;

        struct Object {

        };

        struct Function {

        } Function;

        struct List {
            
        } List;

        struct HashMap {

        } HashMap;

        struct HashMap {

        } HashSet;

    } data;
} RtObject;


typedef struct ByteCode {
    OpCode code;

    RtObject *interm1;
    RtObject *interm2;
    RtObject *interm3;

} ByteCode;





