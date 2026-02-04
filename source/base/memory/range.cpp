#include "lib.hpp"

#include <base/memory/range.hpp>

#include <base/memory/pattern.hpp>

#include <format>
#include <limits>

namespace base::memory
{
	range::range(handle base, std::size_t size) : m_base(base), m_size(size) {}

	handle range::begin()
	{
		return m_base;
	}

	handle range::end()
	{
		return m_base.add(m_size);
	}

	std::size_t range::size()
	{
		return m_size;
	}

	bool range::contains(handle h)
	{
		return h.as<std::uintptr_t>() >= begin().as<std::uintptr_t>() && h.as<std::uintptr_t>() <= end().as<std::uintptr_t>();
	}

	/*
	* The following is an implementation of the boyer moore horspool algorithm
	* used to find a pattern in a string, but in this case, a byte pattern
	* in the game's .text region. The documentation I provide here is of
	* my own interpretation and understanding of the algorithm as I am not
	* the original author.
	*
	* @param data The byte pattern containing the signature to search.
	* @param data_length The length in bytes of the signature.
	* @param base The handle containing the location in memory of the .text region.
	* @param base_size The total size in bytes of the .text memory region.
	*
	* @return The address relative to the .text memory region.
	*/
	static handle boyer_moore_horspool(std::optional<std::uint8_t> const *data, std::size_t data_length, handle base, std::size_t base_size)
	{
		auto shift_max = data_length, index_max = data_length - 1, index_wildcard = std::numeric_limits<std::size_t>::max();
		std::size_t shift_table[std::numeric_limits<std::uint8_t>::max() + 1];
		
		// Starts from the end of the desired search pattern and searches for the
		// right most wild card. We constrain the maximum shift value to be the index
		// of the wildcard relative to the end of the pattern. This seems to be to avoid
		// generating an invalid shift value for wildcard spots, but also seems to degrade
		// the search performance as the wildcard is found closer to the end of the pattern. 
		for (auto i = index_max - 1; i >= 0; --i)
			if (!data[i])
			{
				shift_max = index_max - i;
				index_wildcard = i;
				break;
			}
			
		// Default each entry in the shift table to shift max. The idea is, if an entry in
		// the shift table is met that is not a part of the sig, then we should skip it entirely and 
		// shift the entire pattern, skipping the offending value.
		for (auto &e : shift_table)
			e = shift_max;
			
		// Populates the shift table where each entry in the table is the
		// amount to shift the pattern on a mismatch for that specific byte.
		for (auto i = index_wildcard + 1; i < index_max; ++i)
			shift_table[*data[i]] = index_max - i;
			
		// Scan the data block for the desired byte pattern.
		for (std::size_t index_cur = 0; index_cur <= base_size - data_length;)
			for (auto index_sig = index_max; index_sig >= 0; --index_sig)
			{
				// Shift pattern on a byte mismatch.
				if (data[index_sig] && *data[index_sig] != base.add(index_cur + index_sig).as<std::uint8_t &>())
				{
					index_cur += shift_table[base.add(index_cur + index_max).as<std::uint8_t &>()];
					break;
				}
				
				// Return the offset relative to the data block when the pattern is found.
				if (index_sig == 0)
					return handle(reinterpret_cast<void*>(index_cur));
			}
			
		return nullptr;
	}

	/*
	* Helper function, being a pre-step before the actual pattern search.
	* This pulls the necessary information from the signature and casts the
	* data block to the appropriate type expected for the algorithm.
	*
	* @param sig The desired signature pattern to find.
	*
	* @return The handle containing the address of the signature pattern relative
	* to the .text memory region.
	*/
	handle range::scan(pattern const &sig)
	{
		auto data = sig.m_bytes.data();
		auto length = sig.m_bytes.size();

		return boyer_moore_horspool(data, length, m_base, m_size);
	}
}
