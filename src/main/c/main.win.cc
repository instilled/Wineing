
#include "wineing.win.h"
#include <stdlib.h>
#include <string.h>

void cmd_parse(int, char**, w_conf &);

int main(int argc, char** argv)
{
  // Interrupt ctrl-c and kill and do proper shutdown
  // http://www.cs.cf.ac.uk/Dave/C/node24.html
  //signal(SIGINT, sigproc);
  //signal(SIGQUIT, sigproc);

  w_conf conf;

  conf.cchan_in_fqcn  = DEFAULTS_CCHAN_IN_NAME;
  conf.cchan_out_fqcn = DEFAULTS_CCHAN_OUT_NAME;
  conf.mchan_fqcn     = DEFAULTS_MCHAN_NAME;
  conf.tape_basedir   = DEFAULTS_TAPE_BASE_DIR;

  cmd_parse(argc, argv, conf);

  log(LOG_INFO, "Starting Wineing");

  // Be nice and let Linux users know that we are running a windows
  // application! HAHAHAHA!
  //MessageBox(NULL, "I'm Wineing!", "", MB_OK);

  w_ctx ctx;
  ctx.conf = &conf;

  wineing_init(ctx);
  wineing_run(ctx);
  wineing_shutdown(ctx);

  return 0;
}

void cmd_print_usage()
{
  printf("Usage: wineing.exe "
         "--cchan-in=<fqcn> "
         "--cchan-out=<fqcn> "
         "--mchan=<fqcn> "
         "[--tape-root=<dir>]\n\n");

  printf("Wineing TBD.\n\n");
  printf("ZMQ channels:\n");
  printf("  --cchan-in       Incoming control messages channel (binds to a\n");
  printf("                   ZMQ PULL socket)\n");
  printf("  --cchan-out      Outgoing control messages channel (binds to a\n");
  printf("                   ZMQ PUSH socket)\n");
  printf("  --mchan          Market data channel (binds to a ZMQ PUB socket\n");
  printf("                   socket)\n");
  printf("NxCore related options:\n");
  printf("  [--tape-root]    The directory from which to serve the tape files\n");
  printf("                   Defaults to 'C:\\md\\'. The path has to end "
                             "in '\\'\n");
}

char * cmd_parse_opt(char *argv)
{
  int eq = strcspn (argv, "=");
  return &argv[eq+1];
}

void cmd_parse(int argc, char** argv, w_conf &conf)
{
  int allOpts = 0;
  for(int i = 1; i < argc; i++) {
    switch(argv[i][2])
      {
      case 'c':
        if(argv[i][8] == 'i') {
          conf.cchan_in_fqcn = cmd_parse_opt(argv[i]);
          allOpts |= 0x1;
        } else {
          conf.cchan_out_fqcn = cmd_parse_opt(argv[i]);
          allOpts |= 0x2;
        }
        break;

      case 'm':
        conf.mchan_fqcn = cmd_parse_opt(argv[i]);
        allOpts |= 0x4;
        break;

      case 't':
        conf.tape_basedir = cmd_parse_opt(argv[i]);
        break;
      }
  }

  if(allOpts != 7) {
    cmd_print_usage();
    exit(1);
  }
}
