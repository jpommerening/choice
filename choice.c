#include "choice.h"
#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

static int prefixcmp( const char* target, const char* prefix ) {
  char t, p;
  int i;
  while( (t = *target) != '\0' && (p = *prefix) != '\0' && t == p ) {
    target++;
    prefix++;
  };
  if( p == '\0' ) {
    for( i=0; *target++ != '\0'; i++ );
    return i;
  } else {
    for( i=0; *prefix++ != '\0'; i-- );
    return i;
  }
}

#include <string.h>
#include <stdlib.h>
static int levenshtein( const char *string1, const char *string2,
                        int w, int s, int a, int d ) {
  int len1 = strlen(string1), len2 = strlen(string2);
  int *row0 = calloc(sizeof(int), (len2 + 1));
  int *row1 = calloc(sizeof(int), (len2 + 1));
  int *row2 = calloc(sizeof(int), (len2 + 1));
  int i, j;

  for( j = 0; j <= len2; j++ ) row1[j] = j * a;
  for( i = 0; i < len1; i++ ) {
    int *dummy;

    row2[0] = (i + 1) * d;
    for( j = 0; j < len2; j++ ) {
      /* substitution */
      row2[j + 1] = row1[j] + s * (string1[i] != string2[j]);
      /* swap */
      if( i > 0 && j > 0 && string1[i - 1] == string2[j] &&
          string1[i] == string2[j - 1] &&
          row2[j + 1] > row0[j - 1] + w )
        row2[j + 1] = row0[j - 1] + w;
      /* deletion */
      if( row2[j + 1] > row1[j + 1] + d )
        row2[j + 1] = row1[j + 1] + d;
      /* insertion */
      if( row2[j + 1] > row2[j] + a )
        row2[j + 1] = row2[j] + a;
    }

    dummy = row0;
    row0 = row1;
    row1 = row2;
    row2 = dummy;
  }

  i = row1[len2];
  free(row0);
  free(row1);
  free(row2);

  if( i > len2 )
    return -1;
  return i;
}
static int fuzzycmp(const char* target, const char* str) {
  if( *target != *str )
    return -1;
  return levenshtein(str, target, 2, 8, 1, 6);
}

#define FUZZY_MAX 10
static option_t* option_by_name( option_t* options, const char* str ) {
  option_t* option = NULL;
  int val, max = FUZZY_MAX + 1;
  while( options->name != NULL || options->abbr != '\0' ) {
    if( options->name == NULL ) {
      options++;
      continue;
    }
    val = fuzzycmp(options->name, str);
    if( val >= 0 && val <= FUZZY_MAX ) {
      if( val < max ) {
        max = val;
        option = options;
      } else if( val == max ) {
        /* ambiguous */
        fprintf( stderr, "ambiguous option --%s, could be --%s, --%s\n", str, option->name, options->name );
        return NULL;
      }
    }
    options++;
  }
  return option;
}
static option_t* option_by_abbr( option_t* options, char c ) {
  while( options->name != NULL || options->abbr != '\0' ) {
    if( options->abbr != '\0' && options->abbr == c ) {
      return options;
    }
    options++;
  }
  return NULL;
}

#ifndef _GNU_SOURCE
static char* strchrnul( char* str, int c ) {
  int s = *str;
  while( s != c && s != '\0' ) s = *(++str);
  return str;
}
#endif

static char* arg_shiftstr( command_t* command ) {
  char* str = command->argv[0];
  command->argc -= 1;
  command->argv = &(command->argv[1]);
  return str;
}
static char arg_shiftc( command_t* command ) {
  char c = command->argv[0][0];
  command->argv[0] += 1;
  return c;
}
static char* arg_shiftstrchr( command_t* command, char c ) {
  char* str = command->argv[0];
  command->argv[0] = strchrnul( str, c );
  return str;
}

int option_parse( option_t* options, int argc, char* argv[] ) {
  command_t command = { options, argv[0], argc - 1, &(argv[1]) };
  option_t* option = NULL;

#define S_ANY 0
#define S_ARG 1
#define S_ABBR 2
#define S_NAME 3
#define S_DONE 4
#define ARG (command.argv[0])

  char* name;
  char abbr;
  char* arg;
  int state = S_ANY;

  while( command.argc > 0 ) {

    switch( state ) {
      case S_ANY:
        if( ARG[0] == '-' ) {
          if( option ) {
            (option->callback)( option, NULL );
            option = NULL;
          }
          if( ARG[1] == '-' ) {
            ARG += 2;
            state = S_NAME;
          } else if( ARG[1] != '\0' ) {
            ARG += 1;
            state = S_ABBR;
          }
          break;
        }
      case S_ARG:
        if( option == NULL ) {
          /* plain string option */
          arg = arg_shiftstr( &command );
          fprintf( stderr, "plain %s!\n", arg );
          return;
        } else {
          arg = arg_shiftstr( &command );
          (option->callback)( option, arg );
          option = NULL;
          state = S_ANY;
        }
        break;
      case S_ABBR:
        abbr = arg_shiftc( &command );
        if( abbr == '\0' ) {
          /* end of abbreviated option list */
          arg_shiftstr( &command );
          state = S_ANY;
        } else {
          option = option_by_abbr( command.options, abbr );
          if( option == NULL ) {
            /* unknown option */
            fprintf( stderr, "unknown option -%c!\n", abbr );
            return;
          }
          if( option->flags & OPTION_ARG ) {
            arg = arg_shiftstr( &command );
            if( arg[0] != '\0' ) {
              (option->callback)( option, arg );
              option = NULL;
              state = S_ANY;
            } else {
              state = (option->flags & OPTION_REQARG) ? S_ARG : S_ANY;
            }
          } else {
            (option->callback)( option, NULL );
            option = NULL;
            state = S_ABBR;
          }
        }
        break;
      case S_NAME:
        name = arg_shiftstrchr( &command, '=' );
        arg  = arg_shiftstr( &command );
        if( arg[0] == '=' ) *arg++ = '\0';
        if( *name == '\0' ) {
          /* "--" */
          arg_shiftstr( &command );
          state = S_DONE;
        } else {
          option = option_by_name( command.options, name );
          if( option == NULL ) {
            /* unknown option */
            fprintf( stderr, "unknown option --%s!\n", name );
            return;
          }
          if( option->flags & OPTION_ARG ) {
            if( arg[0] != '\0' ) {
              (option->callback)( option, arg );
              option = NULL;
              state = S_ANY;
            } else {
              state = (option->flags & OPTION_REQARG) ? S_ARG : S_ANY;
            }
          } else if( arg[0] != '\0' ) {
            /* does not take args */
            fprintf( stderr, "option --%s does not take parameters (%s)!\n", option->name, arg );
            return;
          } else {
            (option->callback)( option, NULL );
            option = NULL;
            state = S_ANY;
          }
        }
        break;
      case S_DONE:
        return;
    }
  }

#undef S_ANY
#undef S_ARG
#undef S_ABBR
#undef S_NAME
#undef S_DONE
#undef ARG
}

