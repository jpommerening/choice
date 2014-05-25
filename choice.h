/* -*- Mode: C; tab-width: 2; c-basic-offset: 2 -*- */
/* vim:set softtabstop=2 shiftwidth=2: */
/*
 * choice -- the dyslexic option parser
 * ====================================
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
 */
#ifndef __CHOICE_H
#define __CHOICE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  OPTION_REQARG = 1,
  OPTION_OPTARG = 2,
  OPTION_ARG = 3,
  OPTION_NODASH = 4,
  OPTION_MULTIPLE = 8,
  OPTION_CALLED = 16
} option_flag_t;

typedef struct option_s option_t;

typedef int (*option_cb)( const option_t* option, const char* arg );
typedef int (*option_cmp)( const char* target, const char* arg, size_t len );

struct option_s {
  const char abbr;
  const char* name;
  const char* desc;
  unsigned flags;
  option_cb callback;
  void* data;
};

extern int option_true( const option_t* option, const char* arg );
extern int option_false( const option_t* option, const char* arg );
extern int option_long( const option_t* option, const char* arg );
extern int option_str( const option_t* option, const char* arg );
extern int option_log( const option_t* option, const char* arg );
extern int option_help( const option_t* option, const char* arg );
extern int option_subopt( const option_t* option, const char* arg );

extern int option_exactcmp( const char* target, const char* arg, size_t len );
extern int option_prefixcmp( const char* target, const char* arg, size_t len );
extern int option_fuzzycmp( const char* target, const char* arg, size_t len );

#define OPTION_TRUE(abbr, name, desc, bool_var) \
  { abbr, name, desc, 0, option_true, &bool_var }
#define OPTION_FALSE(abbr, name, desc, bool_var) \
  { abbr, name, desc, 0, option_false, &bool_var }
#define OPTION_LONG(abbr, name, desc, long_var) \
  { abbr, name, desc, OPTION_REQARG, option_long, &long_var }
#define OPTION_STR(abbr, name, desc, str_var) \
  { abbr, name, desc, OPTION_REQARG, option_str, &str_var }
#define OPTION_SUBOPT(name, desc, opts) \
  { abbr, name, desc, OPTION_REQARG, option_subopt, &(opts[0]) }
#define OPTION_EOL \
  { '\0', NULL, NULL, 0, NULL, NULL }

extern int option_error( const char* fmt, ... );
extern int option_parse( option_t* options, int argc, char* argv[] );
extern int subopt_parse( option_t* options, char *argv );

#ifdef __cplusplus
}
#endif

#endif
