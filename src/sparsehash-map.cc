#include <iostream>
#include <sparsehash/sparse_hash_map>
#include "dzfile-intern.h"
/*
extern "C"
{
  void hashmap_init();
  int add_dzfd(int fd, dzfd_t* dzfd);
  int remove_dzfd(int fd);
  dzfd_t *find_dzfd(int fd);
}
*/
using google::sparse_hash_map;      // namespace where class lives by default
using namespace std;
sparse_hash_map<int, uint64_t> h5fdmap;
void hashmap_init()
{
  h5fdmap.set_deleted_key(-1);
}
int add_dzfd(int fd, dzfd_t *dzfd)
{
  if (!dzfd)
    return -1;
  h5fdmap[fd] = (uint64_t)dzfd;
  return 0;
}
int remove_dzfd(int fd)
{
  h5fdmap.erase(fd);
  return 0;
}
dzfd_t *find_dzfd(int fd)
{
  uint64_t value;
  if (h5fdmap.find(fd) == h5fdmap.end()) {
    return NULL;
  }
  value = h5fdmap[fd];
  return (dzfd_t *)value;
}
