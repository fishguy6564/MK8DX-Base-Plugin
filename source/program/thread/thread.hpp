#pragma once

#include <lib.hpp>
#include <nn.hpp>

#include <span>

namespace exl::thread {

    class Thread {

        private:
        nn::os::ThreadType m_Thread;
        std::span<std::byte> m_Stack;

        inline void* GetStack() const { return reinterpret_cast<void*>(m_Stack.data()); }
        inline std::size_t GetStackSize() const { return m_Stack.size(); }

        public:
        constexpr inline static auto StackAlignment = nn::os::ThreadStackAlignment;

        inline Thread(const std::span<std::byte> stack) : m_Stack(stack) {
            /* Ensure stack passed isn't empty. */
            EXL_ASSERT(!stack.empty());
            /* Ensure stack alignement. */
            EXL_ASSERT(reinterpret_cast<uintptr_t>(GetStack()) % StackAlignment == 0);
            EXL_ASSERT(GetStackSize() % StackAlignment == 0);

        }
        inline Result Create(nn::os::ThreadFunction function, void *argument, s32 priority, s32 ideal_core) {
            return nn::os::CreateThread(&m_Thread, function, argument, GetStack(), GetStackSize(), priority, ideal_core);
        }
        inline Result Create(nn::os::ThreadFunction function, void *argument, s32 priority) {
            return nn::os::CreateThread(&m_Thread, function, argument, GetStack(), GetStackSize(), priority);
        }
        inline Result Create(nn::os::ThreadFunction function, void *argument) {
            return Create(function, argument, nn::os::DefaultThreadPriority);
        }
        inline Result Create(nn::os::ThreadFunction function, s32 priority, s32 ideal_core) {
            return Create(function, (void*) NULL, priority, ideal_core);
        }
        inline Result Create(nn::os::ThreadFunction function, s32 priority) {
            return Create(function, (void*) NULL, priority);
        }
        inline Result Create(nn::os::ThreadFunction function) {
            return Create(function, (void*) NULL);
        }
        inline void Destroy() {
            nn::os::DestroyThread(&m_Thread);
        }
        inline void Start() {
            nn::os::StartThread(&m_Thread);
        }
        inline void Wait() {
            nn::os::WaitThread(&m_Thread);
        }
        inline bool TryWait() {
            return nn::os::TryWaitThread(&m_Thread);
        }
        inline s32 Suspend() {
            return nn::os::SuspendThread(&m_Thread);
        }
        inline s32 Resume() {
            return nn::os::ResumeThread(&m_Thread);
        }
        inline s32 GetSuspendCount() const {
            return nn::os::GetThreadSuspendCount(&m_Thread);
        }
        inline void CancelSynchronization() {
            nn::os::CancelThreadSynchronization(&m_Thread);
        }
        inline s32 ChangePriority(s32 priority) {
            return nn::os::ChangeThreadPriority(&m_Thread, priority);
        }
        inline s32 GetPriority() const {
            return nn::os::GetThreadPriority(&m_Thread);
        }
        inline s32 GetCurrentPriority() const {
            return nn::os::GetThreadCurrentPriority(&m_Thread);
        }
        inline void SetName(const char *name) {
            nn::os::SetThreadName(&m_Thread, name);
        }
        inline void SetNamePointer(const char *name) {
            nn::os::SetThreadNamePointer(&m_Thread, name);
        }
        inline const char *GetNamePointer() const {
            return nn::os::GetThreadNamePointer(&m_Thread);
        }
        inline void SetCoreMask(s32 ideal_core, u64 affinity_mask) {
            nn::os::SetThreadCoreMask(&m_Thread, ideal_core, affinity_mask);
        }
        inline void GetCoreMask(s32 *out_ideal_core, u64 *out_affinity_mask) const {
            nn::os::GetThreadCoreMask(out_ideal_core, out_affinity_mask, &m_Thread);
        } 
        inline nn::os::ThreadId GetId() const {
            return nn::os::GetThreadId(&m_Thread);
        }
        inline ~Thread() {
            Destroy();
        }
    };
};