int subopt_parse( option_t* options, char* argv ) {
  option_t* option = NULL;

  char* name;
  char* arg;

  while( *argv != '\0' ) {
    name = argv;
    argv = strchrnul( argv, ',' );
    if( argv[0] == ',' ) *argv++ = '\0';
    arg  = strchrnul( name, '=' );
    if( arg[0] == '=' ) *arg++ = '\0';

    option = option_by_name( options, name );
    if( option == NULL ) {
      /* unknown option */
      return;
    }
    if( option->flags & OPTION_ARG ) {
      if( arg[0] != '\0' ) {
        (option->callback)( option, arg );
        option = NULL;
      } else {
        /* requires arg */
        return ;
      }
    } else if( arg[0] != '\0' ) {
      /* does not take args */
      return;
    } else {
      (option->callback)( option, NULL );
      option = NULL;
    }
  }

}

int option_true( option_t* option, const char* arg ) {
  if( option->data == NULL ) return 0;
  *((bool*) option->data) = true;
}

int option_false( option_t* option, const char* arg ) {
  if( option->data == NULL ) return 0;
  *((bool*) option->data) = false;
}

int option_long( option_t* option, const char* arg ) {
  if( option->data == NULL ) return 0;
  *((long*) option->data) = (arg == NULL) ? 0 : atol(arg);
}

int option_str( option_t* option, const char* arg ) {
  if( option->data == NULL ) return 0;
  *((const char**) option->data) = arg;
}

int option_log( option_t* option, const char* arg ) {
  if( option->name != NULL && option->abbr != '\0' )
    printf( " -%c, --%-10s", option->abbr, option->name );
  else if( option->name != NULL )
    printf( "     --%-10s", option->name );
  else if( option->abbr != '\0' )
    printf( " -%c, %12s", option->abbr, "" );

  if( arg )
    printf( " -> \"%s\"\n", arg );
  else
    printf( "\n" );
}

int option_subopt( option_t* option, const char* arg ) {
  char * str = arg;
  subopt_parse( option->data, str );
  return 0;
}

/*
static option_t options[] = {
  { "bool", "boolean option", 'b', 0, &option_log, NULL },
  { "string", "string option", 's', OPTION_REQARG, &option_log, NULL },
  { "strong", "strong option", '\0', OPTION_OPTARG, &option_log, NULL },
  OPTION_EOL
};

int main( int argc, char* argv[] ) {
  printf( "name, name, %i (expect 0)\n",  prefixcmp( "name", "name" ) );
  printf( "name, namee, %i (expect -1)\n", prefixcmp( "name", "namee" ) );
  printf( "name, nam, %i (expect 1)\n",   prefixcmp( "name", "nam" ) );
  printf( "name, namoo, %i (expect -2)\n",   prefixcmp( "name", "namoo" ) );

  parse( options, argc, argv );
  return 0;
}
*/


static struct {
  bool verbose;
  const char* required;
  long optional;
  bool write;
  long bsize;
} config = {
  false,
  NULL,
  0,
  true,
  1024
};

static option_t subopts[] = {
  { "rw", "aertgaert", '\0', 0, &option_true, &config.write },
  { "ro", "aertgaert", '\0', 0, &option_false, &config.write },
  { "bs", "aertgaert", '\0', OPTION_REQARG, &option_long, &config.bsize },
  OPTION_EOL
};

static option_t options[] = {
  { "verbose", "enable verbose stuff", 'v', 0, &option_true, &config.verbose },
  { "required", "required arg", 'r', OPTION_REQARG, &option_str, &config.required },
  { "optional", "optional arg", 'o', OPTION_OPTARG, &option_long, &config.optional },
  { "subopt", "++++", 's', OPTION_REQARG, &option_subopt, &(subopts[0]) },
  OPTION_EOL
};

int main( int argc, char* argv[] ) {
  option_parse( options, argc, argv );
  printf( "verbose: %s, required: %s, optional: %li, write: %s, bsize: %li\n",
          config.verbose ? "true" : "false",
          config.required,
          config.optional,
          config.write ? "true" : "false",
          config.bsize );
  return 0;
}
