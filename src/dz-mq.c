#include "dzfile.h"
#include <string.h>
#include <zmq.h>

int
dz_openmq(int port)
{

  int rc;
  char mqurl[128] = { 0 };
  struct dzfile_priv* priv = get_priv();
  if (!priv)
    return -1;
  priv->mqcontext = zmq_ctx_new();
  if (!priv->mqcontext)
    return -1;
  priv->mqsocket = zmq_socket(priv->mqcontext, ZMQ_PUSH);
  if (!priv->mqsocket)
    return -1;
  int timeout = 1;
  sprintf(mqurl, "tcp://localhost:%d", port);
  rc = zmq_connect(priv->mqsocket, mqurl);
  if (rc == -1) {
    dz_msg(DZ_LOG_ERROR,
           "connect mqserver '%s' failed with %d\n",
           mqurl,
           zmq_errno());
    return -1;
  }
  return 0;
}
int
dz_closemq()
{
  struct dzfile_priv* priv = get_priv();
  if (!priv)
    return -1;
  if (priv->mqsocket)
    zmq_close(priv->mqsocket);
  if (priv->mqcontext)
    zmq_ctx_destroy(priv->mqcontext);
  return 0;
}
int
dz_notifymq(const char* path)
{
  struct dzfile_priv* priv = get_priv();
  int rc;
  if (!priv)
    return -1;
  rc = zmq_send(priv->mqsocket, path, strlen(path), 0);
  return rc;
}
