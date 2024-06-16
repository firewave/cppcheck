// cppcheck-suppress misra-c2012-2.3
typedef struct {
    int x; // cppcheck-suppress unusedStructMember
} X;

static void func(void) {};

int main(void) {
    #ifdef A
    X x = {5};
    func();
    return x.x;
    
    #elif defined(B)
    return 0;
    #endif
    
}