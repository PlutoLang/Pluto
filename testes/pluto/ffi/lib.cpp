#include <cstdint> // uint8_t
#include <cstring> // memcpy
#include <stdexcept>
#include "../../../src/vendor/Soup/soup/base.hpp"

SOUP_CEXPORT int MY_MAGIC_INT = 69;

SOUP_CEXPORT int add(int a, int b)
{
    return a + b;
}

struct Result
{
    int sum;
    int product;
};

SOUP_CEXPORT void quick_maffs(Result *out, int a, int b)
{
    out->sum = a + b;
    out->product = a * b;
}

SOUP_CEXPORT void throw_exception(const char* msg)
{
    throw std::runtime_error(msg);
}

SOUP_CEXPORT void throw_int(int value)
{
    throw value;
}

SOUP_CEXPORT void buffer_test(uint8_t* in, size_t inlen, uint8_t* out, size_t outlen)
{
    if (inlen > outlen)
    {
        inlen = outlen;
    }
    memcpy(out, in, inlen);
}

using callback_t = int(*)(int);

static callback_t s_cb;

SOUP_CEXPORT void set_cb(callback_t cb)
{
    s_cb = cb;
}

SOUP_CEXPORT int call_cb(int val)
{
    return s_cb(val);
}
