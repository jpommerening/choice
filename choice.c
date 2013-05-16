/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */
/* vim:set softtabstop=2 shiftwidth=2: */
/*
 * choice -- the dyslexic option parser
 *
 * Latest source-code available at:
 *    https://github.com/jpommerening/choice
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Jonas Pommerening <jonas.pommerening@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE, DATA, OR PROFITS;  OR BUSINESS
 * INTERRUPTION)  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * ---------------------------------------------------------------------------
 *
 * Todo-List:
 *   [x] bells
 *   [x] whistles
 *   [ ] proper error handling (return codes, callbacks)
 *   [ ] help-printing for subopts
 *   [ ] completion support
 *   [ ] keep looking after first "non-option"
 *   [ ] refactor parser loop
 */
#include "choice.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

/* MARK: option callbacks *//**
 * @name option callbacks
 * @{
 */

/** Store `true` in variable pointed to by `option->data` */
int option_true( option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((bool*) option->data) = true;
  return 0;
}

/** Store `false` in variable pointed to by `option->data` */
int option_false( option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((bool*) option->data) = false;
  return 0;
}

/** Store a `long` number in variable pointed to by `option->data` */
int option_long( option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((long*) option->data) = (arg == NULL) ? 0 : atol(arg);
  return 0;
}

/** Store `arg` unmodified in variable pointed to by `option->data` */
int option_str( option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((const char**) option->data) = arg;
  return 0;
}

/** Write a line to `stdout` */
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
  return 0;
}

/**
 * Print the given format and indent for the next columns,
 * Print a new line plus the indent if the formatted string
 * is longer than the desired indent.
 */
static void pindent( const char* fmt, ... ) {
  char str[26];
  int len;
  va_list vargs, vargs2;
  va_start(vargs, fmt);
  va_copy(vargs2, vargs);
  len = vsnprintf(&(str[0]), sizeof(str), fmt, vargs );

  if( len >= sizeof(str)-1 ) {
    printf( "  " );
    vprintf( fmt, vargs2 );
    printf( "\n  %-28s", "" );
  } else {
    printf( "  %-28s", str );
  }
  va_end(vargs);
}

/** Print the classic option help, that we've come to expect from UNIX programs */
int option_help( option_t* option, const char* arg ) {
  bool reqarg = option->flags & OPTION_REQARG;
  bool optarg = option->flags & OPTION_OPTARG;
  if( arg == NULL ) arg = "arg";

  if( option->name != NULL && option->abbr != '\0' ) {
    if( reqarg ) {
      pindent( "-%c, --%s=<%s>", option->abbr, option->name, arg );
    } else if( optarg ) {
      pindent( "-%c, --%s [%s]", option->abbr, option->name, arg );
    } else {
      pindent( "-%c, --%s", option->abbr, option->name, arg );
    }
  } else if( option->name != NULL ) {
    if( reqarg ) {
      pindent( "    --%s=<%s>", option->name, arg );
    } else if( optarg ) {
      pindent( "    --%s [%s]", option->name, arg );
    } else {
      pindent( "    --%s", option->name );
    }
  } else if( option->abbr != '\0' ) {
    if( reqarg ) {
      pindent( "-%c <%s>", option->abbr, arg );
    } else if( optarg ) {
      pindent( "-%c [%s]", option->abbr, arg );
    } else {
      pindent( "-%c", option->abbr );
    }
  }

  printf( "%s\n", option->desc );
  return 0;
}

/**
 * Parse the suboptions in the given `arg`.
 * See `subopt_parse` for more info.
 */
int option_subopt( option_t* option, const char* arg ) {
  char* str = (char*)arg; /* Trust me, it's going to be fine. */
  return subopt_parse( option->data, str );
}

/** @} */

/* MARK: comparators *//**
 * @name comparators
 * @{
 */

static int levenshtein( const char *string1, size_t len1,
                        const char *string2, size_t len2,
                        int swp, int sub, int ins, int del );

/**
 * Check for exact match.
 *
 *     ( "test", "test" ) -> 0
 *     ( "test", "tset" ) -> -1
 *     ( "test", "test1" ) -> -1
 *     ( "test", "tes" ) -> -1
 */
int choice_exactcmp( const char* target, const char* str ) {
  return strcmp( target, str ) == 0 ? 0 : -1;
}

/**
 * Check the common prefix of two strings.
 * Return 0 if strings are strictly equal, positive if `str` is a prefix
 * of `target`, negative otherwise.
 *
 *     ( "test", "test" ) -> 0
 *     ( "test", "tset" ) -> -3
 *     ( "test", "test1" ) -> -1
 *     ( "test", "tes" ) -> 1
 *     ( "test", "te" ) -> 2
 *     ( "test", "teapot" ) -> -4
 */
