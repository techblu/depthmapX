// Paf Template Library --- a set of useful C++ templates
//
// Copyright (c) 1996-2011 Alasdair Turner (a.turner@ucl.ac.uk)
//
//-----------------------------------------------------------------------------
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See the lgpl.txt file for details
//-----------------------------------------------------------------------------
//
// Paf's cross platform box of tricks
// Everything you need to write any C++.  All in one file!
//
// pstring     similar to STL string
// pmemvec     base clase for pvector and prefvec
// pvector     similar to STL vector
// prefvec     pvector with a different allocator (vector of references)
// pqvector    searchable prefvec
// pqmap       a simple map class, based on a binary tree
// plist       a simple list class
// ptree       a simple tree template
// pflipper    used for flipping between two vectors (or anythings...)
// pexception  exception class, base for various exception types
//
//
// A3 eliminates the double referencing used previously in the
// vector classes

#ifndef __PAFTL_H__
#define __PAFTL_H__

#define PAFTL_DATE "01-FEB-2011"
// 31-jan-2011: unicode constructor for pstring
// 04-aug-2010: fix bug on quicksort to avoid sorting zero length array
// 06-jun-2010: rewrite quicksort to avoid infinite loop
// 31-aug-2009: change pstring constructors to align with STL string
// 28-nov-2007: full implementation for ANSI standards
// 28-nov-2007: minor bug on pvecsub construction: should be 2 << count rather than 1 << count
// 30-aug-2007: make compatible with Unix / MacOS

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <fstream>

#ifdef _WIN32
// Quick mod - TV
#pragma warning (disable: 4996 )
#pragma warning (disable: 4396 )
#else

#endif

#ifdef _MSC_VER // MSVC compiler
   typedef signed __int64 int64;
   typedef unsigned __int64 uint64;
#else
   #include <stdint.h> // not guaranteed to exist, but does in Mac / Ubuntu
   typedef int64_t int64;
   typedef uint64_t uint64;
#endif

using namespace std;

#ifndef bool
//   #define bool int
#endif
#ifndef true
   #define true 1
#endif
#ifndef false
   #define false 0
#endif

///////////////////////////////////////////////////////////////////////////////

// namespace paftl {

class pexception;
class pstring;
template <class T> class pvector;
template <class T> class prefvec;
template <class T> class pqvector;
template <class T1, class T2> class pqmap;
template <class T> class plist;
template <class T> class ptree;
template <class T> class pflipper;

// a few basic types

typedef pvector<int>       pvecint;
typedef pvector<float>     pvecfloat;
typedef pvector<double>    pvecdouble;
typedef pqvector<pstring>  pvecstring;

///////////////////////////////////////////////////////////////////////////////

// miscellaneous enums (paftl used as namespace)

namespace paftl
{
   enum add_t {ADD_UNIQUE, ADD_REPLACE, ADD_DUPLICATE, ADD_HERE};
   const size_t npos = size_t(-1);
}

///////////////////////////////////////////////////////////////////////////////

class pexception
{
public:
   enum exception_t { UNDEFINED           = 0x0000,
                      MEMORY_ALLOCATION   = 0x0001,
                      FILE_ERROR          = 0x0002,
                      MAX_ARRAY_EXCEEDED  = 0x0003};

protected:
   int m_exception;
   size_t m_data;
public:
   pexception(int n_exception = UNDEFINED, size_t data = 0)
      { m_exception = n_exception; m_data = data; }
   int error_code()
      { return m_exception; }
   size_t info()
      { return m_data; }
};


///////////////////////////////////////////////////////////////////////////////

// pmemvec: base allocation for pvector and prefvec

template <class T> class pmemvec
{
public:
   class exception : public pexception
   {
   public:
      enum exception_t { PVECTOR_UNDEFINED      = 0x1000,
                         EMPTY_VECTOR           = 0x1001,
                         UNASSIGNED_ITERATOR    = 0x1002,
                         OUT_OF_RANGE           = 0x1003};
   public:
      exception(int n_exception = PVECTOR_UNDEFINED, size_t data = 0) : pexception( n_exception, data ) {}
   };
protected:
   T *m_data;
   unsigned short m_shift;
   size_t m_length;
public:
   // redefine
   pmemvec(size_t sz = 0);
   pmemvec(const pmemvec<T>& );
   virtual ~pmemvec();
   pmemvec<T>& operator = (const pmemvec<T>& );
   //
   virtual void push_back(const T& item);
   virtual void pop_back();
   virtual void remove_at(size_t pos = 0);
   virtual void remove_at(const pvecint& list);
   virtual void insert_at(size_t pos, const T& item);
   //
   virtual void set(size_t count);
   virtual void set(const T& item, size_t count);
   //
   virtual void clear();
   virtual void clearnofree();
protected:
   size_t storage_size() const
      { return m_shift ? (2 << m_shift) : 0; }
   void grow(size_t pos);
   void shrink();
public:
   size_t size() const
      { return m_length; }
   T& base_at(size_t pos)
      { return m_data[pos]; }
   const T& base_at(size_t pos) const
      { return m_data[pos]; }
public:
   istream& read( istream& stream, streampos offset = streampos(-1) );
   ostream& write( ostream& stream );
};

template <class T>
pmemvec<T>::pmemvec(size_t sz)
{
   // note: uses same as grow / storage_size, but cannot rely on function existence when calling constructor
   if (sz == 0) {
      m_data = NULL;
      m_shift = 0;
   }
   else {
      do {
         m_shift++;
      } while ((size_t(2) << m_shift) < sz);
      m_data = new T [storage_size()];
      if (m_data == NULL) {
         throw pexception(pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size());
      }
   }
   m_length = 0;
}

template <class T>
pmemvec<T>::pmemvec(const pmemvec& v)
{
   m_shift  = v.m_shift;
   m_length = v.m_length;

   if (m_shift) {
      m_data = new T [storage_size()];
      if (m_data == NULL)
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );

      if (m_length) {
         for (size_t i = 0; i < m_length; i++)
            m_data[i] = v.m_data[i];
      }
   }
   else {
      m_data = NULL;
   }
}

template <class T>
pmemvec<T>::~pmemvec()
{
   if (m_data)
   {
      delete [] m_data;
      m_data = NULL;
   }
}

template <class T>
pmemvec<T>& pmemvec<T>::operator = (const pmemvec<T>& v)
{
   if (m_shift < v.m_shift)
   {
      if (m_shift != 0) {
         delete [] m_data;
      }
      m_shift = v.m_shift;
      if (m_shift) {
         m_data = new T [storage_size()];
         if (!m_data)
            throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );
      }
      else {
         m_data = NULL;
      }
   }
   m_length = v.m_length;
   if (m_length) {
      for (size_t i = 0; i < m_length; i++)
         m_data[i] = v.m_data[i];
   }
   return *this;
}

template <class T>
void pmemvec<T>::push_back(const T& item)
{
   if (m_length >= storage_size()) {
      grow( m_length );
   }
   m_data[m_length] = item;
   m_length++;
}

template <class T>
void pmemvec<T>::pop_back()
{
   if (m_length == 0)
      throw exception( exception::EMPTY_VECTOR );

   --m_length;

   // Preferably include shrink code here
}

template <class T>
void pmemvec<T>::insert_at(size_t pos, const T& item)
{
   if (pos == paftl::npos || pos > m_length) {
      throw exception( exception::OUT_OF_RANGE );
   }
   if (m_length >= storage_size()) {
      grow( pos );
   }
   else {
      for (size_t i = m_length; i > pos; i--) {
         m_data[i] = m_data[i - 1];
      }
   }
   m_data[pos] = item;
   m_length++;
}

template <class T>
void pmemvec<T>::remove_at(size_t pos)
{
   // This is a simple but reliable remove item from vector
   if (m_length == 0)
      throw exception( exception::EMPTY_VECTOR );
   else if (pos == paftl::npos || pos >= m_length)
      throw exception( exception::OUT_OF_RANGE );

   for (size_t i = pos; i < m_length - 1; i++) {
      m_data[i] = m_data[i + 1];
   }
   --m_length;

   // Preferably include shrink code here
}

template <class T>
void pmemvec<T>::set(size_t count)
{
   clear();
   do {
      m_shift++;
   } while ((size_t(2) << m_shift) < count);
   m_data = new T [storage_size()];
   if (!m_data)
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );
   m_length = count;
}

template <class T>
void pmemvec<T>::set(const T& item, size_t count)
{
   clear();
   do {
      m_shift++;
   } while ((size_t(2) << m_shift) < count);
   m_data = new T [storage_size()];
   if (!m_data)
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );
   m_length = count;
   for (size_t i = 0; i < m_length; i++) {
      m_data[i] = item;
   }
}

template <class T>
void pmemvec<T>::clear()
{
   m_length = 0;
   m_shift = 0;
   if (m_data)
   {
      delete [] m_data;
      m_data = NULL;
   }
}

template <class T>
void pmemvec<T>::clearnofree()
{
   m_length = 0;
}

template <class T>
void pmemvec<T>::grow(size_t pos)
{
   m_shift++;

   T *new_data = new T [storage_size()];
   if (!new_data)
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );

   if (m_length) {
      for (size_t i = 0; i < m_length + 1; i++)
         new_data[i] = (i < pos) ? m_data[i] : m_data[i-1];
   }
   if (m_data) {
      delete [] m_data;
   }
   m_data = new_data;
}

template <class T>
void pmemvec<T>::shrink()
{
}

template <class T>
istream& pmemvec<T>::read( istream& stream, streampos offset )
{
   if (offset != streampos(-1)) {
      stream.seekg( offset );
   }
   // READ / WRITE USES 32-bit LENGTHS (number of elements)
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int length;
   stream.read( (char *) &length, sizeof(unsigned int) );
   m_length = size_t(length);
   if (m_length >= storage_size()) {
      if (m_data) {
         delete [] m_data;
         m_data = NULL;
      }
      while (m_length >= storage_size())
         m_shift++;
      m_data = new T [storage_size()];
      if (!m_data)
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );
   }
   if (m_length != 0) {
      stream.read( (char *) m_data, sizeof(T) * streamsize(m_length) );
   }
   return stream;
}

template <class T>
ostream& pmemvec<T>::write( ostream& stream )
{
   // READ / WRITE USES 32-bit LENGTHS (number of elements)
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems

   // check for max unsigned int exceeded
   if (m_length > size_t((unsigned int)-1)) {
      throw pexception( pexception::MAX_ARRAY_EXCEEDED, m_length );
   }
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int length = (unsigned int)(m_length);
   stream.write( (char *) &length, sizeof(unsigned int) );
   if (m_length != 0) {
      stream.write( (char *) m_data, sizeof(T) * streamsize(m_length) );
   }
   return stream;
}

///////////////////////////////////////////////////////////////////////////////

