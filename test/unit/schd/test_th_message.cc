#include <iostream>

#include "test/unit/unit-test.h"

using namespace bblocks;
using namespace std;

static const string _path = "/test_th_message";

//................................................................................ test_handler ....

class ICallee
{
public:

    virtual void Handle(int val) = 0;

};

class Callee : public ICallee
{
public:

    static const int MAX_CALLS = 1000;

    Callee() : count_(0) {}

    void Handle(int val)
    {
        DEBUG(_path) << "Got handle " << count_;
        ASSERT(val == 0xfeaf);
        if (++count_ == MAX_CALLS) {
            BBlocks::Wakeup();
        }
    }

    atomic<int> count_;
};

class Caller
{
public:

    static const uint64_t TEST = 1024;

    Caller(ICallee * h = NULL) : h_(h) {}

    void Start(int val)
    {
        ASSERT(h_);
        BBlocks::Schedule(h_, &ICallee::Handle, val);
    }

    ICallee *h_;
};

void
test_handler()
{
    BBlocks::Start();

    Callee callee;
    Caller caller(&callee);
    for (int i = 0; i < Callee::MAX_CALLS; ++i) {
        BBlocks::Schedule(&caller, &Caller::Start, /*val=*/ (int) 0xfeaf);
    }

    BBlocks::Wait();
    BBlocks::Shutdown();
}

//........................................................................................ main ....

int
main(int argc, char ** argv)
{
    InitTestSetup();

    TEST(test_handler);

    TeardownTestSetup();

    return 0;
}
