#include <iostream>
#include <sparsehash/sparse_hash_map>

using google::sparse_hash_map;      // namespace where class lives by default
using namespace std;

// Custom hash function for int.
struct MyHash {
  size_t operator()(const int& x) const {
    // A simple hash function.
    return x * 2654435761;
  }
};

// Custom hash function for C string.
struct MyStringHash {
  size_t operator()(const char* str) const {
    // A simple hash function.
    return (size_t)*str;
  }
};

// Custom equivalence function for C string.
struct MyStringEqual {
  bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) == 0;
  }
};

int main() {
  sparse_hash_map<int, const char*, MyHash> mymap;
  mymap.set_deleted_key(-1);    // important for sparsehash
  mymap[1] = "one";
  mymap[2] = "two";
  mymap[3] = "three";
  mymap[4] = "four";
  mymap[5] = "five";
  mymap[6] = "six";
  mymap[7] = "seven";
  mymap[8] = "eight";
  mymap[9] = "nine";
  mymap[10] = "ten";

  // Iterate over all the elements in the hash table.
  sparse_hash_map<int, const char*, MyHash>::iterator it = mymap.begin();
  for (; it != mymap.end(); ++it) {
    cout << it->first << " " << it->second << endl;
  }

  // Find the value associated with the key 1.
  it = mymap.find(1);
  if (it != mymap.end()) {
    cout << "Found the value " << it->second << " for key " << it->first << endl;
  }

  // Erase the element with key 2.
  mymap.erase(2);

  // Insert a new element.
  mymap.insert(make_pair(11, "eleven"));

  // Check if the key 2 is present.
  if (mymap.find(2) == mymap.end()) {
    cout << "Key 2 is not found." << endl;
  }

  // Use a custom hash function for C string.
  sparse_hash_map<const char*, int, MyStringHash, MyStringEqual> mymap2;
  mymap2["hello"] = 1;
  mymap2["world"] = 2;

  // Iterate over all the elements in the hash table.
  sparse_hash_map<const char*, int, MyStringHash, MyStringEqual>::iterator it2 = mymap2.begin();
  for (; it2 != mymap2.end(); ++it2) {
    cout << it2->first << " " << it2->second << endl;
  }
  sparse_hash_map<int, uint64_t> h5fdmap;
  h5fdmap.set_deleted_key(-1);    // important for sparsehash
  h5fdmap[1] = 12345; 
  h5fdmap[2] = 67890; 
  h5fdmap[5] = 68650; 
  sparse_hash_map<int, uint64_t>::iterator fit = h5fdmap.begin();
  for (; fit != h5fdmap.end(); ++fit) {
    cout << fit->first << " " << fit->second << endl;
  }
  h5fdmap.erase(2);
  for (fit = h5fdmap.begin(); fit != h5fdmap.end(); ++fit) {
    cout << fit->first << " " << fit->second << endl;
  }
 
  return 0;
}
