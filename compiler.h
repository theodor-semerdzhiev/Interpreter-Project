
/**
 * if (10 > num) {
 * 
 * } else {
 * 
 * }
 * 
 * 1. LOAD_VAR num
 * 2. BIGGER THAN 10 NUM [4]
 * 3. JUMP 5
 * 4. [if block ...]
 * 5. [else block ... ]
 * 6. everything else
 * 
 * 
 * 
 * 
 * 
*/


typedef enum OpCode {
    //TODO
    
    
    GET_FROM_STACK,

    
} OpCode;


typedef struct Intermediate {

} Intermediate;


typedef struct ByteCode {
    OpCode code;

    Intermediate *interm1;
    Intermediate *interm2;
    Intermediate *interm3;

} ByteCode;





