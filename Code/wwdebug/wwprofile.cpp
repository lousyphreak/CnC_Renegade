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
 *                 Project Name : WWDebug                                                      *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwdebug/wwprofile.cpp                        $*
 *                                                                                             *
 *                      $Author:: Jani_p                                                      $*
 *                                                                                             *
 *                     $Modtime:: 4/01/02 10:30a                                              $*
 *                                                                                             *
 *                    $Revision:: 20                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *   WWProfile_Get_Ticks -- Retrieves the cpu performance counter                              *
 *   WWProfileHierachyNodeClass::WWProfileHierachyNodeClass -- Constructor                     *
 *   WWProfileHierachyNodeClass::~WWProfileHierachyNodeClass -- Destructor                     *
 *   WWProfileHierachyNodeClass::Get_Sub_Node -- Searches for a child node by name (pointer)   *
 *   WWProfileHierachyNodeClass::Reset -- Reset all profiling data in the tree                 *
 *   WWProfileHierachyNodeClass::Call -- Start timing                                          *
 *   WWProfileHierachyNodeClass::Return -- Stop timing, record results                         *
 *   WWProfileManager::Start_Profile -- Begin a named profile                                  *
 *   WWProfileManager::Stop_Profile -- Stop timing and record the results.                     *
 *   WWProfileManager::Reset -- Reset the contents of the profiling system                     *
 *   WWProfileManager::Increment_Frame_Counter -- Increment the frame counter                  *
 *   WWProfileManager::Get_Time_Since_Reset -- returns the elapsed time since last reset       *
 *   WWProfileManager::Get_Iterator -- Creates an iterator for the profile tree                *
 *   WWProfileManager::Release_Iterator -- Return an iterator for the profile tree             *
 *   WWProfileManager::Get_In_Order_Iterator -- Creates an "in-order" iterator for the profile *
 *   WWProfileManager::Release_In_Order_Iterator -- Return an "in-order" iterator              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "wwprofile.h"
#include "wwdebug.h"
#include "wwmemlog.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_thread.h>
#include <SDL3/SDL_timer.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

namespace {

std::vector<WWProfileHierachyNodeClass*> g_profile_collect_vector;
double g_total_frame_times = 0.0;
bool g_profile_collecting = false;

std::int64_t WWProfile_Query_Ticks()
{
	return static_cast<std::int64_t>(SDL_GetPerformanceCounter());
}

float WWProfile_Query_Seconds_Per_Tick()
{
	const Uint64 frequency = SDL_GetPerformanceFrequency();
	if (frequency == 0) {
		return 0.0f;
	}
	return static_cast<float>(1.0 / static_cast<double>(frequency));
}

std::string WWProfile_Format_String(const char * format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	va_list copy;
	va_copy(copy, arguments);
	const int required = std::vsnprintf(nullptr, 0, format, copy);
	va_end(copy);

	std::string buffer;
	if (required > 0) {
		buffer.resize(static_cast<std::size_t>(required));
		std::vsnprintf(buffer.data(), buffer.size() + 1, format, arguments);
	}
	va_end(arguments);
	return buffer;
}

bool WWProfile_Write_Text(SDL_IOStream * stream, const std::string & text)
{
	if (stream == nullptr || text.empty()) {
		return stream != nullptr;
	}
	return SDL_WriteIO(stream, text.data(), text.size()) == text.size();
}

void WWProfile_Write_Node(SDL_IOStream * stream, const WWProfileHierachyNodeClass * node, int recursion)
{
	if (node == nullptr) {
		return;
	}

	if (node->Get_Total_Time() != 0.0f) {
		std::string line(static_cast<std::size_t>(recursion), '\t');
		line += WWProfile_Format_String("%s\t%d\t%f\r\n", node->Get_Name(), node->Get_Total_Calls(), node->Get_Total_Time() * 1000.0f);
		WWProfile_Write_Text(stream, line);
	}

	WWProfile_Write_Node(stream, node->Get_Child(), recursion + 1);
	WWProfile_Write_Node(stream, node->Get_Sibling(), recursion);
}

} // namespace

unsigned WWProfile_Get_System_Time()
{
	return static_cast<unsigned>(SDL_GetTicks());
}

