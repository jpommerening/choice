#ifndef _CHOICE_H_
#define _CHOICE_H_

#define CHC_EINVAL 1  /* invalid option */
#define CHC_ENOARG 2  /* option has no argument */
#define CHC_EREQARG 3 /* option requres an argument */
#define CHC_EONCE 4   /* option already seen */
#define CHC_EAMBGS 5  /* option is ambigous */

typedef enum {
  OPTION_REQARG = 1,
  OPTION_OPTARG = 2,
  OPTION_ARG = 3,
  OPTION_CALLED = 4,
  OPTION_MULTIPLE = 8
} option_flag_t;

typedef struct option_s option_t;
typedef struct command_s command_t;

typedef int (*option_cb)( option_t* option, const char* arg );

struct option_s {
  const char* name;
  const char* desc;
  const char abbr;
  unsigned flags;
  option_cb callback;
  void* data;
};

struct command_s {
  option_t* options;
  const char* name;
  int argc;
  char** argv;
};

extern int option_true( option_t* option, const char* arg );
extern int option_false( option_t* option, const char* arg );
extern int option_long( option_t* option, const char* arg );
extern int option_str( option_t* option, const char* arg );
extern int option_log( option_t* option, const char* arg );

#define OPTION_TRUE(name, desc, abbr, bool_var) \
  { name, desc, abbr, 0, option_true, &bool_var }
#define OPTION_FALSE(name, desc, abbr, bool_var) \
  { name, desc, abbr, 0, option_false, &bool_var }
#define OPTION_LONG(name, desc, abbr, long_var) \
  { name, desc, abbr, OPTION_REQARG, option_long, &long_var }
#define OPTION_STR(name, desc, abbr, str_var) \
  { name, desc, abbr, OPTION_REQARG, option_str, &str_var }
#define OPTION_EOL \
  { NULL, NULL, '\0', 0, NULL, NULL }

extern int option_parse( option_t* options, int argc, char* argv[] );
extern int subopt_parse( option_t* options, char *argv );

#endif