template <class T> class pvector : public pmemvec<T>
{
protected:
   mutable size_t m_current;
public:
   pvector(size_t sz = 0) : pmemvec<T>(sz)
      {m_current = paftl::npos;}
   pvector(const pvector<T>& v) : pmemvec<T>(v)
      {m_current = v.m_current;}
   pvector<T>& operator = (const pvector<T>& );
   virtual ~pvector()
      {;}
   //
   T& at(size_t pos)
      { return pmemvec<T>::m_data[pos]; }
   const T& at(size_t pos) const
      { return pmemvec<T>::m_data[pos]; }
   T& operator[](size_t pos)
      { return at(pos); }
   const T& operator[](size_t pos) const
      { return at(pos); }
   //
   T& head()
      { return at(0); }
   const T& head() const
      { return at(0); }
   T& tail()
      { return at(pmemvec<T>::m_length-1); }
   const T& tail() const
      { return at(pmemvec<T>::m_length-1); }
   // standard operations (unordered vector)
   T& find(const T& item);
   const T& find(const T& item) const;
   size_t findindex(const T& item) const;
   // binary operations (ordered vector)
   size_t add(const T& item, int type = paftl::ADD_UNIQUE); // ignored if already exists
   T& search(const T& item);
   const T& search(const T& item) const;
   size_t searchindex(const T& item) const;
   size_t searchfloorindex(const T& item) const;
   size_t searchceilindex(const T& item) const;
   void remove(const T& item)
   { pmemvec<T>::remove_at(searchindex(item)); }
   // set operations (ordered vector)
   void operator += (const pvector<T>& v);
   // qsort algo:
   void sort();
   void sort(size_t left, size_t right);
   //
   
   // Quick mod - TV
#if defined(_WIN32)   
   friend pvector<T> intersect(const pvector<T>& a, const pvector<T>& b);
#endif   
};

template <class T>
pvector<T>& pvector<T>::operator = (const pvector<T>& v)
{
   if (&v != this)
   {
      pmemvec<T>::operator = (v);
   }
   return *this;
}

template <class T>
T& pvector<T>::find(const T& item)
{
   if (findindex(item) == paftl::npos) {
      throw pmemvec<T>::exception::exception(pmemvec<T>::exception::OUT_OF_RANGE);
   }
   return at(m_current);
}
template <class T>
const T& pvector<T>::find(const T& item) const
{
   if (findindex(item) == paftl::npos) {
      throw pmemvec<T>::exception(pmemvec<T>::exception::OUT_OF_RANGE);
   }
   return at(m_current);
}

template <class T>
size_t pvector<T>::findindex(const T& item) const
{
   for (size_t i = 0; i < pmemvec<T>::m_length; i++) {
      if (at(i) == item) {
         m_current = i;
         return i;
      }
   }
   return paftl::npos;
}

// oops... we need an iterator... add use a current position marker.

template <class T>
T& pvector<T>::search(const T& item)
{
   if (searchindex(item) == paftl::npos) {
      throw pmemvec<T>::exception(pmemvec<T>::exception::OUT_OF_RANGE); // Not found
   }
   return at(m_current);
}
template <class T>
const T& pvector<T>::search(const T& item) const
{
   if (searchindex(item) == paftl::npos) {
      throw pmemvec<T>::exception(pmemvec<T>::exception::OUT_OF_RANGE); // Not found
   }
   return at(m_current);
}

template <class T>
size_t pvector<T>::searchindex(const T& item) const
{
   if (pmemvec<T>::m_length != 0) {
      size_t ihere, ifloor = 0, itop = pmemvec<T>::m_length - 1;
      while (itop != paftl::npos && ifloor <= itop) {
         m_current = ihere = (ifloor + itop) / 2;
         if (item == at(ihere)) {
            return m_current;
         }
         else if (item > at(ihere)) {
            ifloor = ihere + 1;
         }
         else {
            itop = ihere - 1;
         }
      }
   }
   return paftl::npos;
}

template <class T>
size_t pvector<T>::searchfloorindex(const T& item) const
{
   searchindex(item);
   while (m_current != 0 && at(m_current) > item) {
      m_current--;
   }
   return m_current;
}

template <class T>
size_t pvector<T>::searchceilindex(const T& item) const
{
   searchindex(item);
   while (m_current < pmemvec<T>::m_length && at(m_current) < item) {
      m_current++;
   }
   return m_current;
}


// Note: uses m_current set by searchindex

// Really need a list 'merge' function as well... will write this soon!

template <class T>
size_t pvector<T>::add(const T& item, int type) // UNIQUE by default
{
   size_t where = paftl::npos;
   if (pmemvec<T>::m_length == 0 || item > pvector<T>::tail()) { // often used for push_back, so handle quickly if so
      pmemvec<T>::push_back( item );
      where = pmemvec<T>::m_length - 1;
   }
   else {
      // if you call with ADD_HERE, it is assumed you've just used search or searchindex
      // i.e., we don't need to go through the binary search again to find the insert position
      if (type != paftl::ADD_HERE) {
         searchindex(item);
      }
      if (item < at(m_current)) {
         pmemvec<T>::insert_at( m_current, item );
         where = m_current;
      }
      else if (item > at(m_current) || type == paftl::ADD_DUPLICATE) {
         pmemvec<T>::insert_at( m_current + 1, item );
         where = m_current + 1;
      }
      else if (type == paftl::ADD_REPLACE || type == paftl::ADD_HERE) {
         // relies on good assignment operator
         at(m_current) = item;
      }
      // n.b., type "UNIQUE" does not replace, returns -1
   }
   return where;
}

template <class T>
void pvector<T>::operator += (const pvector<T>& v)
{
   if (this != &v && pmemvec<T>::m_length + v.pmemvec<T>::m_length > 0) {

      while (pmemvec<T>::m_length + v.pmemvec<T>::m_length >= pmemvec<T>::storage_size())
         pmemvec<T>::m_shift++;

      T *new_data = new T [pmemvec<T>::storage_size()];
      if (!new_data)
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * pmemvec<T>::storage_size() );

      size_t i = 0, j = 0, k = 0;
      while (i + j < pmemvec<T>::m_length + v.pmemvec<T>::m_length) {
         if ( i < pmemvec<T>::m_length ) {
            if (j < v.pmemvec<T>::m_length) {
               if (pmemvec<T>::m_data[i] < v.pmemvec<T>::m_data[j]) {
                  new_data[k++] = pmemvec<T>::m_data[i++];
               }
               else if (pmemvec<T>::the_data()[i] > v.pmemvec<T>::the_data()[j]) {
                  new_data[k++] = v.pmemvec<T>::m_data[j++];
               }
               else {
                  new_data[k++] = pmemvec<T>::m_data[i++]; j++;
               }
            }
            else {
               while (i < pmemvec<T>::m_length) {
                  new_data[k++] = pmemvec<T>::m_data[i++];
               }
            }
         }
         else {
            while (j < v.pmemvec<T>::m_length) {
               new_data[k++] = v.pmemvec<T>::m_data[j++];
            }
         }
      }
      if (pmemvec<T>::m_data) {
         delete [] pmemvec<T>::m_data;
      }
      pmemvec<T>::m_length = k;
      pmemvec<T>::m_data = new_data;
   }
}

template <class T>
void pvector<T>::sort()
{
   if (pmemvec<T>::m_length != 0) {
      sort(0,pmemvec<T>::m_length-1);
   }
}

// rewrite 6-jun-10 (was entering infinite loop, now appears to work properly)
template <class T>
void pvector<T>::sort(size_t left, size_t right)
{
   size_t i = left, j = right;
   const T& val = pmemvec<T>::m_data[(left+right)/2];
   while (j != paftl::npos && i <= j) {
      while (i <= j && pmemvec<T>::m_data[i] < val)
         i++;
      while (j != paftl::npos && pmemvec<T>::m_data[j] > val)
         j--;
      if (j != paftl::npos && i <= j) {
         // swap contents
         T temp = pmemvec<T>::m_data[i];
         pmemvec<T>::m_data[i] = pmemvec<T>::m_data[j];
         pmemvec<T>::m_data[j] = temp;
         i++; j--;
      }
   }
   if (j != paftl::npos && left < j)
      sort(left, j);
   if (i < right)
      sort(i, right);
}

// requires two sorted lists
template <class T>
inline pvector<T> intersect(const pvector<T>& a, const pvector<T>& b)
{
   pvector<T> retvec;
   size_t i = 0;
   size_t j = 0;
   while (i < a.size() && j < b.size()) {
      if (a[i] == b[j]) {
         retvec.push_back(a[i]);
         i++; j++;
      }
      else {
         while (a[i] < b[j] && i < a.size()) {
            i++;
         }
         while (a[i] > b[j] && j < b.size()) {
            j++;
         }
      }
   }
   return retvec;
}

///////////////////////////////////////////////////////////////////////////////

// this version for bulk deletes: copies over entries to new list
// (first requires pvector to have been defined)

template <class T>
void pmemvec<T>::remove_at(const pvecint& list)
{
   if (m_length == 0)
      throw exception( exception::EMPTY_VECTOR );
   if (list.size() >= m_length) {
      // if the list does not contain duplicates, then this simply means delete all contents:
      clear();
      return;
   }
   // shrink new vector:
   while ((m_length - list.size()) * 2 < storage_size()) {
      m_shift--;
   }
   size_t new_length = 0;
   T *new_data = new T [storage_size()];
   bool *rem_flag = new bool [m_length];
   if (!new_data)
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * storage_size() );
   if (!rem_flag)
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(bool) * storage_size() );
   size_t i;
   for (i = 0; i < m_length; i++) {
      rem_flag[i] = false;
   }
   for (i = 0; i < list.size(); i++) {
      if (size_t(list[i]) == paftl::npos || size_t(list[i]) >= m_length)
         throw exception( exception::OUT_OF_RANGE );
      rem_flag[list[i]] = true;
   }
   for (i = 0; i < m_length; i++) {
      if (!rem_flag[i]) {
         new_data[new_length] = m_data[i];
         new_length++;
      }
   }
   m_length = new_length;
   delete [] m_data;
   delete [] rem_flag;
   m_data = new_data;
}

///////////////////////////////////////////////////////////////////////////////

// prefvec: a vector of references (useful for larger objects)
// should be able to use pqvector in most cases

