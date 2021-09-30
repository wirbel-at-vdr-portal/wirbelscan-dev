/*******************************************************************************
 * wirbelscan: A plugin for the Video Disk Recorder
 * See the README file for copyright information and how to reach the author.
 ******************************************************************************/
#pragma once
#include <vector>
#include <mutex>
#include <algorithm>


/*******************************************************************************
 * returns true if Item1 should be sorted before Item2 in a TList.
 ******************************************************************************/

typedef bool (*TListSortCompare)(void* Item1, void* Item2);


/*******************************************************************************
 * class TList
 ******************************************************************************/

template<class T> class TList {
private:
  std::mutex m;
protected:
  std::vector<T> v;
public:
  TList(void) {}                                         // constructor
  TList(const TList<T>& other) : v(other.v) {}           // non-swap copy constructor
  ~TList() { v.clear(); }

  void Add(T p) {                                        // Add a new item to the list.
     const std::lock_guard<std::mutex> lock(m);
     v.push_back(p);
     if (v.size() == v.capacity())
        v.reserve(v.capacity() << 1);
     }

  int Capacity(void) {                                   // returns max number of items
     const std::lock_guard<std::mutex> lock(m);
     return v.capacity();
     }

  void Capacity(size_t newCap) {                         // sets max number of items
     const std::lock_guard<std::mutex> lock(m);
     v.reserve(newCap);
     }

  void Clear(void) {                                     // Clears the list.
     const std::lock_guard<std::mutex> lock(m);
     v.clear();
     }

  int Count(void) {                                      // Current number of items.
     const std::lock_guard<std::mutex> lock(m);
     return v.size();
     }

  void Delete(size_t Index) {                            // Removes items from list.
     const std::lock_guard<std::mutex> lock(m);
     v.erase(v.begin()+Index);
     }

  void Exchange(size_t Index1, size_t Index2) {          // Exchanges two items
     const std::lock_guard<std::mutex> lock(m);
     T p1 = v[Index1];
     T p2 = v[Index2];
     v[Index1] = p2;
     v[Index2] = p1;
     }

  TList<T> Expand(void) {                                // Increases the capacity of the list if needed.                 
     const std::lock_guard<std::mutex> lock(m);
     if (v.size() == v.capacity())
        v.resize(v.capacity() << 1);
     return *this;
     }

  T First(void) {                                       // Returns the first non-nil pointer in the list.
     const std::lock_guard<std::mutex> lock(m);
     return v.front();
     }

  T Last(void)  {                                       // Returns the last non-nil pointer in the list.
     const std::lock_guard<std::mutex> lock(m);
     return v.back();
     }

  int IndexOf(T Item) {                                 // Returns the index of a given item.
     const std::lock_guard<std::mutex> lock(m);
     for(size_t i=0; i<v.size(); i++) {
        if (v[i] == Item)
           return i;
        }
     return -1;
     }

  void Insert(size_t Index, T Item) {                   // Inserts a new pointer in the list at a given position.
     const std::lock_guard<std::mutex> lock(m);
     v.insert(v.begin() + Index , Item);
     }

  void Move(size_t CurIndex, size_t NewIndex) {         // Moves a pointer from one position in the list to another.
     if (CurIndex == NewIndex)
        return;

     const std::lock_guard<std::mutex> lock(m);
     T& item = v[CurIndex];
     Delete(CurIndex);

     if (CurIndex < NewIndex)
        Insert(NewIndex-1, item);
     else
        Insert(NewIndex  , item);
     }

  T& operator[](int const& Index) {                     // Provides access to Items (pointers) in the list.
     const std::lock_guard<std::mutex> lock(m);
     return v[Index];
     }

  TList<T>& operator=(const TList<T>& other) {          // copy assignment operator.
     if (this != &other) {
        const std::lock_guard<std::mutex> lock(m);
        v = other.v;
        }
     return *this;
     }

  T& Items(size_t const& Index) {
     const std::lock_guard<std::mutex> lock(m);
     return v[Index];
     }

  int Remove(T Item) {                                  // Removes a value from the list & returns it's index before removal
     const std::lock_guard<std::mutex> lock(m);
     for(size_t i=0; i<v.size(); i++) {
        if (v[i] == Item) {
           v.erase(v.begin()+i);
           return i;
           }
        }
     return -1;
     }

  void Sort(TListSortCompare Compare) {                 // Sorts the items in the list using a function.
     const std::lock_guard<std::mutex> lock(m);
     std::sort(v.begin(), v.end(), Compare);
     }

  void Sort(void) {                                     // Sorts the items in the list using operator '<'.
     const std::lock_guard<std::mutex> lock(m);
     std::sort(v.begin(), v.end());
     }

  void Assign(TList<T>& from) {                         // Copy the contents of other lists.
     const std::lock_guard<std::mutex> lock(m);
     v.assign(from.v.begin(), from.v.end());
     }

  void AddList(TList<T>& aList) {                       // Add all items from another list
     const std::lock_guard<std::mutex> lock(m);
     v.insert(v.end(),aList.v.begin(),aList.v.end());
     }

  T* List(void) {                                       // Returns the items in an array.
     const std::lock_guard<std::mutex> lock(m);
     return v.data();
     }

  void Pack(void) {                                     // Removes nullptr's from the list and frees unused memory.
     const std::lock_guard<std::mutex> lock(m);
     v.shrink_to_fit();
     }
};
