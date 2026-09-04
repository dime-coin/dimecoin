// Standalone test for i2p_sam.h — verifies SAM v3 protocol logic end-to-end
// against a mock SAM server. Compile: g++ -std=c++11 -o i2p_test i2p_sam_test.cpp
#include "i2p_sam.h"
#include <cstdio>
#include <cstring>

static int g_failures = 0;
#define CHECK(cond) do { if(!(cond)) { printf("FAIL: %s (line %d)\n", #cond, __LINE__); ++g_failures; } else { printf("ok: %s\n", #cond); } } while(0)

int main() {
    i2p::SamConnection c;
    CHECK(c.Connect("127.0.0.1", 7656));

    CHECK(c.Hello());

    std::string dest;
    CHECK(c.CreateSession("testid", dest, "TRANSIENT", 7));
    CHECK(!dest.empty());
    printf("  session dest (truncated): %.16s...\n", dest.c_str());

    i2p_socket_t data = c.StreamConnect("testid", "somedestinationbase64");
    CHECK(data >= 0);

    // Data channel echo: send a byte, read it back from the mock loopback.
    char out = 'Z';
    ssize_t n = ::send(data, &out, 1, 0);
    CHECK(n == 1);
    char in = 0;
    n = ::recv(data, &in, 1, 0);
    CHECK(n == 1);
    CHECK(in == 'Z');

    ::close(data);

    if (g_failures == 0) printf("\nALL TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