template <class T> class prefvec : public pmemvec<T *>
{
public:
   prefvec(size_t sz = 0) : pmemvec<T *>(sz)
      {;}
   prefvec(const prefvec<T>& );
   virtual ~prefvec();
   prefvec<T>& operator = (const prefvec<T>& );
   //
   void push_back(const T& item);
   void pop_back();
   void remove_at(size_t pos = 0);
   void free_at(size_t pos = 0);
   void remove_at(const pvecint& list);
   void insert_at(size_t pos, const T& item);
   //
   void set(size_t count);
   void set(const T& item, size_t count);
   //
   void clear();
   void clearnofree();
   //
   T& at(size_t pos)
      { return  *(pmemvec<T *>::m_data[pos]); }
   const T& at(size_t pos) const
      { return *(pmemvec<T *>::m_data[pos]); }
   T& operator[](size_t pos)
      { return at(pos); }
   const T& operator[](size_t pos) const
      { return at(pos); }
   //
   T& head()
      { return at(0); }
   const T& head() const
      { return at(0); }
   T& tail()
      { return at(pmemvec<T *>::m_length-1); }
   const T& tail() const
      { return at(pmemvec<T *>::m_length-1); }
   //
   // NOTE: no find (as often equivalence operator will not be defined)
   //
   // Override read and write
   istream& read( istream& stream );
   ostream& write( ostream& stream );
};

template <class T>
prefvec<T>::prefvec(const prefvec& v) : pmemvec<T *>(v)
{
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      pmemvec<T *>::m_data[i] = new T(v.at(i));
      if (pmemvec<T *>::m_data[i] == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
      }
   }
}

template <class T>
prefvec<T>& prefvec<T>::operator = (const prefvec<T>& v)
{
   if (&v != this) {
      for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
         delete pmemvec<T *>::m_data[i];
      }
      pmemvec<T *>::operator = (v);
      for (size_t j = 0; j < pmemvec<T *>::m_length; j++) {
         pmemvec<T *>::m_data[j] = new T(v.at(j));
         if (pmemvec<T *>::m_data[j] == NULL) {
            throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
         }
      }
   }
   return *this;
}

template <class T>
prefvec<T>::~prefvec()
{
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      if (pmemvec<T *>::m_data[i])
         delete pmemvec<T *>::m_data[i];
   }
   // virtual destructor called for pmemvec
}

template <class T>
void prefvec<T>::push_back(const T& item)
{
   T *p = new T(item);
   pmemvec<T *>::push_back( p );
}

template <class T>
void prefvec<T>::pop_back()
{
   if (pmemvec<T *>::m_data[pmemvec<T *>::m_length - 1]) {
      delete pmemvec<T *>::m_data[pmemvec<T *>::m_length - 1];
      pmemvec<T *>::m_data[pmemvec<T *>::m_length - 1] = NULL;
   }
   pmemvec<T *>::pop_back();
}

template <class T>
void prefvec<T>::remove_at(size_t pos)
{
   if (pmemvec<T *>::m_length == 0)
      throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::EMPTY_VECTOR );
   else if (pos == paftl::npos || pos >= pmemvec<T *>::m_length)
      throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::OUT_OF_RANGE );

   if (pmemvec<T *>::m_data[pos]) {
      delete pmemvec<T *>::m_data[pos];
      pmemvec<T *>::m_data[pos] = NULL;
   }
   pmemvec<T *>::remove_at( pos );
}

// just frees the memory at position: does not manipulate vector

template <class T>
void prefvec<T>::free_at(size_t pos)
{
   if (pmemvec<T *>::m_length == 0)
      throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::EMPTY_VECTOR );
   else if (pos == paftl::npos || pos >= pmemvec<T *>::m_length)
      throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::OUT_OF_RANGE );

   delete pmemvec<T *>::m_data[pos];
   pmemvec<T *>::m_data[pos] = NULL;
}

// this version for intended for bulk deletes (also retains previous ordering)
template <class T>
void prefvec<T>::remove_at(const pvecint& list)
{
   if (pmemvec<T *>::m_length == 0)
      throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::EMPTY_VECTOR );

   for (size_t i = 0; i < list.size(); i++) {
      if (size_t(list[i]) == paftl::npos || size_t(list[i]) >= pmemvec<T *>::m_length)
         throw (typename pmemvec<T *>::exception)( pmemvec<T *>::exception::OUT_OF_RANGE );
      if (pmemvec<T *>::m_data[list[i]]) {
         delete pmemvec<T *>::m_data[list[i]];
         pmemvec<T *>::m_data[list[i]] = NULL;
      }
   }
   pmemvec<T *>::remove_at( list );
}


template <class T>
void prefvec<T>::insert_at(size_t pos, const T& item)
{
   T *p = new T(item);
   if (p == NULL) {
      throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
   }
   pmemvec<T *>::insert_at(pos, p);
}

template <class T>
void prefvec<T>::set(size_t count)
{
   pmemvec<T *>::set(count);
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      pmemvec<T *>::m_data[i] = NULL;
      if (pmemvec<T *>::m_data[i] == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
      }
   }
}

template <class T>
void prefvec<T>::set(const T& item, size_t count)
{
   pmemvec<T *>::set(count);
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      pmemvec<T *>::m_data[i] = new T(item);
      if (pmemvec<T *>::m_data[i] == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
      }
   }
}


template <class T>
void prefvec<T>::clear()
{
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      if (pmemvec<T *>::m_data[i]) {
         delete pmemvec<T *>::m_data[i];
         pmemvec<T *>::m_data[i] = NULL;
      }
   }
   pmemvec<T *>::clear();
}

template <class T>
void prefvec<T>::clearnofree()
{
   // still have to delete objects pointed to, just doesn't just clear the list of pointers:
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      if (pmemvec<T *>::m_data[i]) {
         delete pmemvec<T *>::m_data[i];
         pmemvec<T *>::m_data[i] = NULL;
      }
   }
   pmemvec<T *>::clearnofree();
}

// Note: read and write only work for structures without pointers

template <class T>
istream& prefvec<T>::read( istream& stream )
{
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      if (pmemvec<T *>::m_data[i]) {
         delete pmemvec<T *>::m_data[i];
         pmemvec<T *>::m_data[i] = NULL;
      }
   }
   // READ / WRITE USES 32-bit LENGTHS (number of elements)
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int length;
   stream.read( (char *) &length, sizeof(unsigned int) );
   if (stream.fail()) {
      throw pexception(pexception::FILE_ERROR);
   }
   pmemvec<T *>::m_length = size_t(length);
   if (pmemvec<T *>::m_length >= pmemvec<T *>::storage_size()) {
      if (pmemvec<T *>::m_data) {
         delete [] pmemvec<T *>::m_data;
      }
      while (pmemvec<T *>::m_length >= pmemvec<T *>::storage_size())
         pmemvec<T *>::m_shift++;
      pmemvec<T *>::m_data = new T * [pmemvec<T *>::storage_size()];
      if (pmemvec<T *>::m_data == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, pmemvec<T *>::storage_size() * sizeof(T) );
      }
   }
   for (size_t j = 0; j < pmemvec<T *>::m_length; j++) {
      T *p = new T;
      if (p == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) );
      }
      stream.read( (char *) p, sizeof(T) );
      if (stream.fail()) {
         throw pexception(pexception::FILE_ERROR);
      }
      pmemvec<T *>::m_data[j] = p;
   }
   return stream;
}

template <class T>
ostream& prefvec<T>::write( ostream& stream )
{
   // READ / WRITE USES 32-bit LENGTHS (number of elements)
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems

   // check for max unsigned int exceeded
   if (pmemvec<T *>::m_length > size_t((unsigned int)-1)) {
      // Quick mod - TV
#if 0
      throw exception( pexception::MAX_ARRAY_EXCEEDED, pmemvec<T *>::m_length );
#else
        ;
#endif
   }

   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int length = (unsigned int)(pmemvec<T *>::m_length);
   stream.write( (char *) &length, sizeof(unsigned int) );

   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      stream.write( (char *) &at(i), sizeof(T) );
   }
   return stream;
}

///////////////////////////////////////////////////////////////////////////////

// pqvector... prefvec with the binary addition routine...
// (i.e., almost the hash table...)

// (so... I think we might replace pqmap with an inherited form of this soon)
// (if MS would oblige...)

template <class T> class pqvector : public prefvec<T>
{
protected:
   mutable size_t m_current;
public:
   pqvector(size_t sz = 0) : prefvec<T>(sz) {;}
   pqvector(const pqvector<T>& v) : prefvec<T>( v ) {;}
   virtual ~pqvector() {;}
   pqvector<T>& operator = (const pqvector<T>& v)
   { prefvec<T>::operator = (v); return *this; }
   //
   // at, [] and so on as before
   //
   // standard operations (unordered vector)
   T& find(const T& item);
   const T& find(const T& item) const;
   size_t findindex(const T& item) const;
   //
   // binary operations (ordered vector)
   T& search(const T& item);
   const T& search(const T& item) const;
   size_t searchindex(const T& item) const;
   void remove(const T& item)
   { remove_at(searchindex(item)); }
   size_t add(const T& item, int type = paftl::ADD_UNIQUE);
   T& current()
   { return prefvec<T>::at(m_current); }
   const T& current() const
   { return pmemvec<T *>::at(m_current); }
   // qsort algo:
   void sort();
   void sort(size_t left, size_t right);
};

template <class T>
T& pqvector<T>::find(const T& item)
{
   if (findindex(item) == paftl::npos) {
      throw pmemvec<T *>::exception(pmemvec<T *>::exception::OUT_OF_RANGE);
   }
   return prefvec<T>::at(m_current);
}
template <class T>
const T& pqvector<T>::find(const T& item) const
{
   if (findindex(item) == paftl::npos) {
      throw pmemvec<T *>::exception(pmemvec<T *>::exception::OUT_OF_RANGE);
   }
   return prefvec<T>::at(m_current);
}

template <class T>
size_t pqvector<T>::findindex(const T& item) const
{
   for (size_t i = 0; i < pmemvec<T *>::m_length; i++) {
      if (prefvec<T>::at(i) == item) {
         m_current = i;
         return i;
      }
   }
   return paftl::npos;
}

// oops... we need an iterator... add use a current position marker.

template <class T>
T& pqvector<T>::search(const T& item)
{
   if (searchindex(item) == paftl::npos) {
      throw (typename pmemvec<T *>::exception)(pmemvec<T *>::exception::OUT_OF_RANGE); // Not found
   }
   return prefvec<T>::at(m_current);
}
template <class T>
const T& pqvector<T>::search(const T& item) const
{
   if (searchindex(item) == paftl::npos) {
      throw (typename pmemvec<T *>::exception)(pmemvec<T *>::exception::OUT_OF_RANGE); // Not found
   }
   return prefvec<T>::at(m_current);
}

template <class T>
size_t pqvector<T>::searchindex(const T& item) const
{
  if (pmemvec<T *>::size() != 0) {
      size_t ihere, ifloor = 0, itop = pmemvec<T *>::size() - 1;
      while (itop != paftl::npos && ifloor <= itop) {
         m_current = ihere = (ifloor + itop) / 2;
         if (item == prefvec<T>::at(ihere)) {
            return m_current;
         }
         else if (item > prefvec<T>::at(ihere)) {
            ifloor = ihere + 1;
         }
         else {
            itop = ihere - 1;
         }
      }
   }
   return paftl::npos;
}

