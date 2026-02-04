#include <nn/os/os_tick.hpp>
#include <base/pointers.hpp>
#include <base/memory/batch.hpp>
#include <base/memory/range.hpp>
#include <base/memory/handle.hpp>
#include <base/memory/pattern.hpp>
#include <base/memory/externals.hpp>

#include <format>
#include <vector>

extern const ThunkMetadata __start_dx_thunks;
extern const ThunkMetadata __stop_dx_thunks;

namespace base
{
	std::shared_ptr<Pointers> Pointers::pInstance = nullptr;

	/*
	* Currently designed to contain the desired byte patterns to search for in the main .text memory block.
	* The class creates a thread for each signature, and batches them up. Once all patterns are batched, it
	* lets the searcher rip by running the desired threads possible. This allows for at most 4 byte pattern
	* searches to occur in parallel given the switch's hardware limitations.
	*/
	Pointers::Pointers()
	{
		memory::batch<4> batch;
		this->mFunctionLookupTable = new std::unordered_map<const char*, uintptr_t>();

		const ThunkMetadata* it = &__start_dx_thunks;
    	const ThunkMetadata* end = &__stop_dx_thunks;

		// Get Main Module Info.
        exl::util::ModuleInfo info = exl::util::GetMainModuleInfo();
        exl::util::Range text = info.m_Text;

		uintptr_t mainStart = text.m_Start;

		for (; it < end; ++it) {
			batch.add(it->mName, it->mPattern, [this, it, mainStart](memory::handle handle){
				uintptr_t addr = handle.add(it->mOffset).as<uintptr_t>();
				*it->mPointer = addr + mainStart;

				this->mFunctionLookupTable->emplace(it->mName, addr);
			});
		}

		Logging.Log("Starting batch run!");

		// Run batch searcher.
		batch.run(memory::range(memory::handle(text.m_Start), text.m_Size));

		Logging.Log("Finished batch run!");
	}

	std::shared_ptr<Pointers> Pointers::create()
	{
		if (pInstance == nullptr)
		{
			auto instance = new Pointers();
			pInstance = std::shared_ptr<Pointers>(instance);
		}

		return pInstance;
	}
	
	std::shared_ptr<Pointers> Pointers::getInstance()
	{
		return pInstance;
	}

	/*
	* Destructor for Pointers object.
	*/
	Pointers::~Pointers()
	{
		// pInstance = nullptr;
	}

	/*
	* Gets the address with an offset of the results of the desired pattern given the key associated with it.
	*
	* @param key The key associated with the byte pattern.
	* @param offset An offset applied to the address of the byte pattern.
	*
	* @return The address of the byte pattern relative to the main .text memory block.
	*/
	uintptr_t Pointers::get(const char* key, u32 offset, bool adjust)
	{
		uintptr_t addr = this->get(key, adjust);
		if(addr)
			return reinterpret_cast<uintptr_t>(reinterpret_cast<uint8_t*>(addr) + offset);
		return 0;
	}

	/*
	* Gets the pointer of the results of the desired pattern given the key associated with it.
	*
	* @param key The key associated with the byte pattern.
	*
	* @return The address of the byte pattern relative to the main .text memory block.
	*/
	uintptr_t Pointers::get(const char* key, bool adjust)
	{
		if(this->mFunctionLookupTable->find(key) == mFunctionLookupTable->end()) return 0;

		if(adjust)
		{
			// Get Main Module Info.
			exl::util::ModuleInfo info = exl::util::GetMainModuleInfo();
			exl::util::Range text = info.m_Text;

			return this->mFunctionLookupTable->at(key) + text.m_Start;
		}

		return this->mFunctionLookupTable->at(key);
	}
}