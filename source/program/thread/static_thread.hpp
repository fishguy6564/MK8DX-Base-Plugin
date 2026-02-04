#pragma once

#include <lib.hpp>
#include <nn.hpp>

#include <array>
#include <functional>
#include "thread.hpp"

namespace exl::thread {

    template<size_t StackSize> 
    class StaticThread : public Thread {
        static_assert(StackSize % Thread::StackAlignment == 0, "Thread stack must be aligned!");
        
        private:
        alignas(Thread::StackAlignment) 
        std::array<std::byte, StackSize> m_StackData;

        public:
        constexpr StaticThread() : Thread(std::span(m_StackData)) { }
    };
}