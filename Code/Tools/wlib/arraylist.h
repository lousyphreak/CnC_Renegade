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

/****************************************************************************\
*        C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S         *
******************************************************************************
Project Name: 
File Name   : arraylist.h
Author      : Neal Kettler
Start Date  : Jan 19, 1997
Last Update : Jan 19, 1997
------------------------------------------------------------------------------

Array implementation of a list.  Note: There are some freaky C++ memory tricks
going on here.  If you think there's a leak, see me before changing it.
The way this works is that it allocates an array to hold 'N' items on the
first list add.  It doesn't call the constructors for those 'N' items until
necessary (for efficiency).  When an item is added to a slot then a new
class is constructed inside the array element using the placement new operator
and the class's copy constructor.  When elements are removed the destructor
is then manually called on this memory location.

All data added to the list is copied so feel free to delete/destroy/modify
the original after an add.

You _must_ have a good copy constructor for any classes that you use this template
for!  A good copy constructor is one that won't blindly duplicate pointers
that don't belong to them, etc...

\****************************************************************************/

#ifndef ARRAYLIST_HEADER
#define ARRAYLIST_HEADER    

#include <cstdint>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <new.h>
#include <math.h>

#include "wstypes.h"

//
// Usage: ArrayList<int> TheList;
//
template <class T>
class ArrayList
{
 public:
                   ArrayList();
                   ArrayList(ArrayList<T> &other);
                  ~ArrayList();

  // Remove all entries from the lsit 
  void             clear(void);

  // Add a node after the zero based 'pos'
  int8_t             add(IN T &node,int32_t pos);
  int8_t             addTail(IN T &node);
  int8_t             addHead(IN T &node);
  int8_t		   addSortedAsc(IN T &node);		// Ascending
  int8_t		   addSortedDes(IN T &node);		// Descending
  /*int8_t		   addNumSortedAsc(IN T &node);		// Ascending
  int8_t		   addNumSortedDes(IN T &node);		// Descending*/
  int8_t             addMany(OUT T *outarray, int32_t pos, int32_t howmany);

  // Remove a node
  int8_t             remove(OUT T &node,int32_t pos);
  int8_t             remove(int32_t pos);
  int8_t             removeHead(OUT T &node);
  int8_t             removeTail(OUT T &node);
  int32_t           removeMany(OUT T *outarray, int32_t pos, int32_t howmany);

  // Replace one obj with another
  int8_t             replace(IN T &node, int32_t pos);


  // Get a node without removing from the list
  int8_t             get(OUT T &node,int32_t pos) RO;
  int8_t             getHead(OUT T &node) RO;
  int8_t             getTail(OUT T &node) RO;        

  // Get a pointer to the interally managed copy (careful!)
  int8_t             getPointer(OUT T **node,int32_t pos) RO;

  // Get the number of entries in the list
  int32_t           length(void) RO;

  // UNSAFE! for classes, see note below!
  int8_t             setSize(int32_t newsize, IN T &filler);

  // Print information on the list
  void             print(FILE *out);

  // assignment operator
  ArrayList<T>   &operator=(IN ArrayList<T> &other);

 private:
  int32_t 	   _sortedLookup(IN T &target, int ascending);
  int32_t           Entries_;   // Number of entries
  int32_t           Slots_;     // Number of available slots

  T               *Vector_;    // The actual memory where the list is held

  enum
  {
    INITIAL_SIZE = 10
  };

  int8_t             growVector(void);   // Expand the number of slots
  int8_t             shrinkVector(void); // Reduce the number of slots
};


//Create the empty list 
template <class T>
ArrayList<T>::ArrayList()
{
  Entries_=0;
  Slots_=0;
  Vector_=NULL;
}

// copy constructor
template <class T>
ArrayList<T>::ArrayList(ArrayList<T> &other)
{
  Entries_=0;
  Slots_=0;
  Vector_=NULL;
  (*this)=other;
}

