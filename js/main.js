(function() {
  var $terminal = $('#terminal').terminal();
  window.T = $terminal.data('terminal');
  function P(line) {
    var argv = line.split(' ');
    var name = argv.shift();
    var Module = Choice(T);
    Module.callMain(argv);
    setTimeout(function() { T.prompt(P); T.$cursor.focus(); }, 0);
  }

  T.prompt(P);
  T.$textarea.focus();

  function typein(str) {
    var intv = setInterval(function typist() {
      if (str.length) {
        T.type(str[0]);
        str = str.slice(1);
      } else {
        T.enter();
        clearInterval(intv);
        intv = null;
      }
    }, 90);
  }

  typein('choice --halp');
}());


