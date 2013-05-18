// Avoid `console` errors in browsers that lack a console.
(function() {
    var method;
    var noop = function () {};
    var methods = [
        'assert', 'clear', 'count', 'debug', 'dir', 'dirxml', 'error',
        'exception', 'group', 'groupCollapsed', 'groupEnd', 'info', 'log',
        'markTimeline', 'profile', 'profileEnd', 'table', 'time', 'timeEnd',
        'timeStamp', 'trace', 'warn'
    ];
    var length = methods.length;
    var console = (window.console = window.console || {});

    while (length--) {
        method = methods[length];

        // Only stub undefined methods.
        if (!console[method]) {
            console[method] = noop;
        }
    }
}());

// Terminal plugin
(function ($) {
  function Terminal(element, options) {
    this.options = options;
    this.history = [];
    this.buffer = '';
    this.cursor = 0;
    this.callback = function() {};

    var $textarea = $('<textarea rows="1" cols="1" style="opacity: 0"></textarea>');
    var $kbdinput = $('<kbd></kbd>');
    $textarea.on('keypress', $.proxy(this.keypress, this));

    this.$element = $(element);
    this.$textarea = $textarea;
    this.$kbdinput = $kbdinput;
    this.$element.append(this.$kbdinput);
    this.$element.append(this.$textarea);
    this.$element.on('click', function() { $textarea.focus(); });
    this.$element.on('focus', function() { $textarea.focus(); });
  }

  Terminal.prototype = {
    
    constructor: Terminal

  , PS1: (window.location.host || 'Terminal') + ' $ '
  , PS2: ' > '

  , repeat: {
      delay: 1000,
      rate: 7/1000
    }

  , commands: {
  
    }

  , colors: {
    }

  , print: function(output, cls) {
      if (output) {
        var samp = $('<samp' + (cls?' class="'+cls+'"':'') + '></samp>').text(output);
        this.$textarea.before(samp);
        if( this.$kbdinput.text() ) this.$kbdinput = $('<kbd></kbd>');
        this.$textarea.before(this.$kbdinput);
      }
    }

  , println: function(output, cls) {
      if (output) {
        var samp = $('<samp' + (cls?' class="'+cls+'"':'') + '></samp>').text(output);
        this.$textarea.before(samp);
      }
      this.$textarea.before('<br/>');
      if( this.$kbdinput.text() ) this.$kbdinput = $('<kbd></kbd>');
      this.$textarea.before(this.$kbdinput);
    }

  , prompt: function(output, callback) {
      if (typeof output === 'function') {
        callback = output;
        output = this.PS1;
      }
      this.print(output, 'prompt');
      this.callback = callback;
    }

  , type: function(str) {
      if (typeof str === 'number') {
        switch (str) {
          case 13:
            this.println();
            this.callback( this.buffer );
            this.buffer = "";
            break;
          case 8:
            this.buffer = this.buffer.slice(0,-1);
            this.$kbdinput.text(this.buffer);
            break;
        }
      } else {
        this.buffer += str;
        this.$kbdinput.text(this.buffer);
      }
    }

  , keypress: function(event) {
      if (event.which < 20)
        this.type(event.which);
      else
        this.type(String.fromCharCode(event.which));
    }

  };

  $.fn.terminal = function (option) {
      return this.each(function () {
              var $this = $(this)
                , data = $this.data('terminal')
                , options = $.extend({}, $.fn.terminal.defaults, $this.data(), typeof option == 'object' && option)
              if (!data) $this.data('terminal', (data = new Terminal(this, options)))
            })
        }

  $.fn.terminal.defaults = {
  }

  $.fn.terminal.Constructor = Terminal
}(jQuery));