//Free all the memory...
template <class T>
ArrayList<T>::~ArrayList()
{
  clear();                   // Remove the entries & call destructors on them

  delete[]((uint8_t*)Vector_); // this will prevent the destructors from
                             //  gettting called on elements not
                             //  containing valid objects.

  //fprintf(stderr,"Arraylist destructor\n");
}

// assignment operator
template <class T>
ArrayList<T> &ArrayList<T>::operator=(IN ArrayList<T> &other)
{
  T node;
  clear();
  for (int i=0; i<other.length(); i++)
  {
    other.get(node,i);
    addTail(node);
  } 
  return(*this);
}


// Remove all the entries and free the memory
template <class T>
void ArrayList<T>::clear()
{
  for (int i=0; i<Entries_; i++)
  {
    (Vector_+i)->~T();    // Call the destructor manually. Don't try this
                          //  at home kiddies!
  }
  Entries_=0;
}            

// ************************* UNSAFE UNSAFE UNSAFE *************************
// Note - Don't call this on any type with a constructor/destructor since this
//  is really dumb and just puts a new one of filler in. Well, it's kindof safe
//  just be careful.
// It's fine for simple classes like ints though..
//
// Add/remove entries in a stupid manner...
//
// **************************************************************************
template <class T>
int8_t ArrayList<T>::setSize(int32_t newsize, IN T &filler)
{
  int oldEntries=Entries_;
  Entries_ = newsize;

  if (newsize<0)
    return(false);

  // Grow the vector as much as we need to
  while (newsize > Slots_)
    growVector();

  // Create new objects in the blank holes
  for (int i=oldEntries; i<Entries_; i++)
  {
    // Now put the replacement object in there...
    new((void *)(Vector_+i)) T(filler); // Trust me, this isn't a memory leak
  }

  // If we're at 33% usage or less, shrink the vector
  if ((Entries_*3) <= Slots_)  // don't do while, because I think shrink will never goto 0
    shrinkVector();

  return(true);
}


// When adding into a position, the new node goes at the zero based slot
// specified by pos. All other nodes get moved one slot down.
template <class T>
int8_t ArrayList<T>::add(IN T &node,int32_t pos)
{
  if (pos > Entries_)      // You can only access one of the end of the vector
    pos=Entries_; 
  if (pos >= Slots_)        // If we're at the end, grow the list
    growVector();
  if (Entries_ >= Slots_)   // enuff space?
    growVector();

  // If we are insering into the middle or front of the list we have to
  //  slide the old objects forward.
  if (pos < Entries_)   // If there are elements after the add point
    memmove(Vector_+pos+1,Vector_+pos,sizeof(T)*(Entries_-pos)); // move them forward

  //fprintf(stderr,"Placement new to %p\n",(Vector_+pos));

  // This uses the placement new operator. placement new allows us to
  //  specify the memory address for the new object.  In this case we 
  //  want it at the 'pos' index into our array.
  new((void *)(Vector_+pos)) T((T &)node); // Trust me, this isn't a memory leak
  Entries_++;                         // one new entry
  return(TRUE);
}


// Add to the first node, all others get shifted down one slot
template <class T>
int8_t ArrayList<T>::addHead(IN T &node)  
{
  return(add(node,0));
}


// Append to the end of the list
template <class T>
int8_t ArrayList<T>::addTail(IN T &node)
{
  return(add(node,length()));
}  


// addSortedX only works (properly) if evrerything else in the list is added
// using addSorted.
template <class T>
int8_t ArrayList<T>::addSortedAsc(IN T &node)
{
	int32_t pos = _sortedLookup(node, 1);
	return(add(node, pos));
}


// addSortedX only works (properly) if evrerything else in the list is added
// using addSorted.
template <class T>
int8_t ArrayList<T>::addSortedDes(IN T &node)
{
	int32_t pos = _sortedLookup(node, 0);
	return(add(node, pos));
}


