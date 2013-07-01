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
    var $element = $(element);
    var $cursor = $('<span class="cursor">&nbsp;</span>');
    var $textarea = $('<textarea rows="1" cols="1" style="opacity: 0;"></textarea>');
    var $kbdinput = $('<kbd></kbd>');

    this.options = options;
    this.callback = function() {};
    this.readline = new LineEdit(new Events());
    new Move(this.readline);
    new Edit(this.readline);
    new History(this.readline);
    new Kill(this.readline);
    new Completion(this.readline, function() { return ["--verbose","--help"]});

    var that = this;

    new Input(this.readline, $textarea);
    new Binding(this.readline);

    this.readline.on('konami-code', function() {
      console.log("yay");
    });

    this.readline.on('change', function(str) {
      that.kbd(str);
    });
    this.readline.on('move', function(col) {
      that.kbd(col);
    });
    this.readline.on('accept-line', function() {
      var text = this.line;
      console.log("accept", text);
      this.trigger('end-of-line');
      that.println();
      that.callback(text);
    });
    this.readline.on('completions', function(completions) {
      this.trigger('end-of-line');
      that.println();
      for (var i=0; i<completions.length; i++ ) {
        that.println(completions[i]);
      }
      that.prompt(that.PS1, that.callback);
      that.kbd();
    });

    $textarea.on('focus', function() {
      $cursor.addClass('pulse');
    });
    $textarea.on('blur', function() {
      $cursor.removeClass('pulse');
    });

    this.$element = $element;
    this.$cursor = $cursor;
    this.$textarea = $textarea;
    this.$kbdinput = $kbdinput;
    this.$element.append(this.$cursor);
    this.$element.append(this.$kbdinput);
    this.$element.append(this.$textarea);
    this.$element.on('click', function() { $textarea.focus(); });
    this.$element.on('focus', function() { $textarea.focus(); });
  }

  var escElem = $('<div/>');
  function escapeHtml(str) {
    return escElem.text(str).html();
  }

    var names = {
      RUBOUT: 8,
      TAB: 9,
      LFD: 10,
      NEWLINE: 10,
      RET: 13,
      RETURN: 13,
      ESC: 27,
      ESCAPE: 27,
      SPC: 32,
      SPACE: 32,
      PGUP: 33,
      PAGEUP: 33,
      PGDN: 34,
      PAGEDOWN: 34,
      END: 35,
      HOME: 36,
      LEFT: 37,
      UP: 38,
      RIGHT: 39,
      DOWN: 40,
      INS: 45,
      INSERT: 45,
      DEL: 46,
      DELETE: 46
    };

  Terminal.prototype = {
    
    constructor: Terminal

  , PS1: (window.location.host || 'Terminal') + ' $ '
  , PS2: ' > '

  , print: function(output, cls) {
      if (output) {
        var samp = $('<samp' + (cls?' class="'+cls+'"':'') + '></samp>').text(output);
        this.$cursor.before(samp);
        if( this.$kbdinput.html() ) this.$kbdinput = $('<kbd></kbd>');
        this.$cursor.before(this.$kbdinput);
        this.$element.scrollTop(this.$element[0].scrollHeight);
      }
    }

  , println: function(output, cls) {
      if (output) {
        var samp = $('<samp' + (cls?' class="'+cls+'"':'') + '></samp>').text(output);
        this.$cursor.before(samp);
      }
      this.$cursor.before('<br/>');
      if( this.$kbdinput.html() ) this.$kbdinput = $('<kbd></kbd>');
      this.$cursor.before(this.$kbdinput);
      this.$element.scrollTop(this.$element[0].scrollHeight);
    }

  , prompt: function(output, callback) {
      if (typeof output === 'function') {
        callback = output;
        output = this.PS1;
      }
      this.print(output, 'prompt');
      this.callback = callback;
    }

  , kbd: function(param) {
      var cursor = this.readline.cursor;
      var buffer = this.readline.line;
      if (typeof param === 'string')
        buffer = param;
      else if (typeof param === 'number')
        cursor = param;
      if (cursor >= buffer.length) {
        this.$cursor.text(" ");
        this.$kbdinput.text(buffer);
        this.$kbdinput.append(this.$cursor);
      } else {
        var pre = buffer.substr(0, cursor);
        var cur = buffer[cursor];
        var post = buffer.substr(cursor+1);

        this.$cursor.text(cur);
        this.$kbdinput.text(pre);
        this.$kbdinput.append(this.$cursor);
        this.$kbdinput.append(escapeHtml(post));
      }
      this.$element.scrollTop(this.$element[0].scrollHeight);
    }

  , type: function(str) {
      this.readline.trigger('self-insert', str);
    }

  , enter: function(str) {
      if (str) this.type(str);
      this.readline.trigger('accept-line');
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
