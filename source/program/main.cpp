#include <stdio.h>

#include "lib.hpp"
#include "base/pointers.hpp"
#include <format>

HOOK_DEFINE_TRAMPOLINE(LoggerPoC)
{
    static void Callback()
    {
        Logging.Log("turbo-exlaunch! :^)");

        Orig();
    }
};

namespace nn::oe
{
    void Initialize();
};

extern "C" void exl_main(void *x0, void *x1)
{
    exl::hook::Initialize();

    LoggerPoC::InstallAtFuncPtr(nn::oe::Initialize);

    Logging.Log("Patching!");

    // The following is required to run the byte pattern searcher.
    base::Pointers::create();
}