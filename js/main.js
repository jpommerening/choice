$(function() {
  var input  = $( 'input[name="command"]' );
  var prompt = $( 'form#prompt' );
  var stdout = $( 'div#console' );
  var code   = $( 'code.console' );

  function repl() {
    var value = input.val();
    var argv  = value.split(' ');
    var cmd   = argv.shift();
    console.log( cmd, argv );
    output = 'test\n';

    prompt.before( '<samp class="prompt">choice.github.io $ </samp><kbd>' + value + '</kbd>\n' );
    prompt.before( '<samp>' + output + '</samp>' );

    input.val('');
    input.focus();
    stdout.scrollTop( code.height() );
    return false;
  }

  function typein(cmd) {
    var c = 0;
    var i = setInterval( function() {
      var v = input.val();
      input.val(v + cmd[c++]);
      if( c >= cmd.length ) {
        clearInterval(i);
      }
    }, 110);
  }

  prompt.submit(repl);
  input.focus();
  typein("choice ");
});
