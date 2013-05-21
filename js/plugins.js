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

    var $cursor = $('<span class="cursor">&nbsp;</span>');
    var $textarea = $('<textarea rows="1" cols="1" style="opacity: 0;"></textarea>');
    var $kbdinput = $('<kbd></kbd>');
    $textarea.on('keypress', $.proxy(this.keypress, this));
    $textarea.on('keydown', $.proxy(this.keydown, this));
    $textarea.on('focus', function() {
      $cursor.addClass('pulse');
    });
    $textarea.on('blur', function() {
      $cursor.removeClass('pulse');
    });

    this.$element = $(element);
    this.$cursor = $cursor;
    this.$textarea = $textarea;
    this.$kbdinput = $kbdinput;
    this.$element.append(this.$cursor);
    this.$element.append(this.$kbdinput);
    this.$element.append(this.$textarea);
    this.$element.on('click', function() { $textarea.focus(); });
    this.$element.on('focus', function() { $textarea.focus(); });
  }

    var history = [];
    var maxlen = 50;
    var entry = -1;

    function history_next() {
      if (entry >= history.length) return undefined;
      return history[entry++];
    }

    function history_previous() {
      if (entry < 0) return undefined;
      return history[entry--];
    }

    function history_search_forward(str) {
      for (; entry < history.length; entry++)
        if (history[entry].substr(0, str.length) == str)
          return history[entry];
      return str;
    }

    function history_search_backward(str) {
      for (; entry >= 0; entry--)
        if (history[entry].substr(0, str.length) == str)
          return history[entry];
      return str;
    }

    function push(str) {
      history.push(str);
      if (history.length > maxlen)
        history = history.splice(1);
      entry = history.length - 1;
    }

  var escElem = $('<div/>');
  function escapeHtml(str) {
    return escElem.text(str).html();
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
        this.$cursor.before(samp);
        if( this.$kbdinput.html() ) this.$kbdinput = $('<kbd></kbd>');
        this.$cursor.before(this.$kbdinput);
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
    }

  , prompt: function(output, callback) {
      if (typeof output === 'function') {
        callback = output;
        output = this.PS1;
      }
      this.print(output, 'prompt');
      this.callback = callback;
    }

  , kbd: function() {
      if (this.cursor >= this.buffer.length) {
        this.$cursor.text(" ");
        this.$kbdinput.text(this.buffer);
        this.$kbdinput.after(this.$cursor);
      } else {
        var pre = this.buffer.substr(0, this.cursor);
        var cur = this.buffer[this.cursor];
        var post = this.buffer.substr(this.cursor+1);

        this.$cursor.text(cur);
        this.$kbdinput.text(pre);
        this.$kbdinput.append(this.$cursor);
        this.$kbdinput.append(escapeHtml(post));
      }
    }

  , type: function(str) {
      if (typeof str === 'number') {
        switch (str) {
          case 33: // pg up
            this.buffer = history_search_backward(this.buffer) || '';
            break;
          case 34: // pg down
            this.buffer = history_search_forward(this.buffer.substr(0, this.cursor)) || '';
            break;
          case 35: // end
            this.cursor = this.buffer.length;
            break;
          case 36: // home
            this.cursor = 0;
            break;
          case 37: // left
            if (this.cursor > 0) this.cursor -= 1;
            break;
          case 38: // up
            this.buffer = history_previous();
            break;
          case 39: // right
            if (this.cursor < this.buffer.length) this.cursor += 1;
            break;
          case 40: // down
            this.buffer = history_next();
            break;
          case 13:
          case 10:
            this.cursor = this.buffer.length;
            this.kbd();
            this.println();
            push( this.buffer );
            this.callback( this.buffer );
            this.cursor = 0;
            this.buffer = '';
            return;
          case 8:
            var tmp = this.buffer;
            if (this.cursor > 0) {
              this.buffer = ( tmp.substr(0, this.cursor-1)
                            + tmp.substr(this.cursor, tmp.length) );
              this.cursor -= 1;
            }
            break;
        }
        this.kbd();
      } else {
        var tmp = this.buffer;
        this.buffer = ( tmp.substr(0, this.cursor) + str
                      + tmp.substr(this.cursor, tmp.length) );
        this.cursor += str.length;
        this.kbd();
      }
    }

  , keydown: function(event) {
      console.log(history, entry, event.which);
      this.type(event.which);
      return 0;
    }

  , keypress: function(event) {
      if (event.which >= 32)
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
