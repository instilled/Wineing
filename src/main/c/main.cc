
#include "wineing.h"

void cmd_parse(int, char**, WineingConf &);

int main(int argc, char** argv)
{
  // Interrupt ctrl-c and kill and do proper shutdown
  // http://www.cs.cf.ac.uk/Dave/C/node24.html
  //signal(SIGINT, sigproc);
  //signal(SIGQUIT, sigproc);

  WineingConf conf;

  conf.schan_fqcn   = DEFAULTS_SCHAN_NAME;
  conf.cchan_fqcn   = DEFAULTS_CCHAN_NAME;
  conf.mchan_fqcn   = DEFAULTS_MCHAN_NAME;
  conf.tape_basedir = DEFAULTS_TAPE_BASE_DIR;

  cmd_parse(argc, argv, conf);

  log(LOG_INFO, "Starting Wineing");

  // Be nice and let Linux users know that we are running a windows
  // application! HAHAHAHA!
  //MessageBox(NULL, "I'm Wineing!", "", MB_OK);

  WineingCtx ctx;
  ctx.conf = &conf;

  wineing_init(ctx);
  wineing_run(ctx);
  wineing_shutdown(ctx);

  return 0;
}

void cmd_print_usage()
{
  printf("Usage: wineing.exe "
         "--cchan=<fqcn> "
         "--mchan=<fqcn> "
         "[--schan=<fqcn>] "
         "[--tape-root=<dir>]\n\n");

  printf("Wineing TBD.\n\n");
  printf("ZMQ channels:\n");
  printf("  --cchan          Incoming control messages channel (binds to a\n");
  printf("                   ZMQ PUSH socket)\n");
  printf("  --mchan          Market data channel (binds to a ZMQ PUB socket\n");
  printf("                   socket)\n");
  printf("  [--schan]        Optional. Sync channel (synchronous ZMQ REQ/REP\n");
  printf("                   socket)\n");
  printf("                   Defaults to 'tcp://localhost:9991'\n\n");
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

void cmd_parse(int argc, char** argv, WineingConf &conf)
{
  int allOpts = 0;
  for(int i = 1; i < argc; i++) {
    switch(argv[i][2])
      {
      case 's':
        conf.schan_fqcn = cmd_parse_opt(argv[i]);
        break;

      case 'c':
        conf.cchan_fqcn = cmd_parse_opt(argv[i]);
        allOpts |= 0x1;
        break;

      case 'm':
        conf.mchan_fqcn = cmd_parse_opt(argv[i]);
        allOpts |= 0x2;
        break;

      case 't':
        conf.tape_basedir = cmd_parse_opt(argv[i]);
        break;
      }
  }

  if(allOpts != 3) {
    cmd_print_usage();
    exit(1);
  }
}