// This is the binary search used by addSorted
template <class T>
int32_t ArrayList<T>::_sortedLookup(IN T &target, int ascending)
{
	int	low, mid, high;
	T* 	lowtarget;
	T* 	hightarget;
	T* 	midtarget;


	// Trivial cases
	if( Entries_ == 0 )
		return 0;

	low = 0;
	high = Entries_ - 1;
	while( 1 )
	{
		assert( low <= high );
		mid = low + (int)(floor(((double)high - (double)low) / (double)2));

		getPointer(&lowtarget, low);
		getPointer(&hightarget, high);
		getPointer(&midtarget, mid);

		// Exact match
		if( *midtarget == target )  return mid;

		// Single element
		if( high == low )
		{
			if( ascending )
			{
				if( target <= *lowtarget )
					return low;
				else
					return low + 1;
			}
			else
			{
				if( target <= *lowtarget )
					return low + 1;
				else
					return low;
			}
		}

		// Two elemsnts
		if( (high - low) == 1 )
		{
			if( ascending )
			{
				if( target <= *lowtarget )
					return low;
				else if( target <= *hightarget )
					return high;
				else
					return high + 1;
			}
			else
			{
				if( target <= *hightarget )
					return high + 1;
				else if( target <= *lowtarget )
					return high;
				else
					return low;
			}
		}

		// Sorry, try again...
		if( ascending )
		{
			if( target < *midtarget )
				high = mid;
			else
				low = mid;
		}
		else
		{
			if( target < *midtarget )
				low = mid;
			else
				high = mid;
		}
	}
}


/*// addNumSortedX works in much the same way as addSortedX, except that I needed
// it for a very specific thing.  I needed a list of strings numerically sorted, 
// not alphabetically sorted.  Furthermore these strings were composed of numbers
// delimited by underscores.  In the interest of keeping it generic, these 
// functions take as args a node, a delimiting character, and a count of the
// number of fields to include in a sort.  If this is the list of strings:
//
//	55_100, 2_5, 23_32, 98_445, 2_48, 8_88, 2_3, 2_4
//
// An alphabetical sort is:
//	
//	2_3, 2_4, 2_48, 2_5, 55_100, 8_88, 98_445
//
// But a numerical sort by calling addNumSortedAsc(<whatever>, "_", 2) will result in:
//
//	2_3, 2_4, 2_5, 2_48, 8_88, 55_100, 98_445
//
// Yes...now that you mention it I am on crack...
//	
template <class T>
int8_t ArrayList<T>::addNumSortedAsc(IN T &node, char delim, int fields)
{
        int32_t pos = _numSortedLookup(node, delim, fields, 1);
        return(add(node, pos));
}


// See addNumSortedAsc comment above.
template <class T>
int8_t ArrayList<T>::addSortedDes(IN T &node, char delim, int fields)
{
        int32_t pos = _sortedLookup(node, delim, fields, 0);
        return(add(node, pos));
}


// This is the binary search used by addSorted
template <class T>
int32_t ArrayList<T>::_numSortedLookup(IN T &target, char delim, int fields, int ascending)
{
	int	low, mid, high;
	T* 	lowtarget;
	T* 	hightarget;
	T* 	midtarget;


	// Trivial case 
	if( Entries_ == 0 )
		return 0;
	
	low = 0;
	high = Entries_;
	while( 1 )
	{
		assert( low <= high );
		mid = low + (int)(floor(((double)high - (double)low) / (double)2));

		getPointer(&lowtarget, low);
		getPointer(&hightarget, high);
		getPointer(&midtarget, mid);

		// Exact match
		if( *midtarget == target )  return mid;

		// Single element
		if( high == low )
		{
			if( ascending )
			{
				if( target <= *lowtarget )
					return low;
				else
					return low + 1;
			}
			else
			{
				if( target <= *lowtarget )
					return low + 1;
				else
					return low;
			}
		}

		// Two elemsnts
		if( (high - low) == 1 )
		{
			if( ascending )
			{
				if( target <= *lowtarget )
					return low;
				else 
					return high;
			}
			else
			{
				if( target <= *lowtarget )
					return high;
				else 
					return low;
			}
		}

		// Sorry, try again...
		if( ascending )
		{
			if( target < *midtarget )
				high = mid;
			else
				low = mid;
		}
		else
		{
			if( target < *midtarget )
				low = mid;
			else
				high = mid;
		}
	}
}*/