// Note: uses m_current set by searchindex

// Really need a list 'merge' function as well... will write this soon!

template <class T>
size_t pqvector<T>::add(const T& item, int type) // default type UNIQUE
{
   size_t where = paftl::npos;
   if (pmemvec<T *>::size() == 0 || item > prefvec<T>::tail()) { // often used for push_back, so handle quickly if so
      prefvec<T>::push_back( item );
      where = pmemvec<T *>::size() - 1;
   }
   else {
      // if you call with ADD_HERE, it is assumed you've just used search or searchindex
      // i.e., we don't need to go through the binary search again to find the insert position
      if (type != paftl::ADD_HERE) {
         searchindex(item);
      }
      if (item < prefvec<T>::at(m_current)) {
         prefvec<T>::insert_at( m_current, item );
         where = m_current;
      }
      else if (item > prefvec<T>::at(m_current) || type == paftl::ADD_DUPLICATE) {
         prefvec<T>::insert_at( m_current + 1, item );
         where = m_current + 1;
      }
      else if (type == paftl::ADD_REPLACE || type == paftl::ADD_HERE) {
         // relies on good assignment operator
         prefvec<T>::at(m_current) = item;
      }
      // n.b., type "UNIQUE" does not replace, returns paftl::npos
   }
   return where;
}

template <class T>
void pqvector<T>::sort()
{
   if (pmemvec<T *>::m_length != 0) {
      sort(0,pmemvec<T *>::m_length-1);
   }
}

// rewrite 6-jun-10 (was entering infinite loop, now appears to work properly)
template <class T>
void pqvector<T>::sort(size_t left, size_t right)
{
   size_t i = left, j = right;
   const T& val = prefvec<T>::at((left+right)/2);
   while (j != paftl::npos && i <= j) {
      while (i <= j && prefvec<T>::at(i) < val)
         i++;
      while (j != paftl::npos && prefvec<T>::at(j) > val)
         j--;
      if (j != paftl::npos && i <= j) {
         // swap contents (using pointer)
         T* temp = pmemvec<T *>::m_data[i];
         pmemvec<T *>::m_data[i] = pmemvec<T *>::m_data[j];
         pmemvec<T *>::m_data[j] = temp;
         i++; j--;
      }
   }
   if (j != paftl::npos && left < j)
      sort(left, j);
   if (i < right)
      sort(i, right);
}

///////////////////////////////////////////////////////////////////////////////////////////////

// psubvec is based on pvector, designed for arrays of chars or shorts, it subsumes itself
// so can be stored as a single pointer: useful if you have a lot of empty arrays

template <class T> class psubvec
{
public:
   static const T npos = -1;
protected:
   T *m_data;
public:
   psubvec()
      { m_data = NULL; }
   psubvec(const psubvec<T>& );
   ~psubvec();
   psubvec<T>& operator = (const psubvec<T>& );
   //
   virtual void push_back(const T item);
   virtual void clear();
public:
   bool isEmpty() // isEmpty is provided in addition to size as is quicker to test
      { return m_data == NULL; }
   T size() const
      { return m_data ? m_data[0] : 0; }
   T& operator [] (T pos)
      { return m_data[pos+1]; }
   const T& operator [] (T pos) const
      { return m_data[pos+1]; }
public:
   istream& read( istream& stream, streampos offset = -1 );
   ostream& write( ostream& stream );
};

template <class T>
psubvec<T>::psubvec(const psubvec& v)
{
   if (v.m_data) {
      T length = v.m_data[0];
      T count = 0;
      while (length >>= 1) // find bit length (note: cannot assume int)
         count++;
      m_data = new T [2 << count];
      if (m_data == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * size_t(2 << count) );
      }
      length = v.m_data[0];
      for (T i = 0; i < length + 1; i++) {
         m_data[i] = v.m_data[i];
      }
   }
   else {
      m_data = NULL;
   }
}

template <class T>
psubvec<T>::~psubvec()
{
   if (m_data)
   {
      delete [] m_data;
      m_data = NULL;
   }
}

template <class T>
psubvec<T>& psubvec<T>::operator = (const psubvec<T>& v)
{
   if (this != &v) {
      if (v.m_data) {
         T length = v.m_data[0];
         T count = 0;
         while (length >>= 1) // find bit length (note: cannot assume int)
            count++;
         m_data = new T [2 << count];
         if (m_data == NULL) {
            throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * size_t(2 << count) );
         }
         length = v.m_data[0];
         for (T i = 0; i < length + 1; i++) {
            m_data[i] = v.m_data[i];
         }
      }
      else {
         m_data = NULL;
      }
   }
   return *this;
}

template <class T>
void psubvec<T>::push_back(const T item)
{
   if (!m_data) {
      m_data = new T [2];
      m_data[0] = 1;
      m_data[1] = item;
   }
   else {
      T length = m_data[0] + 1;
      if ((length & (length - 1)) == 0) { // determine if next length would be power of 2
         T *new_data = new T [length << 1];
         if (new_data == NULL) {
            throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * size_t(length << 1) );
         }
         for (T i = 0; i < length; i++)
            new_data[i] = m_data[i];
         delete [] m_data;
         m_data = new_data;
      }
      m_data[0] = length;
      m_data[length] = item;
   }
}

template <class T>
void psubvec<T>::clear()
{
   if (m_data)
   {
      delete [] m_data;
      m_data = NULL;
   }
}

template <class T>
istream& psubvec<T>::read( istream& stream, streampos offset )
{
   if (m_data) {
      delete [] m_data;
      m_data = NULL;
   }
   T length;
   stream.read( (char *) &length, sizeof(T) );
   if (length) {
      T copy = length;
	   T count = 0;
      while (length >>= 1) // find bit length (note: cannot assume int)
        count++;
      m_data = new T [2 << count];
      if (m_data == NULL) {
         throw pexception( pexception::MEMORY_ALLOCATION, sizeof(T) * size_t(2 << count) );
      }
      stream.read((char *) &m_data, sizeof(T)*(copy+1) );
   }
   return stream;
}

template <class T>
ostream& psubvec<T>::write( ostream& stream )
{
   if (m_data) {
      stream.write((char *) &m_data, sizeof(T)*(m_data[0]+1));
   }
   return stream;
}



///////////////////////////////////////////////////////////////////////////////////////////////

// And now the quick mapping routine

// Helper class keyvaluepair...

template <class T1, class T2> class keyvaluepair {
   // Quick mod - TV
#if defined(_WIN32)
protected:
#else
public:
#endif
   T1 m_key;
   T2 m_value;
public:
   keyvaluepair(const T1 key = T1(), const T2 value = T2())
   { m_key = key; m_value = value; }
   T1& key() { return m_key; }
   const T1 key() const { return m_key; }
   T2& value() { return m_value; }
   const T2& value() const { return m_value; }

   // Quick mod - TV
#if defined (_WIN32)
   friend bool operator == <T1,T2>(const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b);
   friend bool operator <  <T1,T2>(const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b);
   friend bool operator >  <T1,T2>(const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b);
#endif
   istream& read( istream& stream );
   ostream& write( ostream& stream );
};
template <class T1, class T2>
inline bool operator == (const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b)
{ return (a.m_key == b.m_key); }
template <class T1, class T2>
inline bool operator < (const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b)
{ return (a.m_key < b.m_key); }
template <class T1, class T2>
inline bool operator > (const keyvaluepair<T1,T2>& a, const keyvaluepair<T1,T2>& b)
{ return (a.m_key > b.m_key); }
// Note: read and write only work for structures without pointers
template <class T1, class T2>
istream& keyvaluepair<T1,T2>::read( istream& stream )
{
   stream.read( (char *) &m_key, sizeof(T1) );
   stream.read( (char *) &m_value, sizeof(T2) );
   return stream;
}
template <class T1, class T2>
ostream& keyvaluepair<T1,T2>::write( ostream& stream )
{
   stream.write( (char *) &m_key, sizeof(T1) );
   stream.write( (char *) &m_value, sizeof(T2) );
   return stream;
}

template <class T1, class T2> class keyvaluepairref
{
   // Quick mod - TV
#if defined(_WIN32)
protected: 
#else
public:
#endif
   T1 m_key;
   T2 *m_value;
public:
   keyvaluepairref(const T1 key = T1())
   { m_key = key; m_value = NULL; }
   keyvaluepairref(const T1 key, const T2& value)
   { m_key = key; m_value = new T2(value); }
   keyvaluepairref(const keyvaluepairref& k)
   { m_key = k.m_key; m_value = new T2(*(k.m_value)); }
   keyvaluepairref& operator = (const keyvaluepairref& k)
   {
      if (this != &k)
      {
         m_key = k.m_key;
         m_value = new T2(*(k.m_value));
      }
      return *this;
   }
   ~keyvaluepairref()
   { if (m_value) {delete m_value; m_value = NULL;} }
   T1& key() { return m_key; }
   const T1 key() const { return m_key; }
   T2& value() { return *m_value; }
   const T2& value() const { return *m_value; }

   // Quick mod - TV
#if defined(_WIN32)
   friend bool operator == <T1,T2>(const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b);
   friend bool operator <  <T1,T2>(const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b);
   friend bool operator >  <T1,T2>(const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b);
#endif

   //
   virtual istream& read( istream& stream );
   virtual ostream& write( ostream& stream );
};
template <class T1, class T2>
inline bool operator == (const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b)
{ return (a.m_key == b.m_key); }
template <class T1, class T2>
inline bool operator < (const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b)
{ return (a.m_key < b.m_key); }
template <class T1, class T2>
inline bool operator > (const keyvaluepairref<T1,T2>& a, const keyvaluepairref<T1,T2>& b)
{ return (a.m_key > b.m_key); }
// Note: read and write only work for structures without pointers
template <class T1, class T2>
istream& keyvaluepairref<T1,T2>::read( istream& stream )
{
   stream.read( (char *) &m_key, sizeof(T1) );

#ifndef _WIN32
   m_value = new T2;
#endif
   stream.read( (char *) m_value, sizeof(T2) );
   return stream;
}
template <class T1, class T2>
ostream& keyvaluepairref<T1,T2>::write( ostream& stream )
{
   stream.write( (char *) &m_key, sizeof(T1) );
   stream.write( (char *) m_value, sizeof(T2) );
   return stream;
}

// ...and yuk! it gets worse... now we have to define a whole new class:

