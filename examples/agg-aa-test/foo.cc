#include <stdint.h>

int foo = __cplusplus;
uintptr_t bar = 0;

int main()
{
    bar = bar;
    return foo;
}
