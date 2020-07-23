#ifndef MY_PINBALL_H
#define MY_PINBALL_H

#include <TraceLog.h>

#define MAX_COMMANDS 50
#define STRING_SIZE 80
#define NO_STATE -1

enum CommandType
{
  TRANSIENT,
  PERSISTENT
};

struct Command
{
  char name[STRING_SIZE];
  CommandType type;
  int id;
};

struct State
{
  char name[STRING_SIZE];
};

// define some program specific trace levels
#define TL_EVENT    TL_MAX_LEVELS+1
#define TL_COMMAND  TL_MAX_LEVELS+2
#define TL_RULE  TL_MAX_LEVELS+3

// define some program specific trace macros
#define TRACE_EVENT(format, args...) __TRACE(TL_EVENT, "EVENT", format, ## args)
#define TRACE_COMMAND(format, args...) __TRACE(TL_COMMAND, "COMMAND", format, ## args)
#define TRACE_RULE(format, args...) __TRACE(TL_RULE, "RULE", format, ## args)

#endif
