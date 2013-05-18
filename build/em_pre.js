function Choice(term) {
  Module = {
    noInitialRun: true,
    print: function(line) { term.println(line, 'stdout'); },
    printErr: function(line) { term.println(line, 'stderr'); }
  };