template <class T1,class T2,class Pair> class pmemmap
{
// It looks very similar to a pqvector...
protected:
   pqvector<Pair> m_vector;
public:
   pmemmap() {;}
   pmemmap(const pmemmap<T1,T2,Pair>& map ) { m_vector = map.m_vector; }
   ~pmemmap() {;}
   pmemmap<T1,T2,Pair>& operator = (const pmemmap<T1,T2,Pair>& map )
   { if (this != &map) m_vector = map.m_vector; return *this; }
   //
   // at, [] and so on
   T2& at(size_t i)
   { return m_vector.at(i).value(); }
   const T2& at(size_t i) const
   { return m_vector.at(i).value(); }
   T2& operator [] (size_t i)
   { return m_vector.at(i).value(); }
   const T2& operator [] (size_t i) const
   { return m_vector.at(i).value(); }
   T2& head()
   { return m_vector.head().value(); }
   const T2& head() const
   { return m_vector.head().value(); }
   T2& tail()
   { return m_vector.tail().value(); }
   const T2& tail() const
   { return m_vector.tail().value(); }
   size_t size() const
   { return m_vector.size(); }
   void clear()
   { m_vector.clear(); }
   //
   // standard operations (unordered vector)
   T2& find(const T1& item) const
   { return m_vector.find(Pair(item)).value(); }
   size_t findindex(const T1& item) const
   { return m_vector.findindex(Pair(item)); }
   //
   // binary operations (ordered vector)
   T2& search(const T1& item)
   { return m_vector.search(Pair(item)).value(); }
   const T2& search(const T1& item) const
   { return m_vector.search(Pair(item)).value(); }
   const size_t searchindex(const T1& item) const
   { return m_vector.searchindex(Pair(item)); }
   void remove_at(size_t i)
   { m_vector.remove_at(i); }
   void remove_at(const pvecint& list)
   { m_vector.remove_at(list); }
   void remove(const T1& item)
   { remove_at(m_vector.searchindex(item)); }
   size_t add(const T1& k, const T2& v, int type = paftl::ADD_UNIQUE) // note: does not replace!
   { return m_vector.add( Pair(k,v), type ); }
   // extras
   T1& key(size_t i)
   { return m_vector.at(i).key(); }
   const T1 key(size_t i) const
   { return m_vector.at(i).key(); }
   T2& value(size_t i)
   { return m_vector.at(i).value(); }
   const T2& value(size_t i) const
   { return m_vector.at(i).value(); }
   T2& current()
   { return m_vector.current().value(); }
   const T2& current() const
   { return m_vector.current().value(); }
   // read and write (structures without pointers *only*)
   istream& read( istream& stream );
   ostream& write( ostream& stream );
};

// Note: read and write only work for structures without pointers

template <class T1,class T2,class Pair>
istream& pmemmap<T1,T2,Pair>::read( istream& stream )
{
   for (size_t i = m_vector.size() - 1; i != paftl::npos; i--) {
      m_vector.remove_at(i);
   }
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int length;
   stream.read( (char *) &length, sizeof(unsigned int) );
   for (size_t j = 0; j < size_t(length); j++) {
      // these should be in order, so just push them:
      Pair p;
      p.read(stream);
      m_vector.push_back(p);
   }
   return stream;
}

template <class T1,class T2,class Pair>
ostream& pmemmap<T1,T2,Pair>::write( ostream& stream )
{
   // check for max unsigned int exceeded
   if (m_vector.size() > size_t((unsigned int)-1)) {
      throw pexception( pexception::MAX_ARRAY_EXCEEDED, m_vector.size() );
   }

   // n.b., do not change this to size_t as it will cause 32bit to 64bit conversion problems
   unsigned int length = (unsigned int)(m_vector.size());
   stream.write( (char *) &length, sizeof(unsigned int) );
   for (size_t i = 0; i < m_vector.size(); i++) {
      m_vector[i].write(stream);
   }
   return stream;
}

// ...to stop the MS compiler complaining...
#define kvp(T1,T2) keyvaluepair<T1,T2>

template <class T1, class T2> class pmap : public pmemmap<T1,T2,kvp(T1,T2)>
{
};

// ...to stop the MS compiler complaining...
#define kvpr(T1,T2) keyvaluepairref<T1,T2>

template <class T1, class T2> class pqmap : public pmemmap<T1,T2,kvpr(T1,T2)>
{
};

///////////////////////////////////////////////////////////////////////////////

// and a simple list class...
// (with integrated iterator)

template <class T> class plist
{
protected:
   class pitem
   {
   protected:
      T *m_data;
   public:
      pitem *m_previous;
      pitem *m_next;
      pitem(const T *d = NULL, pitem *p = NULL, pitem *n = NULL)
      {
         if (d)
            m_data = new T(*d);
         else
            m_data = NULL;
         m_previous = p;
         m_next = n;
      }
      pitem(const pitem& p)
      {
         throw 1;
      }
      pitem& operator = (const pitem& p)
      {
         throw 1;
      }
      ~pitem()
      {
         if (m_data)
            delete m_data;
         m_data = NULL;
         m_previous = NULL;
         m_next = NULL;
      }
      void predel()
      {
         if (m_previous != NULL) {
            pitem *temp = m_previous->m_previous;
            temp->m_next = this;      // should work without reallocation
            delete m_previous;
            m_previous = temp;        // should work without reallocation
         }
      }
      void postdel()
      {
         if (m_next != NULL) {
            pitem *temp = m_next->m_next;
            temp->m_previous = this;  // should work without reallocation
            delete m_next;
            m_next = temp;            // should work without reallocation
         }
      }
      pitem *preins(const T& v)
      {
         pitem *temp = m_previous;
         m_previous = new pitem(&v, temp, this);
         temp->m_next = m_previous;  // should work without reallocation
         return m_previous;
      }
      pitem *postins(const T& v)
      {
         pitem *temp = m_next;
         m_next = new pitem(&v, this, temp);
         temp->m_previous = m_next;  // should work without reallocation
         return m_next;
      }
      T& data() const
      {
         if (!m_data) {
            throw 1;
         }
         return *m_data;
      }
   };
protected:
   pitem m_head;
   pitem m_tail;
   mutable pitem *m_current;
public:
   bool empty() const
   {
      return (m_head.m_next == &m_tail); // or v.v.
   }
   void pop_back()
   {
      m_tail.predel();  // i.e., delete the one before the tail
   }
   void pop_front()
   {
      m_head.postdel();   // i.e., delete the one after the head
   }
   void push_back(const T& v)
   {
      m_tail.preins(v); // i.e., add before the tail
   }
   void push_front(const T& v)
   {
      m_head.postins(v); // i.e., add after the head
   }
   void first() const
   {
      m_current = m_head.m_next;
   }
   void last() const
   {
      m_current = m_tail.m_previous;
   }
   bool is_head() const
   {
      return (m_current == &m_head);
   }
   bool is_tail() const
   {
      return (m_current == &m_tail);
   }
   const plist<T>& operator ++(int) const
   {
      m_current = m_current->m_next;
      return *this;
   }
   const plist<T>& operator --(int) const
   {
      m_current = m_current->m_previous;
      return *this;
   }
   T& operator *() const {
      return m_current->data();
   };
   void preins(const T& data) {
      m_current->preins(data);
   }
   void postins(const T& data) {
      m_current->postins(data);
   }
   // note pre and post del delete the current item:
   void predel() {
      m_current = m_current->m_previous;
      m_current->postdel();
   }
   void postdel() {
      m_current = m_current->m_next;
      m_current->predel();
   }
   void clear()
   {
      while (!empty()) {
         pop_back();
      }
   }
public:
   plist()
   {
      m_head.m_next = &m_tail;
      m_tail.m_previous = &m_head;
      m_current = &m_head; // just so it's somewhere...
   }
   plist(const plist& list) {
      m_head.m_next = &m_tail;
      m_tail.m_previous = &m_head;
      m_current = list.m_current;
      for (list.first(); !list.is_tail(); list++) {
         push_back(*list);
      }
      list.m_current = m_current; // set back original list current to what it was before
   }
   plist& operator = (const plist& list) {
      if (&list != this) {
         m_current = list.m_current;
         clear();
         for (list.first(); list.is_tail(); list++) {
            push_back(*list);
         }
         list.m_current = m_current; // set back original list current to what it was before
      }
      return *this;
   }
   ~plist()
   {
      // remove contents:
      clear();
   }
};

///////////////////////////////////////////////////////////////////////////////

// ptree: a simple tree class

// Allows template of a template (recursive) definition for tree
#define ptreeT ptree<T>

template <class T> class ptree
{
public:
   class exception : public pexception
   {
   public:
      enum exception_t { PTREE_UNDEFINED = 0x1000,
                         UNASSIGNED_DATA = 0x1001 };
   public:
      exception(int n_exception = PTREE_UNDEFINED) : pexception( n_exception ) {}
   };
protected:
   T *m_data;
   ptree<T> *m_parent;
   pvector<ptreeT *> m_children;
public:
   ptree() {
      m_data = NULL;
      m_parent = NULL;
   }
   ptree(const T& data) {
      m_data = new T(data);
      m_parent = NULL;
   }
   ptree(const ptree<T>& tree ) {
      m_data = tree.m_data;
      for (int i = 0; i < tree.m_children.size(); i++) {
         ptree<T> *child = new ptree<T>( *(tree.m_children[i]) );
         m_children.push_back( child );
         m_children.tail()->m_parent = this;
      }
   }
   ptree<T>& operator = (const ptree<T>& tree) {
      if (this != &tree) {
         m_data = tree.m_data;
         for (int i = 0; i < tree.m_children.size(); i++) {
            ptree<T> *child = new ptree<T>( *(tree.m_children[i]) );
            m_children.push_back( child );
            m_children.tail()->m_parent = this;
         }
      }
      return *this;
   }
   ~ptree() {
      delete m_data;
      m_data = NULL;
      while (m_children.size()) {
         delete m_children.tail();
         m_children.pop_back();
      }
   }
   void addChild(const T& data) {
      m_children.push_back( new ptree<T>(data) );
      m_children.tail()->m_parent = this;
   }
   void removeChild(int ref) {
      ptree<T> *child = m_children[ref];
      m_children.remove(ref);
      delete child;
   }
   int getChildCount() const {
      return m_children.size();
   }
   ptree<T>& getChild(int ref) const {
      return *(m_children[ref]);
   }
   ptree<T>& getLastChild() const {
      return *(m_children[m_children.size() - 1]);
   }
   int hasParent() const {
      return m_parent ? 1 : 0;
   }
   ptree<T>& getParent() const {
      return *m_parent;
   }
   void setParent(ptree<T>& parent) {
      if (m_parent) {
         for (int i = 0; i < m_parent->m_children.size(); i++) {
            if (m_parent->m_children[i] == this) {
               m_parent->m_children.remove(i);
            }
         }
      }
      m_parent = &parent;
   }
   void setValue(const T& data) {
      m_data = new T(data);
   }
   T& getValue() const {
      if (!m_data)
         throw exception( exception::UNASSIGNED_DATA );
      return *m_data;
   }
   pvector<T> getPath() {
      pvector<T> path;
      getPath(path);
      return path;
   }
   void getPath(pvector<T>& path) {
      path.push_back( getValue() );
      if (m_parent) {
         m_parent->getPath(path);
      }
   }
};

