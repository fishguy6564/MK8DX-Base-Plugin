#pragma once

#include "lib.hpp"

#include <unordered_map>
#include <string>
#include <memory>

namespace base
{
	class Pointers
	{
	private:
		Pointers();

		static std::shared_ptr<Pointers> pInstance;

	public:
		static std::shared_ptr<Pointers> create();
		static std::shared_ptr<Pointers> getInstance();

		uintptr_t get(const char* key, bool adjust = true);
		uintptr_t get(const char* key, u32 offset, bool adjust = false);

		std::unordered_map<const char*, uintptr_t>* mFunctionLookupTable;

		~Pointers();

		template<typename T, typename... Args>
		static T call(const char* key, Args... args)
		{
			uintptr_t func = pInstance->get(key, true);

			if(!func)
			{
				if constexpr(std::is_void_v<T>)
					return;
				return T{};
			}

			using Fn = T(*)(Args...);
			Fn funcPtr = reinterpret_cast<Fn>(func);

			if constexpr(std::is_void_v<T>)
			{
				funcPtr(args...);
				return;
			}
				
			return funcPtr(args...);
		}
	};
}