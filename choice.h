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

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define OPTION_EINVAL 1  /* invalid option */
#define OPTION_ENOARG 2  /* option has no argument */
#define OPTION_EREQARG 3 /* option requires an argument */
#define OPTION_EONCE 4   /* option already seen */
#define OPTION_EAMBIG 5  /* option is ambiguous */

typedef enum {
  OPTION_REQARG = 1,
  OPTION_OPTARG = 2,
  OPTION_ARG = 3,
  OPTION_NODASH = 4,
  OPTION_MULTIPLE = 8,
  OPTION_CALLED = 16
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

#ifdef __cplusplus
}
#endif

#endif
