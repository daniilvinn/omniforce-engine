#pragma once

#include <Log/Logger.h>
#include <Foundation/Macros.h>
#include <Foundation/Types.h>
#include <Core/Utils.h>

namespace Omni {

	class OMNIFORCE_API BitStream {
	public:
		BitStream(uint32 size)
		{
			OMNIFORCE_ASSERT_TAGGED(size % 4 == 0, "Bit stream size can only be a multiple of 4");
			OMNIFORCE_ASSERT_TAGGED(size >= 4, "Bit stream size must be greater or equal than 4");

			m_StorageSize = Utils::Align(size, 4u);

			m_Storage = new uint32[size / 4u];
			m_NumBitsUsed = 0;

			memset(m_Storage, 0, m_StorageSize);
		}

		~BitStream()
		{
			delete m_Storage;
		}

		void Append(uint32 num_bits, uint32 data) {
			// Resize if overflow detected
			if (m_NumBitsUsed + num_bits > m_StorageSize * 8u)
				Resize(Utils::Align(m_StorageSize * 1.2, 4));

			// Check if num bits is within (0, 32] range 
			OMNIFORCE_ASSERT_TAGGED(num_bits <= 32 && num_bits > 0, "num_bits must be in (0, 32] range");

			// Optimization by using right shift instead of division by 32.
			// Here I find uint32 cell index within a bit stream storage.
			// It works because shifting to the right is equal to the division by value which is a power of 2.
			uint32 index = m_NumBitsUsed >> 5; // 5 is log2(32)

			// Optimization by using bit masking instead of modulo op.
			// It works because I needed do find a 32 modulo of `data`, and 32 is 2^5, 
			// so I can simply bitmask all other bits except of first 4. 
			// This way only first 4 bits are left, which is the value I needed
			uint32 local_offset = m_NumBitsUsed & 0x1F;

			// Write first half of a value to current bit stream "cell" (uint32). If value fits entirely,
			// then I write entire value to a cell with bit offset of `local_offset`
			m_Storage[index] |= (data << local_offset);

			// If it overflows, we have to write another half of the value to the next cell
			if (local_offset + num_bits > 32u)
				// Shift value to the right with shift amount being amount of written bits in previous write.
				// This way we "discard" bits we've already written and write to the very beginning of next cell.
				m_Storage[index + 1] |= (data >> (32u - local_offset));

			m_NumBitsUsed += num_bits;
		}

		uint32 Read(uint32 num_bits, uint32 bit_offset) const {
			// Assertion to check if read will be out of bit stream bounds. 
			OMNIFORCE_ASSERT_TAGGED(bit_offset + num_bits <= m_StorageSize * 8u, "Out-of-bounds bit stream read");

			// Check if num bits is within (0, 32] range 
			OMNIFORCE_ASSERT_TAGGED(num_bits <= 32 && num_bits > 0, "num_bits must be in (0, 32] range");

			uint32 value = 0;

			// Optimization by using right shift instead of division by 32.
			// Here I find uint32 cell index within a bit stream storage based on requested offset.
			// It works because shifting to the right is equal to the division by value which is a power of 2.
			uint32 index = bit_offset >> 5; // >> 5 instead of division by 32

			// Optimization by using bit masking instead of modulo op.
			// Here I find a bit offset within a single uint32 cell
			// It works because I needed do find a 32 modulo of `data`, and 32 is 2^5, 
			// so I can simply bitmask all other bits except of first 4. 
			// This way only first 4 bits are left, which is the value I needed
			uint32 local_offset = bit_offset & 0x1F;

			// Create a bitmask based on local offset and num of bits to read,
			// which will allow to read only necessary values from a cell
			uint32 bitmask = ((1 << num_bits) - 1) << local_offset;

			// Shift the read bits back to the beginning of a value
			value = (m_Storage[index] & bitmask) >> local_offset;

			// If value is not encoded entirely within a single cell, we need to do a second read from next cell
			if (local_offset + num_bits > 32u) {
				// Write new bitmask based on num of bits left to read
				bitmask = ((1 << (num_bits - (32u - local_offset))) - 1);

				// Read value using bitmask and shift it to the right, so values which were read by previous read are not overriden
				value |= (m_Storage[index + 1] & bitmask) << (32u - local_offset);
			}

			return value;
		}

		void Resize(uint32 new_size) {
			OMNIFORCE_ASSERT_TAGGED(new_size % 4 == 0, "Size must be a multiple of 4");

			uint32* new_storage = new uint32[new_size];
			memset(new_storage, 0, new_size);
			memcpy(new_storage, m_Storage, GetNumStorageBytesUsed());

			delete[] m_Storage;
			m_Storage = new_storage;
			m_StorageSize = new_size;
		}

		const uint32* GetStorage()		const { return m_Storage; }
		uint32 GetNumBitsUsed()			const { return m_NumBitsUsed; }
		uint32 GetNumBytesUsed()		const { return (m_NumBitsUsed + 7u) / 8u; }
		uint32 GetNumStorageBytesUsed() const { return Utils::Align((m_NumBitsUsed + 7) / 8u, 4); }
		uint32 GetStorageSize()			const { return m_StorageSize; }

	private:
		uint32* m_Storage;
		uint32 m_StorageSize;
		uint32 m_NumBitsUsed;
	};

}