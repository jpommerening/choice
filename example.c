#include "choice.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#define UNUSED(x) (void)(x)

static struct {
  bool help;
  bool verbose;
  const char* required;
  long optional;
  bool write;
  long bsize;
} config = {
  false,
  false,
  NULL,
  0,
  true,
  1024
};

int option_subcommand() {
  return 0;
}

static option_t subopts[] = {
  { "rw", "read-write mode", '\0', OPTION_NODASH, &option_true, &config.write },
  { "ro", "read-only mode", '\0', OPTION_NODASH, &option_false, &config.write },
  { "bs", "block size", '\0', OPTION_REQARG|OPTION_NODASH, &option_long, &config.bsize },
  OPTION_EOL
};

static option_t insopts[] = {
  { "optional", "optional arg", 'o', OPTION_OPTARG, &option_long, &config.optional },
  { "subopt", "extra options", 's', OPTION_REQARG, &option_subopt, &(subopts[0]) },
  OPTION_EOL
};

static option_t options[] = {
  { "verbose", "enable verbose stuff", 'v', 0, &option_true, &config.verbose },
  { "required", "required arg", 'r', OPTION_REQARG, &option_str, &config.required },
  { "help", "display help", 'h', 0, &option_true, &config.help },
  { "install", "the install subcommand", '\0', OPTION_NODASH, &option_subcommand, &(insopts[0]) },
  OPTION_EOL
};

static void help() {
  option_t* option;
  printf( "usage: example [options]\n" );

  option = &(options[0]);
  printf( "\noptions:\n" );
  while( option->name != NULL || option->abbr != '\0' ) {
    option_help( option, NULL );
    option++;
  }

  option = &(subopts[0]);
  printf( "\nsubopts:\n" );
  while( option->name != NULL || option->abbr != '\0' ) {
    option_help( option, NULL );
    option++;
  }
}

int main( int argc, char* argv[] ) {
  option_parse( &(options[0]), argc, argv );

  if( config.help ) {
    help();
    return 0;
  } else {
    printf( "verbose: %s, required: %s, optional: %li, write: %s, bsize: %li\n",
            config.verbose ? "true" : "false",
            config.required,
            config.optional,
            config.write ? "true" : "false",
            config.bsize );
  }
  return 0;
}
