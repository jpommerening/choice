# choice -- the dyslexic option parser

> Do what I mean, not what I type, dammit!
> -- me, every time.

Choice is a parser for command-line options with support for
fuzzy option matching and generating command completion.

## why?

### The developer's answer

I can not recall how many ad-hoc option parsers I have written,
but it's probably been a few too many.
You start with a simple loop, looking for the one single flag
you program understands and it goes downhill from there. After a short
struggle, you end up sucking it up to `getopt` and its not quite
portable cousin `getopt_long` and that is that.

### The user's answer

See above.

## Examples

Let's drive this home with a few examples.

```C
#include "choice.h"
#include <stdbool.h>

static struct {
  bool verbose;
  const char* required;
  long optional;
} config = {
  false,
  NULL,
  0
};

static option_t options[] = {
  { "verbose", "enable verbose stuff", 'v', 0, &choice_true, &config.verbose },
  { "required", "required arg", 'r', CHOICE_REQARG, &choice_str, &config.required },
  { "optional", "optional arg", 'o', CHOICE_OPTARG, &choice_long, &config.optional }
};

int main( int argc, const char* argv[] ) {
  choice_parse( options, argc, argv );
}
```

## Credits

Inspiration for this library was taken from @isaacs' `npm isntall`
and @visionmedia's `commander.c`. Thank you!
