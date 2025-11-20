#ifndef __SPASEHASH_MAP__H__
#define __SPASEHASH_MAP__H__

extern "C"
{
  void hashmap_init();
  int add_dzfd(int fd, dzfd_t* dzfd);
  int remove_dzfd(int fd);
  dzfd_t *find_dzfd(int fd);
}



#endif