int choice_prefixcmp( const char* target, const char* str ) {
  char t, s = '\0';
  int i;
  while( (t = *target) != '\0' && (s = *str) != '\0' && t == s ) {
    target++;
    str++;
  };
  if( s == '\0' ) {
    for( i=0; *target++ != '\0'; i++ );
    return i;
  } else {
    for( i=0; *str++ != '\0'; i-- );
    return i;
  }
}

/**
 * Compare using the levenshtein algorithm.
 * The algorithm weights favor addition (1) and swapping (2) of
 * characters, substitution (3) and deletion (4) will have a
 * larger impact on the distance.
 *
 * If the computed distance is greater than or equal to the
 * length of the target, a negative number is returned.
 *
 *     ( "test", "test" ) -> 0
 *     ( "test", "tset" ) -> 2           // swap
 *     ( "test", "test1" ) -> 4          // del one
 *     ( "test", "tes" ) -> 1            // add one
 *     ( "test", "te" ) -> 2             // add two
 *     ( "test", "teapot" ) -> -7 (4-11) // sub two, ..?
 */
int choice_fuzzycmp( const char* target, const char* str ) {
  int lent = strlen( target );
  int lens = strlen( str );
  int dist = levenshtein(str, lens, target, lent, 2, 3, 1, 4);
  if( dist >= lent )
    return lent-dist-1;
  return dist;
}

/**
 * Damerau Levenshtein distance.
 * Computes the number of "edits" that need to be made for
 * `string1` to be the same as `string2`.
 *
 * Adapted & extended from https://github.com/schuyler/levenshtein.
 * released by Schuyler Erle under the 2-term BSD license.
 */
static int levenshtein( const char* str1, size_t len1,
                        const char* str2, size_t len2,
                        int swp, int sub, int ins, int del ) {
  int *vn, *v0, *v1, *v2, *tmp;
  int i, j, next = 0;

  /* strip common prefixes */
  while( len1 > 0 && len2 > 0 && str1[0] == str2[0] )
    str1++, str2++, len1--, len2--;

  /* handle degenerate cases */
  if( !len1 ) return len2 * ins;
  if( !len2 ) return len1 * del;

  vn = calloc( (len2+1)*3, sizeof(int) );
  v0 = &(vn[0]);
  v1 = &(vn[len2+1]);
  v2 = &(vn[(len2+1)*2]);

  /* initialize the column vector */
  for( j = 0; j <= len2; j++ )
    v1[j] = j * ins;
  for( i = 0; i < len1; i++ ) {
    /* set the value of the first row (deletion) */
    v2[0] = (i + 1) * del;

    for( j = 0; j < len2; j++ ) {
      next = v1[j];

      /* substitute */
      if( str1[i] != str2[j] )
        next = v1[j] + sub;
      /* swap */
      if( i && (str1[i-1] == str2[j]) &&
          j && (str1[i] == str2[j-1]) &&
          next > v0[j-1] + swp )
        next = v0[j-1] + swp;
      /* delete */
      if( next > v1[j+1] + del )
        next = v1[j+1] + del;
      /* insert */
      if( next > v2[j] + ins )
        next = v2[j] + ins;

      v2[j+1] = next;
    }

    /* rotate v0 << v1 << v2 */
    tmp = v0;
    v0 = v1;
    v1 = v2;
    v2 = tmp;
  }

  free(vn);
  return next;
}

/** @} */

/* MARK: option lookup *//**
 * @name option lookup
 * @{
 */

/**
 * Lookup by name, disambiguate by result distance.
 */
static option_t* option_by_name( option_t* options, const char* str ) {
  option_t* option = NULL;
  int val, max = INT_MAX;
  while( options->name != NULL || options->abbr != '\0' ) {
    if( options->name != NULL ) {
      val = choice_fuzzycmp( options->name, str );
      if( val >= 0 && val < INT_MAX ) {
        if( val < max ) {
          max = val;
          option = options;
        } else if( val == max ) {
          /* ambiguous */
          fprintf( stderr, "ambiguous option --%s, could be --%s, --%s\n", str, option->name, options->name );
          return NULL;
        }
      }
    }
    options++;
  }
  return option;
}

/**
 * Lookup by abbreviation.
 */
static option_t* option_by_abbr( option_t* options, char c ) {
  while( options->name != NULL || options->abbr != '\0' ) {
    if( options->abbr != '\0' && options->abbr == c ) {
      return options;
    }
    options++;
  }
  return NULL;
}

