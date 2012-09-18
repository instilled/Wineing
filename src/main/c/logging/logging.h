#ifndef _LOGGING_H
#define _LOGGING_H

#include <time.h>
#include <stdio.h>

#define LOG_DEST stdout
#define LOG_FORMAT_TIME "%Y-%m-%d %H:%M:%S"

#define LOG_ALL     0
#define LOG_DEBUG   1
#define LOG_INFO    2
#define LOG_ERROR   3
#define LOG_NONE    4

static const char *LOG_STR[5] = {"ALL",
                                 "DEBUG",
                                 "INFO",
                                 "ERROR",
                                 "NONE"};

#if defined DEBUG && !defined LOG_LEVEL
   #define LOG_LEVEL LOG_DEBUG
#elif !(defined LOG_LEVEL)
   #define LOG_LEVEL LOG_ALL
#endif

// TODO: Calling fprintf too many times

// TODO: Provide option to disable logging completely? > define empty
// macro

//#if LOG_LEVEL == LOG_NONE
//  #define log(level, msg) ()
//#else
#define log(level, msg, args...) {                                      \
  if(level > LOG_LEVEL - 1) {                                           \
    char *buffer = new char[25];                                        \
    time_t now;                                                         \
    time(&now);                                                         \
    struct tm *mytime = localtime(&now);                                \
    strftime(buffer, 25, LOG_FORMAT_TIME, mytime);                      \
    fprintf(LOG_DEST,                                                   \
            "%s: [%-5s] ",                                              \
            buffer,                                                     \
            LOG_STR[level]);                                            \
    fprintf(LOG_DEST,                                                   \
            msg,                                                        \
            ## args);                                                   \
    fprintf(LOG_DEST,                                                   \
            " (%s:%d)\n",                                               \
            __FILE__,                                                   \
            __LINE__);                                                  \
    delete buffer;                                                      \
  } }
//#endif

#endif /* _LOGGING_H */
