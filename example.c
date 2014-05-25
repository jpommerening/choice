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
  { '\0', "rw", "read-write mode", OPTION_NODASH, &option_true, &config.write },
  { '\0', "ro", "read-only mode", OPTION_NODASH, &option_false, &config.write },
  { '\0', "bs", "block size", OPTION_REQARG|OPTION_NODASH, &option_long, &config.bsize },
  OPTION_EOL
};

static option_t insopts[] = {
  { 'o', "--optional", "optional arg", OPTION_OPTARG, &option_long, &config.optional },
  { 's', "--subopt", "extra options", OPTION_REQARG, &option_subopt, &(subopts[0]) },
  OPTION_EOL
};

static option_t options[] = {
  { 'v', "verbose", "enable verbose stuff", 0, &option_true, &config.verbose },
  { 'r', "required", "required arg", OPTION_REQARG, &option_str, &config.required },
  { 'h', "help", "display help", 0, &option_true, &config.help },
  { '\0', "install", "the install subcommand", OPTION_NODASH, &option_subcommand, &(insopts[0]) },
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
  option_parse( &(options[0]), argc-1, argv+1 );

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
