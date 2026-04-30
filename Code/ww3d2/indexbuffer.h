/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : ww3d                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/indexbuffer.h                       $*
 *                                                                                             *
 *              Original Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 7/10/01 12:27p                                              $*
 *                                                                                             *
 *                    $Revision:: 12                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#if defined(_MSC_VER)
#pragma once
#endif

#ifndef INDEXBUFFER_H
#define INDEXBUFFER_H

#include "always.h"
#include "wwdebug.h"
#include "refcount.h"
#include "sphere.h"

#if RENEGADE_WITH_BGFX_RENDERER
#include <vector>
#include <bgfx/bgfx.h>
#endif

class DX8Wrapper;
class SortingRendererClass;
#if !RENEGADE_WITH_BGFX_RENDERER
struct IDirect3DIndexBuffer8;
#endif
class RenderIndexBufferClass;
class SortingIndexBufferClass;

// ----------------------------------------------------------------------------

class IndexBufferClass : public RefCountClass
{
protected:
	virtual ~IndexBufferClass();
public:
	IndexBufferClass(unsigned type, unsigned short index_count);

	void Copy(unsigned int* indices,unsigned start_index,unsigned index_count);
	void Copy(unsigned short* indices,unsigned start_index,unsigned index_count);

	inline unsigned short Get_Index_Count() const { return index_count; }

	inline unsigned Type() const { return type; }

	void Add_Engine_Ref() const;
	void Release_Engine_Ref() const;
	inline unsigned Engine_Refs() const { return engine_refs; }

	class WriteLockClass
	{
		IndexBufferClass* index_buffer;
		unsigned short* indices;
	public:
		WriteLockClass(IndexBufferClass* index_buffer);
		~WriteLockClass();

		unsigned short* Get_Index_Array() { return indices; }
	};

	class AppendLockClass
	{
		IndexBufferClass* index_buffer;
		unsigned short* indices;
	public:
		AppendLockClass(IndexBufferClass* index_buffer,unsigned start_index, unsigned index_range);
		~AppendLockClass();

		unsigned short* Get_Index_Array() { return indices; }
	};

	static unsigned Get_Total_Buffer_Count();
	static unsigned Get_Total_Allocated_Indices();
	static unsigned Get_Total_Allocated_Memory();

protected:
	mutable int					engine_refs;
	unsigned short				index_count;		// number of indices
	unsigned						type;
};


// Ordinary dynamic index data is staged in CPU memory until the renderer copies
// it into bgfx transient buffers at submit time. Sorted translucent paths keep
// their dedicated CPU-resident sorting buffers.
class DynamicIBAccessClass
{
	friend DX8Wrapper;
	friend SortingRendererClass;

	unsigned Type;
	unsigned short IndexCount;	
	unsigned short IndexBufferOffset;
	IndexBufferClass* IndexBuffer;

	void Allocate_Sorting_Dynamic_Buffer();
	void Allocate_Render_Dynamic_Buffer();

public:
	DynamicIBAccessClass(unsigned short type, unsigned short index_count);
	~DynamicIBAccessClass();

	unsigned Get_Type() const { return Type; }
	unsigned short Get_Index_Count() const { return IndexCount; }
	const IndexBufferClass *Peek_Index_Buffer() const { return IndexBuffer; }
	unsigned short Get_Index_Buffer_Offset() const { return IndexBufferOffset; }

	// Call at the end of the execution, or at whatever time you wish to release
	// the recycled dynamic index buffer.
	static void _Deinit();
	static void _Reset(bool frame_changed);

	// To lock the index buffer, create instance of this write class locally.
	// The buffer is automatically unlocked when you exit the scope.
	class WriteLockClass
	{
		DynamicIBAccessClass* DynamicIBAccess;
		unsigned short* Indices;		
	public:
		WriteLockClass(DynamicIBAccessClass* ib_access);
		~WriteLockClass();
		unsigned short* Get_Index_Array() { return Indices; }
	};

	friend WriteLockClass;
};


/**
** RenderIndexBufferClass
** Concrete render-index-buffer implementation used by the active renderer backends.
*/
class RenderIndexBufferClass : public IndexBufferClass
{
	friend IndexBufferClass::WriteLockClass;
	friend IndexBufferClass::AppendLockClass;
public:
	enum UsageType {
		USAGE_DEFAULT=0,
		USAGE_DYNAMIC=1,
		USAGE_SOFTWAREPROCESSING=2,
		USAGE_NPATCHES=4
	};

	RenderIndexBufferClass(unsigned short index_count,UsageType usage=USAGE_DEFAULT, unsigned type=0);
	~RenderIndexBufferClass();

	void Copy(unsigned int* indices,unsigned start_index,unsigned index_count);
	void Copy(unsigned short* indices,unsigned start_index,unsigned index_count);

#if RENEGADE_WITH_BGFX_RENDERER
	unsigned short *Get_Source_Index_Data();
	const unsigned short *Get_Source_Index_Data() const;
	bool Ensure_Bgfx_Buffer() const;
	bgfx::IndexBufferHandle Get_Bgfx_Index_Buffer() const;
#endif

#if !RENEGADE_WITH_BGFX_RENDERER
	inline IDirect3DIndexBuffer8* Get_Native_Index_Buffer()	{ return index_buffer; }
#endif
	
private:
#if !RENEGADE_WITH_BGFX_RENDERER
	IDirect3DIndexBuffer8*	index_buffer;		// actual dx8 index buffer
#else
	mutable bgfx::IndexBufferHandle BgfxIndexBuffer;
	mutable bool BgfxIndexBufferDirty;
	std::vector<unsigned short> IndexData;

	void Mark_Bgfx_Buffer_Dirty();
	bool Sync_Bgfx_Buffer() const;
#endif
};



class SortingIndexBufferClass : public IndexBufferClass
{
	friend DX8Wrapper;
	friend SortingRendererClass;
	friend IndexBufferClass::WriteLockClass;
	friend IndexBufferClass::AppendLockClass;
	friend DynamicIBAccessClass::WriteLockClass;
public:
	SortingIndexBufferClass(unsigned short index_count);
	~SortingIndexBufferClass();

protected:
	unsigned short* index_buffer;
};

#endif //INDEXBUFFER_H
