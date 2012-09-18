

#include "utils.h"

static char* s_recv(void *socket, int flag)
{
  zmq_msg_t message;
  zmq_msg_init (&message);
  zmq_recv (socket, &message, flag);
  int size = zmq_msg_size (&message);
  char *string = malloc (size + 1);
  memcpy (string, zmq_msg_data (&message), size);
  zmq_msg_close (&message);
  string [size] = 0;
  return (string);
}


