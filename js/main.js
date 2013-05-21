(function() {
  var $terminal = $('#terminal').terminal();
  window.T = $terminal.data('terminal');
  function P(line) {
    var argv = line.split(' ');
    var name = argv.shift();
    var Module = Choice(T);
    Module.callMain(argv);
    T.prompt(P);
  }

  T.prompt(P);
  T.$textarea.focus();

  function typein(str) {
    var i = setInterval(function() {
      T.type(str[0]);
      str = str.slice(1);
      if( !str ) {
        T.type(13);
        clearInterval(i);
      }
    }, 90);
  }

  typein('choice --halp');
}());