///////////////////////////////////////////////////////////////////////////////

template <class T> class pflipper
{
protected:
   T m_contents[2];
   short parity;
public:
   pflipper() {
      parity = 0;
   }
   pflipper( const T& a, const T& b ) {
      parity = 0;
      m_contents[0] = a;
      m_contents[1] = b;
   }
   pflipper( const pflipper& f ) {
      parity = f.parity;
      m_contents[0] = f.m_contents[0];
      m_contents[1] = f.m_contents[1];
   }
   virtual ~pflipper() {
   }
   pflipper& operator = (const pflipper& f ) {
      if (this != &f) {
         parity = f.parity;
         m_contents[0] = f.m_contents[0];
         m_contents[1] = f.m_contents[1];
      }
      return *this;
   }
   void flip() {
      parity = (parity == 0) ? 1 : 0;
   }
   T& a() {
      return m_contents[parity];
   }
   T& b() {
      return m_contents[(parity == 0) ? 1 : 0];
   }
   const T& a() const {
      return m_contents[parity];
   }
   const T& b() const {
      return m_contents[(parity == 0) ? 1 : 0];
   }
};

///////////////////////////////////////////////////////////////////////////////

// pstring: a simple string class

class pstring
{
public:
   class exception : public pexception
   {
   public:
      enum exception_t { PSTRING_UNDEFINED = 0x1000,
                         BAD_STRING        = 0x1001,
                         OUT_OF_RANGE      = 0x1002,
                         UNABLE_TO_CONVERT = 0x1003 };
   public:
      exception(int n_exception = PSTRING_UNDEFINED) : pexception( n_exception ) {}
   };

protected:
   size_t  m_alloc;
   size_t  m_start;
   size_t  m_end;
   char *m_data;

public:
   pstring(const char *data = NULL)
   {
      if (!data || (m_end = strlen(data)) < 1) {
         m_data = NULL;
         m_start = 0;
         m_end = 0;
         m_alloc = 0;
      }
      else {
         m_start = 0;
         m_alloc = m_end + 1;
         m_data = new char [m_alloc];
         if (!m_data)
            throw pexception( pexception::MEMORY_ALLOCATION );
         for (size_t i = m_start; i < m_end; i++) m_data[i] = data[i];
         m_data[m_end] = '\0';
      }
   }
   // AT 31.01.11 -- wchar_t implementation
   pstring(const wchar_t *data)
   {
      size_t len = wcslen(data);
      if (!data || len == 0) {
         m_data = NULL;
         m_start = 0;
         m_end = 0;
         m_alloc = 0;
      }
      else {
         m_start = 0;
         m_end = 0;
         m_alloc = len * 2 + 1; // add some redundancy
         m_data = new char [m_alloc];
         if (!m_data)
            throw pexception( pexception::MEMORY_ALLOCATION );
         m_data[m_end] = '\0';
         char *buf = new char[MB_CUR_MAX];
         for (size_t i = 0; i < len && data[i] != L'\0'; i++) {
            int x = wctomb(buf,data[i]);
            if (x != -1) {
               if (m_end + x >= m_alloc - 1) {
                  m_alloc += len;
                  char *temp = new char [m_alloc];
                  strcpy(temp,m_data);
                  delete [] m_data;
                  m_data = temp;
               }
               if (x == 1) {
                  m_data[m_end] = buf[0];
               }
               else {
                  strncat(m_data, buf, x);
               }
               m_end += x;
               m_data[m_end] = '\0';
            }
         }
         delete buf;
      }
   }
   // Changed 31.08.09 -- pstring no longer has a const char constructor
   // this is to avoid accidental construction from an int
   // Instead, there is now the following constructor compatible with STL string:
   pstring(size_t n, char c)
   {
      if (n < 1) {
         m_data = NULL;
         m_start = 0;
         m_end = 0;
         m_alloc = 0;
      }
      else {
         m_start = 0;
         m_end = n;
         m_alloc = m_end + 1;
         m_data = new char [m_alloc];
         if (!m_data)
            throw pexception( pexception::MEMORY_ALLOCATION );
         memset(m_data,c,n);
         m_data[m_end] = '\0';
      }
   }
   // Changed 31.08.09 -- pstring no longer has an int constructor
   // this is to avoid accidental construction from an int
   // instead, use 'reserve' (as you would for an STL string)
   // Added V1.6 to allow initialisation from long / double
   // (due to the phasing out of the pdata class)
   //
   friend pstring pstringify(const char c);
   friend pstring pstringify(const int val, const pstring& format);
   friend pstring pstringify(const double val, const pstring& format);
   //
   pstring(const pstring& s)
   {
      if (s.m_alloc == 0) {
         m_data = NULL;
         m_start = 0;
         m_end = 0;
         m_alloc = 0;
      }
      else if (!s.m_data)
         throw exception( exception::BAD_STRING );
      else {
         m_start = 0;
         m_end = s.m_end - s.m_start;
         m_alloc = m_end + 1;
         m_data = new char [m_alloc];
         if (!m_data)
            throw pexception( pexception::MEMORY_ALLOCATION );
         for (size_t i = 0; i < m_end; i++) {
            m_data[i] = s.m_data[i + s.m_start];
         }
         m_data[m_end] = '\0';
      }
   }
   virtual ~pstring()
   {
      if (m_data) {
         delete [] m_data; m_data = NULL; m_start = 0; m_end = 0; m_alloc = 0;
      }
   }
   pstring& operator = (const pstring& s)
   {
      if (&s != this) {
         if (m_alloc < s.m_end - s.m_start + 1) {
            if (m_data) {
               delete [] m_data;
            }
            if (s.m_end - s.m_start) {
               m_alloc = s.m_end - s.m_start + 1;
               m_data = new char [s.m_alloc];
               if (!m_data)
                  throw pexception( pexception::MEMORY_ALLOCATION );
            }
            else {
               m_alloc = 0;
               m_data = NULL;
            }
         }
         m_start = 0;
         m_end = s.m_end - s.m_start;
         if (s.m_end - s.m_start) {
            if (!s.m_data)
               throw exception( exception::BAD_STRING );
            for (size_t i = 0; i < m_end; i++)
               m_data[i] = s.m_data[i + s.m_start];
            m_data[m_end] = '\0';
         }
      }
      return *this;
   }
   void reserve(const size_t length)
   {
      if (length < 1) {
         if (m_data)
            delete m_data;
         m_data = NULL;
         m_start = 0;
         m_end = 0;
         m_alloc = 0;
      }
     else if (m_alloc < length + 1) {
         if (m_data)
            delete m_data;
         m_start = length;
         m_end   = length;
         m_alloc = length + 1;
         m_data = new char [m_alloc];
         if (!m_data) {
            throw pexception( pexception::MEMORY_ALLOCATION );
         }
         m_data[m_end] = '\0'; // ensure terminated, even if contains no real data
      }
   }
   //
   const char *c_str() const
   {
      return m_data + m_start;   // Note! Not persistent: do not pass this but c_strcpy()!
   }
   char *c_strcpy(char *str) const
   {
      return strcpy( str, (m_data + m_start) );
   }
   // Simple conversions (throw exceptions if not valid)
   int c_int() const;
   double c_double() const;
   // Test validity (through attempted conversion)
   bool is_int() const;
   bool is_double() const;

   char& operator [] (size_t i) const
   {
      if (i >= (m_end - m_start))
         throw exception( exception::OUT_OF_RANGE );
      return m_data[i + m_start];
   }
   void clear()
      { m_start = 0; m_end = 0; }   // Note: no deallocation
   bool empty() const
      { return (m_end == m_start) ? true : false; }
   size_t size() const
      { return m_alloc; }
   size_t length() const
      { return m_end - m_start; }

   pstring substr( size_t start = 0, size_t length = paftl::npos ) const;

   size_t findindex( char c, size_t start = 0, size_t end = paftl::npos ) const;
   size_t findindexreverse( char c, size_t start = paftl::npos, size_t end = 0 ) const;
   size_t findindex( const pstring& str, size_t start = 0, size_t end = paftl::npos ) const;

   // Full tokenizer:
   pvecstring tokenize( char delim = ' ', bool ignorenulls = false );

   pstring splice( char delim = ' ' );
   pstring splice( const pstring& delim );

   pstring& ltrim( char c = ' ');
   pstring& rtrim( char c = ' ');
   pstring& ltrim( const pstring& str );

   pstring& replace( char a );   // simply remove all instances of char a
   pstring& replace( char a, char b );

   pstring& makelower();
   pstring& makeupper();
   pstring& makeinitcaps();

   friend bool compare(const pstring& a, const pstring& b, size_t n);
   friend bool operator == (const pstring& a, const pstring& b);
   friend bool operator != (const pstring& a, const pstring& b);
   friend bool operator <  (const pstring& a, const pstring& b); // for storing in hash tables etc
   friend bool operator >  (const pstring& a, const pstring& b);

   friend pstring operator + (const pstring& a, const pstring& b);

   friend ostream& operator << (ostream& stream, const pstring& str);
   friend istream& operator >> (istream& stream, pstring& str);

   char *raw() const
      { return m_data; }

   // binary read/write
   bool read(istream& stream);
   bool write(ostream& stream);
};

inline pstring pstringify(const char val)
{
   pstring str(1,val);
   return str;
}

inline pstring pstringify(const int val, const pstring& format = "% 16d")
{
   pstring str;
   str.m_start = 0;
   str.m_alloc = 16 + format.length();
   str.m_data = new char [str.m_alloc];
   if (!str.m_data) {
      throw pexception( pexception::MEMORY_ALLOCATION );
   }
   sprintf( str.m_data, format.c_str(), val );
   str.m_end = strlen(str.m_data);
   return str;
}

inline pstring pstringify(const double val, const pstring& format = "%+.16le")
{
   pstring str;
   str.m_start = 0;
   str.m_alloc = 24 + format.length();
   str.m_data = new char [str.m_alloc];
   if (!str.m_data) {
      throw pexception( pexception::MEMORY_ALLOCATION );
   }
   sprintf( str.m_data, format.c_str(), val );
   str.m_end = strlen(str.m_data);
   return str;
}

inline pstring pstring::substr(size_t start, size_t length) const
{
   if (length > (m_end - m_start - start))
      length = m_end - m_start - start;
   pstring sub;
   sub.m_start = 0;
   sub.m_end = length;
   sub.m_alloc = length + 1;
   sub.m_data = new char [sub.m_alloc];
   for (size_t i = 0; i < sub.m_end; i++)
      sub.m_data[i] = m_data[i + m_start + start];
   sub.m_data[length] = '\0';
   return sub;
}