//
// Delete an item at this index and construct a new one in it's place
//
template <class T>
int8_t ArrayList<T>::replace(IN T &node, int32_t pos)
{
  if (Entries_==0)
    return(FALSE);
  if (pos<0)
    pos=0;
  if (pos >= Entries_)
    pos=Entries_-1;
  
  (Vector_+pos)->~T();    // Call the destructor manually. Don't try this
                          //  at home kiddies!

  // Now put the replacement object in there...
  new((void *)(Vector_+pos)) T(node); // Trust me, this isn't a memory leak

  return(TRUE);
}



// Remove at the zero based index specified by 'pos'.  When removing from
// a slot, all others get shifted up by one.
template <class T>
int8_t ArrayList<T>::remove(int32_t pos) 
{
  if (Entries_==0)
    return(FALSE);
  if (pos<0)
    pos=0;
  if (pos >= Entries_)
    pos=Entries_-1;
  
  (Vector_+pos)->~T();    // Call the destructor manually. Don't try this
                          //  at home kiddies!

  memmove(Vector_+pos,Vector_+pos+1,sizeof(T)*(Entries_-pos-1));

  Entries_--;

  // If we're at 33% usage or less, shrink the vector
  if ( (Entries_*3) <= Slots_)
    shrinkVector();

  return(TRUE);
}


// Remove at the zero based index specified by 'pos'.  When removing from
// a slot, all others get shifted up by one.
template <class T>
int8_t ArrayList<T>::remove(OUT T &node, int32_t pos)
{
  int8_t retval;
  retval=get(node,pos);
  if (retval==FALSE)
    return(FALSE);
  return(remove(pos));
}


// Remove the first node of the list
template <class T>
int8_t ArrayList<T>::removeHead(OUT T &node)     
{
  return(remove(node,0));
}


// Remove the last node of the list
template <class T>
int8_t ArrayList<T>::removeTail(OUT T &node)
{
  return(remove(node,Entries_-1));
} 

// get a pointer to the internally managed object.  Try and avoid this, but
//  sometimes efficiency requires it...
// get a copy of an item
template <class T>
int8_t ArrayList<T>::getPointer(OUT T **node,int32_t pos) RO 
{
  if ((pos < 0)||(pos >= Entries_))
    return(FALSE);
  *node=&(Vector_[pos]);
  return(TRUE);  
}
  

// get a copy of an item
template <class T>
int8_t ArrayList<T>::get(OUT T &node,int32_t pos) RO 
{
  if ((pos < 0)||(pos >= Entries_))
    return(FALSE);
  node=Vector_[pos];
  return(TRUE);  
}


// get a copy of the first node of the list
template <class T>
int8_t ArrayList<T>::getHead(OUT T &node) RO 
{
  return(get(node,0));
}


// get a copy of the last node
template <class T>
int8_t ArrayList<T>::getTail(OUT T &node) RO 
{
  return(get(node,Entries_-1));
}      

// just for debugging
template <class T>
void ArrayList<T>::print(FILE *out)
{
  fprintf(out,"--------------------\n");
  //for (int i=0; i<Entries_; i++)
  //  Vector_[i].print();
  fprintf(out,"Entries: %d  Slots:  %d  sizeof(T): %d\n",Entries_,Slots_,
    sizeof(T));
  fprintf(out,"--------------------\n");
}            

// Return the current length of the list
template <class T>
int32_t ArrayList<T>::length(void) RO 
{
  return(Entries_);
}    

