#ifndef __DZ_MQ_H__
#define __DZ_MQ_H__

int dz_openmq(int port);
int dz_closemq();
int dz_notifymq(const char *path);

#endif 