/***********************************************************************************************
 * WWProfile_Get_Ticks -- Retrieves the cpu performance counter                                *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
inline void WWProfile_Get_Ticks(std::int64_t * ticks)
{
	*ticks = WWProfile_Query_Ticks();
}

inline float WWProfile_Get_Seconds_Per_Tick()
{
	return WWProfile_Query_Seconds_Per_Tick();
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::WWProfileHierachyNodeClass -- Constructor                       *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 * name - pointer to a static string which is the name of this profile node                    *
 * parent - parent pointer                                                                     *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 * The name is assumed to be a static pointer, only the pointer is stored and compared for     *
 * efficiency reasons.                                                                         *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
WWProfileHierachyNodeClass::WWProfileHierachyNodeClass( const char * name, WWProfileHierachyNodeClass * parent ) :
	Name( name ),
	TotalCalls( 0 ),
	TotalTime( 0 ),
	StartTime( 0 ),
	RecursionCounter( 0 ),
	Parent( parent ),
	Child( NULL ),
	Sibling( NULL )
{
	Reset();
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::~WWProfileHierachyNodeClass -- Destructor                       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
WWProfileHierachyNodeClass::~WWProfileHierachyNodeClass( void )
{
	delete Child;
	delete Sibling;
}


WWProfileHierachyNodeClass* WWProfileHierachyNodeClass::Clone_Hierarchy(WWProfileHierachyNodeClass* parent)
{
	WWProfileHierachyNodeClass* node=new WWProfileHierachyNodeClass(Name,parent);
	node->TotalCalls=TotalCalls;
	node->TotalTime=TotalTime;
	node->StartTime=StartTime;
	node->RecursionCounter=RecursionCounter;
	
	if (Child) {
		node->Child=Child->Clone_Hierarchy(node);
	}
	if (Sibling) {
		node->Sibling=Sibling->Clone_Hierarchy(parent);
	}

	return node;
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::Get_Sub_Node -- Searches for a child node by name (pointer)     *
 *                                                                                             *
 * INPUT:                                                                                      *
 * name - static string pointer to the name of the node we are searching for                   *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 * All profile names are assumed to be static strings so this function uses pointer compares   *
 * to find the named node.                                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
WWProfileHierachyNodeClass * WWProfileHierachyNodeClass::Get_Sub_Node( const char * name )
{
	// Try to find this sub node
	WWProfileHierachyNodeClass * child = Child;
	while ( child ) {
		if ( child->Name == name ) {
			return child;
		}
		child = child->Sibling;
	}

	// We didn't find it, so add it
	WWProfileHierachyNodeClass * node = new WWProfileHierachyNodeClass( name, this );
	node->Sibling = Child;
	Child = node;
	return node;
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::Reset -- Reset all profiling data in the tree                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileHierachyNodeClass::Reset( void )
{
	TotalCalls = 0;
	TotalTime = 0.0f;

	if ( Child ) {
		Child->Reset();
	}
	if ( Sibling ) {
		Sibling->Reset();
	}
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::Call -- Start timing                                            *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileHierachyNodeClass::Call( void )
{
	TotalCalls++;
	if (RecursionCounter++ == 0) {
		WWProfile_Get_Ticks(&StartTime);
	}
}


/***********************************************************************************************
 * WWProfileHierachyNodeClass::Return -- Stop timing, record results                           *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
bool	WWProfileHierachyNodeClass::Return( void )
{
	if (--RecursionCounter == 0) {
		if ( TotalCalls != 0 ) {
			std::int64_t time;
			WWProfile_Get_Ticks(&time);
			time-=StartTime;

			TotalTime += static_cast<float>(double(time) * WWProfile_Get_Seconds_Per_Tick());
		}
	}
	return RecursionCounter == 0;
}


/***************************************************************************************************
**
** WWProfileManager Implementation
**
***************************************************************************************************/
WWProfileHierachyNodeClass		WWProfileManager::Root( "Root", NULL );
WWProfileHierachyNodeClass	*	WWProfileManager::CurrentNode = &WWProfileManager::Root;
WWProfileHierachyNodeClass	*	WWProfileManager::CurrentRootNode = &WWProfileManager::Root;
int									WWProfileManager::FrameCounter = 0;
std::int64_t						WWProfileManager::ResetTime = 0;