/** @} */

typedef struct command_s command_t;

struct command_s {
  option_t* options;
  const char* name;
  int argc;
  char** argv;
};

/* MARK: argv dissection *//**
 * @name argv dissection
 * @{
 */

#ifndef _GNU_SOURCE
/**
 * This is a neat function, too bad it's not available everywhere.
 */
static char* strchrnul( char* str, int c ) {
  int d = *str;
  while( d != c && d != '\0' ) d = *(++str);
  return str;
}
#endif

/**
 * Shift one argument from the argv list.
 */
static char* arg_shiftstr( command_t* command ) {
  char* str = command->argv[0];
  command->argc -= 1;
  command->argv = &(command->argv[1]);
  return str;
}

/**
 * Shift one character from the current argument.
 */
static char arg_shiftc( command_t* command ) {
  char c = command->argv[0][0];
  command->argv[0] += 1;
  return c;
}

/**
 * Shift characters from the current argument until either
 * the passed character or the end of the string is reached.
 */
static char* arg_shiftstrchr( command_t* command, char c ) {
  char* str = command->argv[0];
  command->argv[0] = strchrnul( str, c );
  return str;
}

/** @} */

/* MARK: option parsing *//**
 * @name option parsing
 * @{
 */

static int option_callback( option_t* option, const char* arg ) {
  if( arg != NULL && *arg == '\0' )
    arg = NULL;

  if( arg == NULL && (option->flags & OPTION_REQARG) )
    return OPTION_EREQARG;
  else if( arg != NULL && !(option->flags & OPTION_ARG) )
    return OPTION_ENOARG;

  if( option->callback )
    return (option->callback)( option, arg );

  return 0;
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
            option_callback( option, NULL );
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
          /* TODO: keep looking */
          state = S_DONE;
        } else {
          arg = arg_shiftstr( &command );
          option_callback( option, arg );
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
            return -1;
          }
          if( option->flags & OPTION_ARG ) {
            arg = arg_shiftstr( &command );
            if( arg[0] != '\0' ) {
              option_callback( option, arg );
              option = NULL;
              state = S_ANY;
            } else {
              state = (option->flags & OPTION_REQARG) ? S_ARG : S_ANY;
            }
          } else {
            option_callback( option, NULL );
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
            return -1;
          }
          if( option->flags & OPTION_ARG ) {
            if( arg[0] != '\0' ) {
              option_callback( option, arg );
              option = NULL;
              state = S_ANY;
            } else {
              state = (option->flags & OPTION_REQARG) ? S_ARG : S_ANY;
            }
          } else if( arg[0] != '\0' ) {
            /* does not take args */
            fprintf( stderr, "option --%s does not take parameters (%s)!\n", option->name, arg );
            return -1;
          } else {
            option_callback( option, NULL );
            option = NULL;
            state = S_ANY;
          }
        }
        break;
      case S_DONE:
        return 0;
    }
  }

#undef S_ANY
#undef S_ARG
#undef S_ABBR
#undef S_NAME
#undef S_DONE
#undef ARG
  return 0;
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
      return 1;
    }
    if( option->flags & OPTION_ARG ) {
      if( arg[0] != '\0' ) {
        (option->callback)( option, arg );
        option = NULL;
      } else {
        /* requires arg */
        return 1;
      }
    } else if( arg[0] != '\0' ) {
      /* does not take args */
      return 1;
    } else {
      (option->callback)( option, NULL );
      option = NULL;
    }
  }
  return 0;
}

/** @} */

#ifdef TESTS
void distance_demo( int (*callback)( const char*, const char* ) ) {
  int i;
  struct {
    const char* a;
    const char* b;
    int d;
  } lev[] = {
    { "test", "test", 0 },
    { "test", "tset", 0 },
    { "test", "test1", 0 },
    { "test", "tes", 0 },
    { "test", "te", 0 },
    { "test", "teapot", 0 }
  };

  for( i=0; i<(sizeof(lev)/sizeof(lev[0])); i++ ) {
    lev[i].d = (callback)( lev[i].a, lev[i].b );
    printf( "  ( \"%s\", \"%s\" ) -> %i\n", lev[i].a, lev[i].b, lev[i].d );
  }
}
int main(void) {
  printf( "\nexact:\n" );
  distance_demo( &choice_exactcmp );
  printf( "\nprefix:\n" );
  distance_demo( &choice_prefixcmp );
  printf( "\nfuzzy:\n" );
  distance_demo( &choice_fuzzycmp );
}
#endif
