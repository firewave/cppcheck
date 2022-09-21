// To test:
// ~/cppcheck/cppcheck --dump naming_test.c && python3 ../naming.py --var='[a-z].*' --function='[a-z].*' --private-member-variable='m[A-Z].*' --const='[_a-zA-Z].*' naming_test.c.dump

// Should not crash when there is no name
void func(int number, int);
