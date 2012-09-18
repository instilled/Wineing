
#ifndef _UTLIS_H
#define _UTLIS_H

/*
 * For valid flags see http://api.zeromq.org/2-2:zmq-recv
 */
static char * s_recv (void *socket, int flag);

#define CMD_OPTIONS_FLAG_REQUIRED          0x1
#define CMD_OPTIONS_FLAG_OPTION            0x2
#define CMD_OPTIONS_FLAG_OPTION_REQUIRED   0x4

typedef struct
{
  int key;
  const char *name;
  const char *argName;
  int flags;
  char *text;
} cmd_options;

void cmd_print_help(cmd_options ops[])

void cmd_parser(cmd_options ops[], int argc, char *argv[])
{
  for(int i = 0; i < argc; i++) {

    int opsI = 0;
    while(ops[opsI++]) {
    }
  }

}


#endif /* _UTLIS_H */
