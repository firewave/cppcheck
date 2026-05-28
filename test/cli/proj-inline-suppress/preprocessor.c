// cppcheck-suppress id0
#define DEF

// cppcheck-suppress id1
#if 0
// cppcheck-suppress id1_0
void f1();
#endif

// cppcheck-suppress id2
#if 1
// cppcheck-suppress id2_1
void f2();
#endif

// cppcheck-suppress id3
#if X
// cppcheck-suppress id3_1
void f3();
#endif
