# choice -- the dyslexic option parser

> Do what I mean, not what I type, dammit!
> -- me, every time.

Choice is a parser for command-line options with support for
fuzzy option matching and generating command completion.

## Why?

### The developer's answer

I can not recall how many ad-hoc option parsers I have written,
but it's probably been a few too many.
You start with a simple loop, looking for the one single flag
you program understands and it goes downhill from there. After a short
struggle, you end up sucking it up to `getopt` and its not quite
portable cousin `getopt_long` and that is that.

### The user's answer

As a heavy command-line user, I've grown pretty accustomed to the
behaviour of `getopt`. The combination of long and short options works
surprisingly well. I don't really care _what_ parses the options.

## Examples

Let's drive this home with a few examples.

```c
#include "choice.h"
#include <stdbool.h>

static struct {
  bool verbose;
  const char* required;
  long optional;
} config = {
  false,
  NULL,
  -1
};

static option_t options[] = {
  { "verbose", "enable verbose stuff", 'v', 0, &choice_true, &config.verbose },
  { "required", "required arg", 'r', OPTION_REQARG, &choice_str, &config.required },
  { "optional", "optional arg", 'o', OPTION_OPTARG, &choice_long, &config.optional }
};

int main( int argc, const char* argv[] ) {
  option_parse( options, argc, argv );
  printf( "verbose: %s\n", config.verbose ? "true" : "false" );
  printf( "required: %s\n", config.required );
  printf( "optional: %li\n", config.optional );
  return 0;
}
```

```console
$ example
verbose: false
required: (null)
optional: -1
$ example --verbose --required=test
verbose: true
required: test
optional: -1
$ example -vrtest
verbose: true
required: test
optional: -1
$ example --vebrose --requird derp
verbose: true
required: derp
optional: -1
$ example --verb --opt
verbose: true
required: (null)
optional: 0
```

## Credits

Inspiration for this library was taken from @isaacs' `npm isntall`
and @visionmedia's `commander.c`.  The levelshtein implementation was adapted
and extended from @schuyler's ruby module.  
_Thank you!_

## License

Copyright &copy; 2013, Jonas Pommerening <jonas.pommerening@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1.  Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