static SDL_ThreadID				ThreadID = 0;


/***********************************************************************************************
 * WWProfileManager::Start_Profile -- Begin a named profile                                    *
 *                                                                                             *
 * Steps one level deeper into the tree, if a child already exists with the specified name     *
 * then it accumulates the profiling; otherwise a new child node is added to the profile tree. *
 *                                                                                             *
 * INPUT:                                                                                      *
 * name - name of this profiling record                                                        *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 * The string used is assumed to be a static string; pointer compares are used throughout      *
 * the profiling code for efficiency.                                                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileManager::Start_Profile( const char * name )
{
	if (SDL_GetCurrentThreadID() != ThreadID) {
		return;
	}

//	int current_thread = ::GetCurrentThreadId();
	if (name != CurrentNode->Get_Name()) {
		CurrentNode = CurrentNode->Get_Sub_Node( name );
	}

	CurrentNode->Call();
}

void	WWProfileManager::Start_Root_Profile( const char * name )
{
	if (SDL_GetCurrentThreadID() != ThreadID) {
		return;
	}

	if (name != CurrentRootNode->Get_Name()) {
		CurrentRootNode = CurrentRootNode->Get_Sub_Node( name );
	}

	CurrentRootNode->Call();
}


/***********************************************************************************************
 * WWProfileManager::Stop_Profile -- Stop timing and record the results.                       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileManager::Stop_Profile( void )
{
	if (SDL_GetCurrentThreadID() != ThreadID) {
		return;
	}

	// Return will indicate whether we should back up to our parent (we may
	// be profiling a recursive function)
	if (CurrentNode->Return()) {
		CurrentNode = CurrentNode->Get_Parent();
	}
}

void	WWProfileManager::Stop_Root_Profile( void )
{
	if (SDL_GetCurrentThreadID() != ThreadID) {
		return;
	}

	// Return will indicate whether we should back up to our parent (we may
	// be profiling a recursive function)
	if (CurrentRootNode->Return()) {
		CurrentRootNode = CurrentRootNode->Get_Parent();
	}
}


/***********************************************************************************************
 * WWProfileManager::Reset -- Reset the contents of the profiling system                       *
 *                                                                                             *
 *    This resets everything except for the tree structure.  All of the timing data is reset.  *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileManager::Reset( void )
{  
	ThreadID = SDL_GetCurrentThreadID();

	Root.Reset();
	FrameCounter = 0;
	WWProfile_Get_Ticks(&ResetTime);
}


/***********************************************************************************************
 * WWProfileManager::Increment_Frame_Counter -- Increment the frame counter                    *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void WWProfileManager::Increment_Frame_Counter( void )
{
	if (g_profile_collecting) {
		float time=Get_Time_Since_Reset();
		g_total_frame_times+=time;
		WWProfileHierachyNodeClass* new_root=Root.Clone_Hierarchy(NULL);
		new_root->Set_Total_Time(time);
		new_root->Set_Total_Calls(1);
		g_profile_collect_vector.push_back(new_root);
		Reset();
	}

	FrameCounter++;

}


/***********************************************************************************************
 * WWProfileManager::Get_Time_Since_Reset -- returns the elapsed time since last reset         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
float WWProfileManager::Get_Time_Since_Reset( void )
{
	std::int64_t time;
	WWProfile_Get_Ticks(&time);
	time -= ResetTime;

	return static_cast<float>(double(time) * WWProfile_Get_Seconds_Per_Tick());
}


/***********************************************************************************************
 * WWProfileManager::Get_Iterator -- Creates an iterator for the profile tree                  *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
WWProfileIterator *	WWProfileManager::Get_Iterator( void )
{
	return new WWProfileIterator( &Root );
}


/***********************************************************************************************
 * WWProfileManager::Release_Iterator -- Return an iterator for the profile tree               *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileManager::Release_Iterator( WWProfileIterator * iterator )
{
	delete iterator;
}


void	WWProfileManager::Begin_Collecting()
{
	Reset();
	g_profile_collecting=true;
	g_total_frame_times=0.0;
}

void	WWProfileManager::End_Collecting(const char* filename)
{
	if ((filename != nullptr) && !g_profile_collect_vector.empty()) {
		SDL_IOStream * file = SDL_IOFromFile(filename, "wb");
		if (file != nullptr) {
			const float avg_frame_time = static_cast<float>(g_total_frame_times / static_cast<double>(g_profile_collect_vector.size()));
			WWProfile_Write_Text(file, WWProfile_Format_String(
				"Total frames: %zu, average frame time: %fms\r\n"
				"All frames taking more than twice the average frame time are marked with keyword SPIKE.\r\n\r\n",
				g_profile_collect_vector.size(),
				avg_frame_time * 1000.0f));

			for (std::size_t index = 0; index < g_profile_collect_vector.size(); ++index) {
				const float frame_time = g_profile_collect_vector[index]->Get_Total_Time();
				WWProfile_Write_Text(file, WWProfile_Format_String(
					"FRAME: %zu %fms %s ---------------\r\n",
					index,
					frame_time * 1000.0f,
					(frame_time > avg_frame_time * 2.0f) ? "SPIKE" : ""));
				WWProfile_Write_Node(file, g_profile_collect_vector[index], 0);
			}

			SDL_CloseIO(file);
		} else {
			WWDEBUG_WARNING(("Failed to open profile output '%s': %s\n", filename, SDL_GetError()));
		}
	}

	for (WWProfileHierachyNodeClass * node : g_profile_collect_vector) {
		delete node;
	}
	g_profile_collect_vector.clear();
	g_profile_collecting=false;
}



/***********************************************************************************************
 * WWProfileManager::Get_In_Order_Iterator -- Creates an "in-order" iterator for the profile t *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
WWProfileInOrderIterator * WWProfileManager::Get_In_Order_Iterator( void )
{
	return new WWProfileInOrderIterator;
}


/***********************************************************************************************
 * WWProfileManager::Release_In_Order_Iterator -- Return an "in-order" iterator                *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   9/24/2000  gth : Created.                                                                 *
 *=============================================================================================*/