inline size_t pstring::findindex( char c, size_t start, size_t end ) const
{
   if (end == paftl::npos || end > m_end - m_start) {
      end = m_end - m_start;
   }
   for (size_t pos = start; pos < end; pos++) {
      if (m_data[pos + m_start] == c) {
         return pos;
      }
   }
   return paftl::npos;
}

inline size_t pstring::findindexreverse( char c, size_t start, size_t end ) const
{
   if (start == paftl::npos || start > m_end - m_start) {
      start = m_end - m_start;
   }
   for (size_t pos = start - 1; pos != paftl::npos && pos >= end; pos--) {
      if (m_data[pos + m_start] == c) {
         return pos;
      }
   }
   return paftl::npos;
}

inline size_t pstring::findindex( const pstring& str, size_t start, size_t end ) const
{
   if (end == paftl::npos) {
      end = m_end - m_start;
   }
   for (size_t pos = start; pos < end; pos++) {
      if (m_data[pos + m_start] == str.m_data[str.m_start]) {
         size_t mark = pos, remark = paftl::npos;
         while (mark != paftl::npos && ++pos < str.m_end - str.m_start + mark) {
            if (pos >= end) {
               return paftl::npos;
            }
            else if (m_data[pos+m_start] != str.m_data[pos-mark+str.m_start]) {
               mark = paftl::npos;
            }
            else if (m_data[pos+str.m_start] == str.m_data[str.m_start]
                     && remark == paftl::npos) {
               remark = pos;
            }
         }
         if (mark != paftl::npos) {
            return mark;
         }
         else if (remark != paftl::npos) {
            return findindex( str, remark );
         }
      }
   }
   return paftl::npos;
}

inline pstring& pstring::ltrim( char c )
{
   size_t pos = m_start;
   while (pos < m_end && m_data[pos] == c)
      pos++;
   m_start = pos;
   return *this;
}

inline pstring& pstring::rtrim( char c )
{
   // this check is necessary, as m_data[m_end] will be reset and could be null
   if (m_start != m_end) {
      size_t pos = m_end;
      while (pos > m_start && m_data[pos-1] == c)
         pos--;
      m_end = pos;
      m_data[m_end] = '\0';
   }
   return *this;
}

inline pstring& pstring::ltrim( const pstring& str )
{
   size_t pos = m_start;
   while (pos < m_end && str.findindex(m_data[pos]) != paftl::npos )
      pos++;
   m_start = pos;
   return *this;
}

// tokenize: going beyond splice...

inline pvecstring pstring::tokenize( char delim, bool ignorenulls )
{
   pvecstring tokens;
   size_t first, last = 0;
   while (last + m_start < m_end) {
      first = last;
      last = findindex(delim, first);
      if (last == paftl::npos)
         last = m_end - m_start;
      tokens.push_back( substr(first, last - first) );
      if (ignorenulls) {
         while (last + m_start < m_end && m_data[last+m_start] == delim)
            last++;     // Lose consecutive delimiters
      }
      else {
         last++;        // Lose delimiter
      }
   }
   return tokens;
}

// splice: kind of combined substr, find operation:

// returns the head, the tail is kept, the delimiter is destroyed
// (the leading whitespace destroy is no longer used: use x.ltrim().splice() )

inline pstring pstring::splice( char delim )
{
   size_t pos = 0;
   pos = findindex(delim, pos);
   if (pos == paftl::npos)
      pos = m_end - m_start;
   pstring str = substr(0, pos);
   pos++; // Lose delimiter
   m_start = m_end > pos + m_start ? pos + m_start : m_end;
   return str;
}

// returns the head, the tail is kept, the delimiter is destroyed

inline pstring pstring::splice( const pstring& delim )
{
   size_t pos = findindex(delim);
   if (pos == paftl::npos)
      pos = m_end - m_start;
   pstring str = substr(0, pos);
   pos += delim.m_end; // Lose delimiter
   m_start = m_end > pos + m_start ? pos + m_start : m_end;
   return str;
}

// Conversion

inline int pstring::c_int() const
{
   int out = 0;
   if (m_start != m_end) {
      char *endptr = m_data + m_start;
      out = (int) strtol( m_data + m_start, &endptr, 10 );
      if (endptr == m_data + m_start) {
         throw exception( exception::UNABLE_TO_CONVERT );
      }
   }
   return out;
}

inline double pstring::c_double() const
{
   double out = 0.0;
   if (m_start != m_end) {
      char *endptr = m_data + m_start;
      out = strtod( m_data + m_start, &endptr );
      if (endptr == m_data + m_start) {
         throw exception( exception::UNABLE_TO_CONVERT );
      }
   }
   return out;
}

// only way to test is to try it!
inline bool pstring::is_int() const
{
   int out = 0;
   if (m_start != m_end) {
      char *endptr = m_data + m_start;
      out = (int) strtol( m_data + m_start, &endptr, 10 ); // 10 is base 10
      if (endptr != m_data + m_start) {
         return true;
      }
   }
   return false;
}

// only way to test is to try it!
inline bool pstring::is_double() const
{
   double out = 0.0;
   if (m_start != m_end) {
      char *endptr = m_data + m_start;
      out = strtod( m_data + m_start, &endptr );
      if (endptr != m_data + m_start) {
         return true;
      }
   }
   return false;
}

// Comparison

inline bool compare(const pstring& a, const pstring& b, size_t n)
{
   if (n > a.length() || n > b.length()) {
      return false;
      //throw pstring::exception( pstring::exception::OUT_OF_RANGE );
   }
   for (size_t i = 0; i < n; i++) {
      if ( a.m_data[i+a.m_start] != b.m_data[i+b.m_start] ) {
         return false;
      }
   }
   return true;
}

inline bool operator != (const pstring& a, const pstring& b)
{
   if (a.length() != b.length()) {
      return true;
   }
   for (size_t i = 0; i < a.length(); i++)
   {
      if (a.m_data[i+a.m_start] != b.m_data[i+b.m_start]) {
         return true;
      }
   }
   return false;
}

inline bool operator == (const pstring& a, const pstring& b)
{
   return !(a != b);
}

inline bool operator < (const pstring& a, const pstring& b) // for storing in hash tables etc
{
   for (size_t i = 0; i < b.length(); i++) {
      if (i == a.length()) {
         return true;   // <- if all characters are the same, but b is longer than a, then a < b
      }
      else if (a.m_data[i+a.m_start] < b.m_data[i+b.m_start]) {
         return true;
      }
      else if (a.m_data[i+a.m_start] > b.m_data[i+b.m_start]) {
         return false;
      }
   }
   return false;
}

inline bool operator > (const pstring& a, const pstring& b) // for storing in hash tables etc
{
   for (size_t i = 0; i < a.length(); i++) {
      if (i == b.length()) {
         return true;   // <- if all characters are the same, but a is longer than b, then a > b
      }
      else if (a.m_data[i+a.m_start] > b.m_data[i+b.m_start]) {
         return true;
      }
      else if (a.m_data[i+a.m_start] < b.m_data[i+b.m_start]) {
         return false;
      }
   }
   return false;
}

inline pstring operator + (const pstring& a, const pstring& b)
{
   pstring str;
   str.reserve(a.length() + b.length());
   str.m_start = 0;
   for (size_t i = 0; i < str.m_end; i++)
   {
      str.m_data[i] = (i < a.length()) ? a.m_data[i+a.m_start] :
                                         b.m_data[i+b.m_start - a.length()];
   }
   return str;
}

inline ostream& operator << (ostream& stream, const pstring& str)
{
   if (str.empty()) {
      return stream;
   }
   return ( stream << str.c_str() );
}

inline istream& operator >> (istream& stream, pstring& str)
{
   // default is 128, but grows upto required length
   if (str.m_alloc < 128) {
      str.m_alloc = 128;
      delete [] str.m_data;
      str.m_data = new char [str.m_alloc];
      if (!str.m_data)
         throw pexception( pexception::MEMORY_ALLOCATION );
   }
   for (size_t i = 0; ; i++) {
      if (i + 1 == str.m_alloc) {
         char *old_data = str.m_data;
         str.m_data = new char[str.m_alloc * 2];
         for (size_t j = 0; j < str.m_alloc; j++)
            str.m_data[j] = old_data[j];
         delete [] old_data;
         str.m_alloc *= 2;
      }
      stream.get( str.m_data[i] );
      // cross platform --- just do this yourself: \n is for MAC = 13, UNIX = 10, MS = 13,10
      if (str.m_data[i] == 10 || stream.eof() || stream.fail()) {
         str.m_data[i] = '\0';
         break;
      }
      else if (str.m_data[i] == 13) {
         if (stream.peek() == 10)
            stream.get( str.m_data[i] );
         str.m_data[i] = '\0';
         break;
      }
   }
   str.m_start = 0;
   str.m_end = strlen(str.m_data);
   return stream;
}

// binary read / write

inline bool pstring::read(istream& stream)
{
   // actually, required alloc is 1 less than required alloc due to null terminator
   unsigned int required_alloc;
   // n.b., there are 32-bit vs 64-bit size_t differences: you must read this in as an
   // unisgned int, or suffer translation problems across platforms
   stream.read( (char *) &required_alloc, sizeof(unsigned int) ); // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   if (required_alloc > 0 && m_alloc < required_alloc + 1) {
      m_alloc = size_t(required_alloc + 1);
      delete [] m_data;
      m_data = new char [m_alloc];
      if (!m_data)
         throw pexception( pexception::MEMORY_ALLOCATION );
   }
   if (required_alloc != 0) {
      stream.read(m_data, streamsize(required_alloc) * sizeof(char));
      m_data[required_alloc] = '\0';
   }
   m_start = 0;
   m_end = size_t(required_alloc);

   return true;
}

inline bool pstring::write(ostream& stream)
{
   // n.b., do not change this to size_t as it will cause 32-bit to 64-bit conversion problems
   unsigned int required_alloc = 0;
   if (m_data)
      required_alloc = (unsigned int)(m_end - m_start);
   stream.write( (char *) &required_alloc, sizeof(unsigned int) );
   if (required_alloc > 0)
      stream.write( m_data + m_start, required_alloc );
   return true;
}

/*
// the default delim_list isn't liked in standard C++
// this function does not appear to be used in depthmapX and has been removed
inline pstring readtoken(istream& stream, const pstring& delim_list)
{
   pstring str;
   str.m_start = 0;
   str.m_end = 0;
   str.m_alloc = 128;
   str.m_data = new char [str.m_alloc];

   char single_char;
   size_t here = paftl::npos;

   while ( !stream.eof() && str.m_end < 128 ) {
      single_char = stream.get();
      if (here == -1) {
         if (isprint(single_char) && delim_list.findindex(single_char) == paftl::npos) {
            here = 0;
            str.m_data[ 0 ] = single_char;
            str.m_end++;
         }
      }
      else {
         here++;
         if (!isprint(single_char) || delim_list.findindex(single_char) != paftl::npos) {
            str.m_data[ str.m_end ] = '\0';
            return str;
         }
         else {
            str.m_data[ str.m_end ] = single_char;
            str.m_end++;
         }
      }
   }
   str.m_data[ str.m_end ] = '\0';
   return str;
}
*/

