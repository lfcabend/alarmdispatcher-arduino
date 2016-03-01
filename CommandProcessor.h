#define BUFF_MAX 20

struct Command {
  char recv[BUFF_MAX];
  unsigned int recv_size = 0;
  boolean readMessage = false;
};