void	WWProfileManager::Release_In_Order_Iterator( WWProfileInOrderIterator * iterator )
{
	delete iterator;
}


/***************************************************************************************************
**
** WWProfileIterator Implementation
**
***************************************************************************************************/
WWProfileIterator::WWProfileIterator( WWProfileHierachyNodeClass * start )
{
	CurrentParent = start;
	CurrentChild = CurrentParent->Get_Child();
}

void	WWProfileIterator::First(void)
{
	CurrentChild = CurrentParent->Get_Child();
}


void	WWProfileIterator::Next(void)
{
	CurrentChild = CurrentChild->Get_Sibling();
}

bool	WWProfileIterator::Is_Done(void)
{
	return CurrentChild == NULL;
}

void	WWProfileIterator::Enter_Child( void )
{
	CurrentParent = CurrentChild;
	CurrentChild = CurrentParent->Get_Child();
}

void	WWProfileIterator::Enter_Child( int index )
{
	CurrentChild = CurrentParent->Get_Child();
	while ( (CurrentChild != NULL) && (index != 0) ) {
		index--;
		CurrentChild = CurrentChild->Get_Sibling();
	}

	if ( CurrentChild != NULL ) {
		CurrentParent = CurrentChild;
		CurrentChild = CurrentParent->Get_Child();
	}
}

void	WWProfileIterator::Enter_Parent( void )
{
	if ( CurrentParent->Get_Parent() != NULL ) {
		CurrentParent = CurrentParent->Get_Parent();
	}
	CurrentChild = CurrentParent->Get_Child();
}

/***************************************************************************************************
**
** WWProfileInOrderIterator Implementation
**
***************************************************************************************************/

WWProfileInOrderIterator::WWProfileInOrderIterator( void )
{
	CurrentNode = &WWProfileManager::Root;
}

void	WWProfileInOrderIterator::First(void)
{
	CurrentNode = &WWProfileManager::Root;
}

void	WWProfileInOrderIterator::Next(void)
{
	if ( CurrentNode->Get_Child() ) {				// If I have a child, go to child
		CurrentNode = CurrentNode->Get_Child();
	} else if ( CurrentNode->Get_Sibling() ) {	// If I have a sibling, go to sibling
		CurrentNode = CurrentNode->Get_Sibling();
	} else {											//	if not, go to my parent's sibling, or his.......
		// Find a parent with a sibling....
		bool done = false;
		while ( CurrentNode != NULL && !done ) {

			// go to my parent
			CurrentNode = CurrentNode->Get_Parent();

			// If I have a sibling, go there
			if ( CurrentNode != NULL && CurrentNode->Get_Sibling() != NULL ) {
				CurrentNode = CurrentNode->Get_Sibling();
				done = true;
			}
		}
	}
}

