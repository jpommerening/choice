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
 * CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE OR OTHERWISE)
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
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <setjmp.h>
#include <errno.h>

/* MARK: option callbacks *//**
 * @name option callbacks
 * @{
 */

/** Store `true` in variable pointed to by `option->data` */
int option_true( const option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((bool*) option->data) = true;
  return 0;
}

/** Store `false` in variable pointed to by `option->data` */
int option_false( const option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((bool*) option->data) = false;
  return 0;
}

/** Store a `long` number in variable pointed to by `option->data` */
int option_long( const option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((long*) option->data) = (arg == NULL) ? 0 : atol(arg);
  return 0;
}

/** Store `arg` unmodified in variable pointed to by `option->data` */
int option_str( const option_t* option, const char* arg ) {
  if( option->data != NULL )
    *((const char**) option->data) = arg;
  return 0;
}

/** Write a line to `stdout` */
int option_log( const option_t* option, const char* arg ) {
  if( option->flags & OPTION_NODASH )
    printf( " %-14s", option->name );
  else if( option->name != NULL && option->abbr != '\0' )
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

/** Print the classic option help we've come to expect from UNIX programs */
int option_help( const option_t* option, const char* arg ) {
  bool nodash = option->flags & OPTION_NODASH;
  bool reqarg = option->flags & OPTION_REQARG;
  bool optarg = option->flags & OPTION_OPTARG;
  const char* dash = nodash ? "" : "--";
  if( arg == NULL ) arg = "arg";

  if( option->name != NULL && option->abbr != '\0' ) {
    if( reqarg ) {
      pindent( "-%c, %s%s=<%s>", option->abbr, dash, option->name, arg );
    } else if( optarg ) {
      pindent( "-%c, %s%s [%s]", option->abbr, dash, option->name, arg );
    } else {
      pindent( "-%c, %s%s", option->abbr, dash, option->name, arg );
    }
  } else if( option->name != NULL ) {
    if( reqarg ) {
      pindent( "    %s%s=<%s>", dash, option->name, arg );
    } else if( optarg ) {
      pindent( "    %s%s [%s]", dash, option->name, arg );
    } else {
      pindent( "    %s%s", dash, option->name );
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
int option_subopt( const option_t* option, const char* arg ) {
  char* str = (char*)arg; /* Trust me, it's going to be fine. */
  return subopt_parse( option->data, str );
}

/** @} */

/* MARK: comparators *//**
 * @name comparators
 * All comparators are called with the option name first, followed by the
 * actual option that the user passed to the program. The length parameter
 * always refers to the length of the given option, which may be delimited
 * by either a null-character or an equals sign.
 * @{
 */

static int prefixlen( const char* string1, const char* string2, size_t len );

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
int choice_exactcmp( const char* target, const char* str, size_t len ) {
  int i = prefixlen( target, str, len );
  if( i == len )
    return target[i] == '\0' ? 0 : -1;
  return -1;
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
int choice_prefixcmp( const char* target, const char* str, size_t len ) {
  int i = prefixlen( target, str, len ), j;
  if( i == len ) {
    for( j=i; target[j] != '\0'; j++ );
    return j - i;
  } else {
    return i - (int) len;
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
int choice_fuzzycmp( const char* target, const char* str, size_t len ) {
  int lent = strlen( target );
  int lens = len;
  int dist = levenshtein(str, lens, target, lent, 2, 3, 1, 4);
  if( dist > lent )
    return lent-dist;
  return dist;
}

/**
 * Return the length of the common prefix of the two strings, but at
 * most `len`.
 */
static int prefixlen( const char* str1, const char* str2, size_t len ) {
  int i = 0;
  while( i < len && str1[i] != '\0' && str1[i] == str2[i] ) ++i;
  return i;
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
  i = prefixlen( str1, str2, len1 < len2 ? len1 : len2 );
  str1 += i; len1 -= i;
  str2 += i; len2 -= i;

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
 * Lookup an option with the given discriminator.
 */
static const option_t* lookup_option( const option_t* options,
                                      int (*by)( const option_t*, void* ),
                                      void* data ) {
  const option_t* option = NULL;
  int val, max = INT_MAX;
  while( options->name != NULL || options->abbr != '\0' ) {
    val = (*by)( options, data );
    if( val >= 0 && val < max ) {
      max = val;
      option = options;
    } else if( val == max ) {
      /* ambiguous */
    }
    options++;
  }
  return option;
}

typedef struct lookup_opts_s {
  const char* str;
  size_t len;
  option_cmp cmp;
} lookup_opts_t;

static int by_nodash( const option_t* option, void* data ) {
  lookup_opts_t* opts = data;
  if( option->flags & OPTION_NODASH && option->name != NULL )
    return (opts->cmp)( option->name, opts->str, opts->len );
  else
    return -1;
}

static int by_dash( const option_t* option, void* data ) {
  lookup_opts_t* opts = data;
  if( option->abbr != '\0' )
    return (option->abbr == opts->str[0]) ? 0 : INT_MAX;
  else
    return -1;
}

static int by_ddash( const option_t* option, void* data ) {
  lookup_opts_t* opts = data;
  if( !(option->flags & OPTION_NODASH) && option->name != NULL )
    return (opts->cmp)( option->name, opts->str, opts->len );
  else
    return -1;
}

/** @} */

#ifndef _GNU_SOURCE
/**
 * This is a neat function, too bad it's not available everywhere.
 */
static char* strchrnul( const char* str, int c ) {
  int d = *str;
  while( d != c && d != '\0' ) d = *(++str);
  return (char*)str;
}
#endif

/* MARK: option parsing *//**
 * @name option parsing
 * @{
 */

static int option_callback( option_t* option, const char* arg ) {
  if( arg != NULL && *arg == '\0' )
    arg = NULL;

  if( arg == NULL && (option->flags & OPTION_REQARG) )
    return EINVAL;
  else if( arg != NULL && !(option->flags & OPTION_ARG) )
    return EINVAL;

  if( option->callback )
    return (option->callback)( option, arg );

  return 0;
}

#define IS_NODASH(x) (((x)[0] != '-') && ((x)[0] != '\0'))
#define IS_DASH(x)   (((x)[0] == '-') && IS_NODASH(x+1))
#define IS_DDASH(x)  (((x)[0] == '-') && IS_DASH(x+1))
#define IS_END(x)    (((x)[0] == '-') && ((x)[1] == '-') && ((x)[2] == '\0'))

static int parse_arg( int argc, const char* argv[], const option_t* option, const char* arg ) {
  return 0;
}

/**
 * Parse an argument that has no dashes. This might be a subcommand or an
 * arbitary word.
 * @return the number of arguments taken from argv, <=0 if an error occured.
 */
static int parse_nodash( int argc, const char* argv[], const option_t* options, option_cmp cmp ) {
  lookup_opts_t opts = { NULL, 0, cmp };
  const char* arg = argv[0];
  const option_t* option;
  assert( arg && IS_NODASH(arg) );

  opts.str = arg;
  opts.len = strchrnul(opts.str, '=') - opts.str;

  if ((option = lookup_option(options, by_nodash, &opts ))) {
    if (OPTION_ARG & option->flags) {
      if (*arg == '=') {
        option->callback(option, arg+1);
        return 1;
      } else if (*argv[1] != '-') {
        option->callback(option, argv[1]);
        return 2;
      } else {
        option->callback(option, NULL);
        return 1;
      }
    } else {
      option->callback(option, NULL);
      return 1;
    }
  } else {
    return 0;
  }
}

/**
 * Parse an argument that starts with a single dash. The argument may
 * contain multiple flags and a parameter.
 * @return the number of arguments taken from argv, <=0 if an error occured.
 */
static int parse_dash( int argc, const char* argv[], const option_t* options ) {
  lookup_opts_t opts = { NULL, 0, NULL };
  const char* arg = argv[0];
  const option_t* option;
  assert( arg && IS_DASH(arg) );

  /* strip the leading dash and iterate through the characters */
  while (*(++arg) != '\0') {

    opts.str = arg;
    opts.len = 1;

    if ((option = lookup_option(options, by_dash, &opts))) {
      if (OPTION_ARG & option->flags) {
        if (*arg != '\0') {
          option->callback(option, arg+1);
          return 1;
        } else if (*argv[1] != '-') {
          option->callback(option, argv[1]);
          return 2;
        } else {
          option->callback(option, NULL);
        }
      } else {
        option->callback(option, NULL);
      }
    } else {
      return -1;
    }
  }
  return 1;
}

/**
 * Parse an argument that starts with a double dash.
 * @return the number of arguments taken from argv, <=0 if an error occured.
 */
static int parse_ddash( int argc, const char* argv[], const option_t* options, option_cmp cmp ) {
  lookup_opts_t opts = { NULL, 0, cmp };
  const char* arg = argv[0];
  const option_t* option;
  assert( arg && IS_DDASH(arg) );

  opts.str = arg+2;
  opts.len = strchrnul(opts.str, '=') - opts.str;

  if ((option = lookup_option(options, by_ddash, &opts))) {
    if (OPTION_ARG & option->flags) {
      if (*arg == '=') {
        option->callback(option, arg+1);
        return 1;
      } else if (*argv[1] != '-') {
        option->callback(option, argv[1]);
        return 2;
      } else {
        option->callback(option, NULL);
        return 1;
      }
    } else {
      option->callback(option, NULL);
      return 1;
    }
  } else {
    return 0;
  }
}

static int parse_end( int argc, const char* argv[], option_t* options ) {
  const char* arg = argv[0];
  assert( arg && IS_END(arg) );
  return 0; // ?
}

static int parse_next( int argc, const char* argv[], option_t* options, option_cmp cmp ) {
  const char* arg;

  if (argc > 0) {
    arg = argv[0];
  } else {
    return 0;
  }

  if (arg[0] != '-') {
    /* "nodash" */
    return parse_nodash( argc, argv, options, cmp );
  } else if (arg[1] != '-') {
    /* "-dash" */
    return parse_dash( argc, argv, options );
  } else if (arg[2] != '\0') {
    /* "--ddash" */
    return parse_ddash( argc, argv, options, cmp );
  } else {
    /* "--" */
    return parse_end( argc, argv, options );
  }
}

int option_parse( option_t* options, int argc, char* argv[] ) {
  int p = 0;
  while ((p = parse_next(argc, argv, options, &choice_fuzzycmp))) {
    argc -= p;
    argv += p;
  }
  return p;
}

int subopt_parse(option_t* options, char* argv) {
  return 0;
}

/** @} */

#ifdef TESTS
void distance_demo( int (*callback)( const char*, const char*, size_t ) ) {
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
    lev[i].d = (callback)( lev[i].a, lev[i].b, strlen(lev[i].b) );
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
