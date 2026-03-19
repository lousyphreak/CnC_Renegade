#pragma once

#include "refcount.h"

#include <vector>

class DX8IndexBufferClass : public RefCountClass
{
public:
	explicit DX8IndexBufferClass(int = 0)
	{
	}
};

class DynamicIBAccessClass
{
public:
	DynamicIBAccessClass(int, int index_count)
		: Storage(static_cast<size_t>(index_count), 0)
	{
	}

	class WriteLockClass
	{
	public:
		explicit WriteLockClass(DynamicIBAccessClass * access)
			: Access(access)
		{
		}

		unsigned short * Get_Index_Array()
		{
			return Access != nullptr ? Access->Storage.data() : nullptr;
		}

	private:
		DynamicIBAccessClass * Access;
	};

private:
	std::vector<unsigned short> Storage;
};