bool	WWProfileInOrderIterator::Is_Done(void)
{
	return CurrentNode == NULL;
}

/*
**
*/
WWTimeItClass::WWTimeItClass( const char * name )
{
	Name = name;
	WWProfile_Get_Ticks( &Time );
}

WWTimeItClass::~WWTimeItClass( void )
{
	std::int64_t End;
	WWProfile_Get_Ticks( &End );
	End -= Time;
#ifdef WWDEBUG
	float time = static_cast<float>(End * WWProfile_Get_Seconds_Per_Tick());
	WWDEBUG_SAY(( "*** WWTIMEIT *** %s took %1.9f\n", Name, time ));
#endif
}


/*
**
*/
WWMeasureItClass::WWMeasureItClass( float * p_result )
{
	WWASSERT(p_result != NULL);
	PResult = p_result;
	WWProfile_Get_Ticks( &Time );
}

WWMeasureItClass::~WWMeasureItClass( void )
{
	std::int64_t End;
	WWProfile_Get_Ticks( &End );
	End -= Time;
	WWASSERT(PResult != NULL);
	*PResult = static_cast<float>(End * WWProfile_Get_Seconds_Per_Tick());
}

// ----------------------------------------------------------------------------
//
//
//
// ----------------------------------------------------------------------------

unsigned WWMemoryAndTimeLog::TabCount;

WWMemoryAndTimeLog::WWMemoryAndTimeLog(const char* name)
	:
	Name((name != NULL) ? name : "<unnamed>"),
	TimeStart(WWProfile_Get_System_Time()),
	AllocCountStart(WWMemoryLogClass::Get_Current_Allocation_Count()),
	AllocSizeStart(WWMemoryLogClass::Get_Current_Allocated_Size())
{
	IntermediateTimeStart=TimeStart;
	IntermediateAllocCountStart=AllocCountStart;
	IntermediateAllocSizeStart=AllocSizeStart;
	std::string indent(static_cast<std::size_t>(TabCount), '\t');
	WWRELEASE_SAY(("%s%s {\n", indent.c_str(), Name));
	TabCount++;
}

WWMemoryAndTimeLog::~WWMemoryAndTimeLog()
{
	if (TabCount>0) TabCount--;
	std::string indent(static_cast<std::size_t>(TabCount), '\t');
	WWRELEASE_SAY(("%s} ", indent.c_str()));

	unsigned current_time=WWProfile_Get_System_Time();
	int current_alloc_count=WWMemoryLogClass::Get_Current_Allocation_Count();
	int current_alloc_size=WWMemoryLogClass::Get_Current_Allocated_Size();
	WWRELEASE_SAY(("IN TOTAL %s took %d.%3.3d s, did %d memory allocations of %d bytes\n",
		Name,
		(current_time - TimeStart)/1000, (current_time - TimeStart)%1000,
		current_alloc_count - AllocCountStart,
		current_alloc_size - AllocSizeStart));
	WWRELEASE_SAY(("\n"));

}


void WWMemoryAndTimeLog::Log_Intermediate(const char* text)
{
	unsigned current_time=WWProfile_Get_System_Time();
	int current_alloc_count=WWMemoryLogClass::Get_Current_Allocation_Count();
	int current_alloc_size=WWMemoryLogClass::Get_Current_Allocated_Size();
	std::string indent(static_cast<std::size_t>(TabCount), '\t');
	WWRELEASE_SAY(("%s%s took %d.%3.3d s, did %d memory allocations of %d bytes\n",
		indent.c_str(),
		text,
		(current_time - IntermediateTimeStart)/1000, (current_time - IntermediateTimeStart)%1000,
		current_alloc_count - IntermediateAllocCountStart,
		current_alloc_size - IntermediateAllocSizeStart));
	IntermediateTimeStart=current_time;
	IntermediateAllocCountStart=current_alloc_count;
	IntermediateAllocSizeStart=current_alloc_size;
}