// To test:
// ~/cppcheck/cppcheck --dump naming_test.cpp && python3 ../naming.py --var='[a-z].*' --function='[a-z].*' --private-member-variable='m[A-Z].*' --const='[_a-zA-Z].*' naming_test.cpp.dump

// No error for mismatching Constructor/Destructor names should be issued, they can not be changed.
namespace test1
{
class TestClass1
{
    TestClass1() {}
    ~TestClass1() {}
    TestClass1(const TestClass1 &) {}
    TestClass1(TestClass1 &&) {}
};
}

namespace test2
{
class A
{
private:
    int intValue;
    int mIntValue;
};
}