inline pstring& pstring::replace(char a)
{
   size_t i = m_start;
   for (size_t j = m_start; j < m_end; i++, j++) {
      if (a == m_data[j]) {
         j++;
         if (j == m_end) {
            break;
         }
      }
      m_data[i] = m_data[j];
   }
   m_end = i;
   m_data[ i ] = '\0';
   return *this;
}

inline pstring& pstring::replace(char a, char b)
{
   for (size_t i = m_start; i < m_end; i++) {
      if (a == m_data[i]) {
         m_data[i] = b;
      }
   }
   return *this;
}

inline pstring& pstring::makelower()
{
   for (size_t i = m_start; i < m_end; i++) {
      // ASCII caps
      if (isupper(m_data[i])) {
         m_data[i] = tolower(m_data[i]);
      }
   }
   return *this;
}
inline pstring& pstring::makeupper()
{
   for (size_t i = m_start; i < m_end; i++) {
      // ASCII lower
      if (islower(m_data[i])) {
         m_data[i] = toupper(m_data[i]);
      }
   }
   return *this;
}
inline pstring& pstring::makeinitcaps()
{
   bool reset = true;
   bool liter = false;
   for (size_t i = m_start; i < m_end; i++) {
      if (!isalpha(m_data[i])) {
         reset = true;
         if (m_data[i] == '\"') {
            // ignore things within quotation marks
            liter = !liter;
         }
      }
      else if (!liter) {
         if (reset) {
            if (islower(m_data[i])) {
               m_data[i] = toupper(m_data[i]);
            }
         }
         else if (isupper(m_data[i])) {
            m_data[i] = tolower(m_data[i]);
         }
         reset = false;
      }
   }
   return *this;
}


///////////////////////////////////////////////////////////////////////////////

// Read and write runlength encoded vectors, with byte alignment through T

// note: vector must have been allocated to accept stream
template <class T>
istream& read_rle( istream& stream, T *vector, size_t length )
{
   unsigned char *data = (unsigned char *) vector;
   for (size_t i = 0; i < sizeof(T); i++) {
      unsigned char runlength = 0, current, last;
      size_t count = 0;
      while (count < length) {
         stream.get((char&)current);
         if (count && current == last) {
            stream.get((char&)runlength);
         }
         else {
            last = current;
            runlength = 1;
         }
         for (size_t i = count; i < count + runlength; i++) {
            data[i + sizeof(T) * count] = current;
         }
         count += runlength;
      }
   }
   return stream;
}

template <class T> ostream& write_rle( ostream& stream, T *vector, size_t length )
{
   unsigned char *data = (unsigned char *) vector;
   for (size_t i = 0; i < sizeof(T); i++) {
      unsigned char runlength = 0, current;
      size_t count = 0;
      while (count < length) {
         do {
            current = data[i + sizeof(T) * count];
            runlength++;
            count++;
         } while (count < length && current == data[i + sizeof(T) * count] && runlength < 255);
         if (runlength == 1) {
            stream.put(current);
         }
         else {
            stream.put(current);
            stream.put(current);
            runlength -= 1; // since we've written current twice anyway to mark the run
            stream.put(runlength);
         }
         runlength = 0;
      }
   }
   return stream;
}

///////////////////////////////////////////////////////////////////////////////

// Hashing for LZW compression

const size_t HASHBITSIZE = 12;
const size_t HASHTABLESIZE = 5021; // n.b., prime > 4096
//const size_t HASHTABLESIZE = 65983; // n.b., prime > 65536
const size_t HASHMAXVALUE = (1 << HASHBITSIZE) - 1;
const size_t HASHMAXCODE = HASHMAXVALUE - 1;

struct phash
{
   unsigned int code;
   unsigned int prefix;
   unsigned char character;
};

class phashtable
{
protected:
   mutable size_t m_current;
   unsigned int m_nextcode;
   phash m_table[HASHTABLESIZE];
public:
   phashtable();
   void add_encode(unsigned int prefix, unsigned char character);
   void add_decode(unsigned int prefix, unsigned char character);
   size_t search(unsigned int prefix, unsigned char character) const;
   unsigned char *decode(unsigned char *buffer, unsigned int code);
   int getnextcode()
   { return m_nextcode; }
};

inline phashtable::phashtable()
{
   for (size_t i = 0; i < HASHTABLESIZE; i++) {
      m_table[i].code = -1;
   }
   m_nextcode = 256; // 0-255 for standard characters
}

inline void phashtable::add_encode(unsigned int prefix, unsigned char character)
{
   if (m_nextcode <= HASHMAXCODE) {
      m_table[m_current].code = m_nextcode;
      m_table[m_current].prefix = prefix;
      m_table[m_current].character = character;
      m_nextcode++;
   }
}

inline void phashtable::add_decode(unsigned int prefix, unsigned char character)
{
   if (m_nextcode <= HASHMAXCODE) {
      m_table[m_nextcode].prefix = prefix;
      m_table[m_nextcode].character = character;
      m_nextcode++;
   }
}

inline size_t phashtable::search(unsigned int prefix, unsigned char character) const
{
   // the bracket overkill on the following line ensures paftl.h compiles on a GNU compiler
   m_current = (((unsigned int) character) << (HASHBITSIZE-8)) ^ prefix;
   size_t offset;
   if (m_current == 0) {
      offset = 1;
   }
   else {
      offset = HASHTABLESIZE - m_current;
   }
   while ( m_table[m_current].code != (unsigned int) -1 &&
          (m_table[m_current].prefix != prefix || m_table[m_current].character != character))
   {
      if (offset > m_current)
         m_current += HASHTABLESIZE;
      m_current -= offset;
   }
   return m_table[m_current].code;
}

inline unsigned char *phashtable::decode(unsigned char *buffer, unsigned int code)
{
  while (code > 255)
  {
    *buffer++ = m_table[code].character;
    code = m_table[code].prefix;
  }
  *buffer = code;
  return buffer;
}

///////////////////////////////////////////////////////////////////////////////

// LZW as of June 2003 US patent has expired
//     as of June 2004 the European patent expires

// Note to paftl users: ensure that you meet patent requirements for your usage

// LZW uses a class so that the hash table may be retained over multiple
// vector read / writes, as well as holding a read buffer

// 12 bit, 4096 pattern holder

// Note: you must *flush* after writing -- the idea is you can write several
// vectors before a flush

template <class T> class plzw
{
protected:
   // read / write buffering
   unsigned long m_bitbuffer;
   bool m_bitswaiting;
protected:
   bool m_firstever;
   unsigned char m_character;
   unsigned int m_prefix;
   phashtable m_hashtable;
   unsigned char m_decodedstring[HASHTABLESIZE];   // <- impossible that the decode string is ever as long as the hash table size
public:
   plzw();
   istream& read( istream& stream, T *vector, int length );
   ostream& write( ostream& stream, T *vector, int length );
protected:
   istream& get(istream& stream, unsigned int& code);
   ostream& put(ostream& stream, const unsigned int code);
};

template <class T>
plzw<T>::plzw()
{
   m_firstever = true;
   m_bitswaiting = false;
}

template <class T>
istream& plzw<T>::read(istream& stream, T *vector, int length )
{
   unsigned char *data = (unsigned char *) vector;
   unsigned char *string;

   if (m_firstever) {
      get(stream, m_prefix);
      m_character = m_prefix;
      data[0] = m_character;
   }

   // T is sequenced, as we assume that patterns will recur through aligned bytes of T
   for (unsigned int i = (m_firstever ? 1 : 0); i < sizeof(T) * length;) {
      unsigned int nextcode;
      get(stream, nextcode);
      if (nextcode >= (unsigned int) m_hashtable.getnextcode()) {
         *m_decodedstring = m_character;
         string = m_hashtable.decode(m_decodedstring + 1, m_prefix);
      }
      else {
         string = m_hashtable.decode(m_decodedstring, nextcode);
      }
      m_character = *string;
      if (i != 0) {
         // this should be skipped on initial read
         m_hashtable.add_decode(m_prefix,m_character);
      }
      while (string >= m_decodedstring && i < sizeof(T) * length) {
         data[i] = *string--;
         i++;
      }
      m_prefix = nextcode;
   }

   // skip to next byte boundary for next read... (and retain context)
   m_bitswaiting = false;
   m_firstever = false;

   return stream;
}

template <class T>
ostream& plzw<T>::write(ostream& stream, T *vector, int length )
{
   unsigned char *data = (unsigned char *) vector;

   m_prefix = data[0];

   for (unsigned int i = 0; i < sizeof(T) * length; i++) {
      m_character = data[i];
      int code = m_hashtable.search(m_prefix, m_character);
      if (code != -1) {
         m_prefix = code;
      }
      else {
         m_hashtable.add_encode(m_prefix, m_character);
         put(stream, m_prefix);
         m_prefix = m_character;
      }
   }

   // skip to next byte boundary
   put(stream, m_prefix);
   if (m_bitswaiting) {
      stream.put((unsigned char)(m_bitbuffer & 0xFF));
      m_bitswaiting = false;
   }

   return stream;
}

// stored, 1 and 2 are the 12-bit codes,
// a b & c are first 4 bits, second 4 bits and third four bits respectively
// b1 a1
// c2 c1
// b2 a2

template <class T>
istream& plzw<T>::get(istream& stream, unsigned int& code)
{
   unsigned char bits;
   if (!m_bitswaiting) {
      stream.get((char&)bits);
      code = bits;
      stream.get((char&)bits);
      code |= (bits & 0x0F) << 8;
      m_bitbuffer = bits & 0xF0;
      m_bitswaiting = true;
   }
   else {
      stream.get((char&)bits);
      code = ((unsigned int)bits) | (m_bitbuffer << 4);
      m_bitswaiting = false;
   }

   return stream;
}

template <class T>
ostream& plzw<T>::put(ostream& stream, const unsigned int code)
{
   if (!m_bitswaiting) {
      stream.put((unsigned char)(code & 0xFF));
      m_bitbuffer = code >> 8;
      m_bitswaiting = true;
   }
   else {
      unsigned char bits;
      bits = ((code >> 4) & 0xF0) | m_bitbuffer;
      stream.put(bits);
      stream.put((unsigned char)(code & 0xFF));
      m_bitswaiting = false;
   }

   return stream;
}

///////////////////////////////////////////////////////////////////////////////

// } // namespace paftl

#endif
