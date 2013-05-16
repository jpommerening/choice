#ifndef _CHOICE_H_
#define _CHOICE_H_

#define OPTION_EINVAL 1  /* invalid option */
#define OPTION_ENOARG 2  /* option has no argument */
#define OPTION_EREQARG 3 /* option requires an argument */
#define OPTION_EONCE 4   /* option already seen */
#define OPTION_EAMBIG 5  /* option is ambiguous */

typedef enum {
  OPTION_REQARG = 1,
  OPTION_OPTARG = 2,
  OPTION_ARG = 3,
  OPTION_CALLED = 4,
  OPTION_MULTIPLE = 8
} option_flag_t;

typedef struct option_s option_t;

typedef int (*option_cb)( option_t* option, const char* arg );

struct option_s {
  const char* name;
  const char* desc;
  const char abbr;
  unsigned flags;
  option_cb callback;
  void* data;
};

extern int option_true( option_t* option, const char* arg );
extern int option_false( option_t* option, const char* arg );
extern int option_long( option_t* option, const char* arg );
extern int option_str( option_t* option, const char* arg );
extern int option_log( option_t* option, const char* arg );
extern int option_help( option_t* option, const char* arg );
extern int option_subopt( option_t* option, const char* arg );

#define OPTION_TRUE(name, desc, abbr, bool_var) \
  { name, desc, abbr, 0, option_true, &bool_var }
#define OPTION_FALSE(name, desc, abbr, bool_var) \
  { name, desc, abbr, 0, option_false, &bool_var }
#define OPTION_LONG(name, desc, abbr, long_var) \
  { name, desc, abbr, OPTION_REQARG, option_long, &long_var }
#define OPTION_STR(name, desc, abbr, str_var) \
  { name, desc, abbr, OPTION_REQARG, option_str, &str_var }
#define OPTION_SUBOPT(name, desc, abbr, opts) \
  { name, desc, abbr, OPTION_REQARG, option_subopt, &(opts[0]) }
#define OPTION_EOL \
  { NULL, NULL, '\0', 0, NULL, NULL }

extern int option_parse( option_t* options, int argc, char* argv[] );
extern int subopt_parse( option_t* options, char *argv );

#endif