// Grow the vector by a factor of 2X 
template <class T>
int8_t ArrayList<T>::growVector(void)
{
  if (Entries_ < Slots_)   // Don't grow until we're at 100% usage
    return(FALSE);

  int   newSlots=Entries_*2;
  if(newSlots < INITIAL_SIZE)
    newSlots=INITIAL_SIZE;

  //fprintf(stderr,"Growing vector to: %d\n",newSlots);

  // The goofy looking new below prevents operator new from getting called
  //  unnecessarily.  This is severall times faster than allocating all of
  //  the slots as objects and then calling the assignment operator on them
  //  when they actually get used.
  //
  T *newVector=(T *)(new uint8_t[newSlots * sizeof(T)]);
  memset(newVector,0,newSlots * sizeof(T)); // zero just to be safe

  if (Vector_ != NULL)
    memcpy(newVector,Vector_,Entries_*sizeof(T));

  delete[]((uint8_t *)Vector_);  // Get rid of the old vector without calling
                               //  destructors

  Vector_=newVector;
  Slots_=newSlots;

  return(TRUE); 
}


// Shrink the vector by a factor of 2X
template <class T>
int8_t ArrayList<T>::shrinkVector(void)
{
  //fprintf(stderr,"Shrink called\n");

  // Don't need to shrink until usage goes below 33%
  if ( (Entries_*3) > Slots_)
    return(FALSE);

  int   newSlots=Slots_/2;
  if(newSlots < INITIAL_SIZE) // never shrink past initial size
    newSlots=INITIAL_SIZE;

  if (newSlots >= Slots_)   // don't need to shrink
    return(FALSE);

  //fprintf(stderr,"Shrinking vector to: %d\n",newSlots);
 
  // The goofy looking new below prevents operator new from getting called
  //  unnecessarily.  This is severall times faster than allocating all of
  //  the slots as objects and then calling the assignment operator on them
  //  when they actually get used.
  //
  T *newVector=(T *)(new uint8_t[newSlots * sizeof(T)]);
 
  if (Vector_ != NULL)    // Vector_ better not be NULL!
    memcpy(newVector,Vector_,Entries_*sizeof(T));

  delete[]((uint8_t *)Vector_);  // Get rid of the old vector without calling
                               //  destructors
 
  Vector_=newVector;
  Slots_=newSlots;
 
  return(TRUE);
}


// Implementation was missing from source code. This function is based on the build
// artifact DLL in the archive and similar functions in this header file.
// Compiles, but not tested. LFeenanEA - January 27th 2025
template <class T>
int32_t ArrayList<T>::removeMany(OUT T *outarray, int32_t pos, int32_t howmany)
{
  if (Entries_==0)
    return(FALSE);
  if (pos<0)
    pos=0;
  if (pos>=Entries_)
    pos=Entries_-1;
  if (pos+howmany>Entries_)
    assert(0);

  for (int i=0; i<howmany; i++)
  {
    if (outarray)
      outarray[i]=Vector_[pos+i];
  }

  memmove(Vector_+pos,Vector_+pos+howmany,sizeof(T)*(Entries_-pos-howmany));

  Entries_-=howmany;

  // If we're at 33% usage or less, shrink the vector
  if ((Entries_*3) <= Slots_)  // don't do while, because I think shrink will never goto 0
    shrinkVector();

  return(howmany);
}


// Implementation was missing from source code. This function is based on the build
// artifact DLL in the archive and similar functions in this header file.
// Compiles, but not tested. LFeenanEA - January 27th 2025
template <class T>
int8_t ArrayList<T>::addMany(IN T *inarray, int32_t pos, int32_t howmany)
{
  if (pos > Entries_)      // You can only access one of the end of the vector
    pos=Entries_;

  // Grow the vector as much as we need to
  while (howmany+Entries_ >= Slots_)
    growVector();

  // If we are insering into the middle or front of the list we have to
  //  slide the old objects forward.
  if (pos < Entries_)   // If there are elements after the add point
    memmove(Vector_+pos+howmany,Vector_+pos,sizeof(T)*(Entries_-pos)); // move them forward

  // This uses the placement new operator. placement new allows us to
  //  specify the memory address for the new object.  In this case we 
  //  want it at the 'pos' index into our array.
  for (int i=0; i<howmany; i++)
  {
    new((void *)(Vector_+i)) T(inarray[i]); // Trust me, this isn't a memory leak
  }

  Entries_+=howmany;
  return(TRUE);
}


#endif
