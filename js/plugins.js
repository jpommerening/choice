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
    this.callback = function() {};
    this.readline = new Readline();

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

  function Readline() {
    this.cursor = 0; /* character offset in current line */
    this.line = 0; /* currently selected history line */
    this.top = 0; /* current top of history */
    this.previous = ''; /* backup for history line */
    this.history = ['']; /* history */
    this.overwrite = false; /* overwrite/insert mode */
  }

  Readline.prototype = {
    constructor: Readline
  , jump: function(line) {
      if (line >= 0 && line <= this.top) {
        if (line != this.top)
          this.history[this.line] = this.previous;
        this.line = line;
        this.previous = this.history[line];
      }
    }
  , text: function() {
      return this.history[this.line];
    }
  /* Moving:
   * http://www.gnu.org/software/bash/manual/html_node/Commands-For-Moving.html */
  , 'beginning-of-line': function() {
      this.cursor = 0;
    }
  , 'end-of-line': function() {
      this.cursor = this.history[this.line].length;
    }
  , 'forward-char': function() {
      if (this.cursor < this.history[this.line].length) this.cursor += 1;
    }
  , 'backward-char': function() {
      if (this.cursor > 0) this.cursor -= 1;
    }
  , 'forward-word': function() {
      var offset = this.history[this.line].indexOf(' ', this.cursor);
      if (offset < 0) {
        this['end-of-line']();
      } else {
        this.cursor = offset;
      }
    }
  , 'backward-word': function() {
      var offset = this.history[this.line].lastIndexOf(' ', this.cursor);
      if (offset < 0) {
        this['beginning-of-line']();
      } else {
        this.cursor = offset;
      }
    }
  /* History:
   * http://www.gnu.org/software/bash/manual/html_node/Commands-For-History.html */
  , 'accept-line': function() {
      if (this.history[this.line]) {
        if (this.top != this.line) {
          this.history[this.top] = this.history[this.line];
          this.history[this.line] = this.previous;
        }
        this.history.push('');
        this.previous = '';
        this.cursor = 0;
        this.line = this.top = this.history.length - 1;
      }
    }
  , 'previous-history': function() {
      this.jump(this.line-1);
    }
  , 'next-history': function() {
      this.jump(this.line+1);
    }
  , 'beginning-of-history': function() {
      this.jump(0);
    }
  , 'end-of-history': function() {
      this.jump(this.top);
    }
  , 'reverse-search-history': function() {
    }
  , 'forward-search-history': function() {
    }
  , 'history-search-forward': function() {
      var prefix = this.text().substr(0, this.cursor);
      var line = this.line;
      while (++line < this.top) {
        if (this.history[line].substr(0, prefix.length) === prefix) {
          this.line = line;
          return;
        }
      }
    }
  , 'history-search-backward': function() {
      var prefix = this.text().substr(0, this.cursor);
      var line = this.line;
      while (--line >= 0) {
        if (this.history[line].substr(0, prefix.length) === prefix) {
          this.line = line;
          return;
        }
      }
    }
  /* Changing Text
   * http://www.gnu.org/software/bash/manual/html_node/Commands-For-Text.html */
  , 'delete-char': function() {
      var text = this.history[this.line];
      var cursor = this.cursor;
      this.history[this.line] = ( text.substr(0, cursor) + text.substr(cursor+1) );
    }
  , 'backward-delete-char': function(num) {
      var text = this.history[this.line];
      var cursor = this.cursor;
      if (typeof num === 'undefined') num = 1;
      if (cursor < num) num = cursor;
      this.history[this.line] = ( text.substr(0, cursor-num) + text.substr(cursor) );
      this.cursor -= num;
  }
  , 'forward-backward-delete-char': function() {
      var text = this.history[this.line];
      var cursor = this.cursor;
      if (cursor >= text.length)
        this['backward-delete-char']();
      else
        this['delete-char']();
    }
  , 'self-insert': function(str) {
      var text = this.history[this.line];
      var cursor = this.cursor;
      var overwrite = this.overwrite ? str.length : 0;
      this.history[this.line] = ( text.substr(0, cursor) + str + text.substr(cursor+overwrite) );
      this.cursor += str.length - overwrite;
    }
  , 'overwrite-mode': function(mode) {
      this.overwrite = (mode === undefined) ? !(this.overwrite) : !!(mode);
    }
  };

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
          /* backspace */
       8: 'backward-delete-char'
          /* page up */
    , 33: 'history-search-backward'
          /* page down */
    , 34: 'history-search-forward'
          /* end */
    , 35: 'end-of-line'
          /* home */
    , 36: 'beginning-of-line'
          /* left */
    , 37: 'backward-char'
          /* up */
    , 38: 'previous-history'
          /* right */
    , 39: 'forward-char'
          /* down */
    , 40: 'next-history'
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
      var cursor = this.readline.cursor;
      var buffer = this.readline.text();
      if (cursor >= buffer.length) {
        this.$cursor.text(" ");
        this.$kbdinput.text(buffer);
        this.$kbdinput.after(this.$cursor);
      } else {
        var pre = buffer.substr(0, cursor);
        var cur = buffer[cursor];
        var post = buffer.substr(cursor+1);

        this.$cursor.text(cur);
        this.$kbdinput.text(pre);
        this.$kbdinput.append(this.$cursor);
        this.$kbdinput.append(escapeHtml(post));
      }
    }

  , type: function(str) {
      if (typeof str === 'number') {
        switch(str) {
          case 13:
          case 10:
            this.readline['end-of-line']();
            var text = this.readline.text();
            this.kbd();
            this.readline['accept-line']();
            this.println();
            this.callback( text );
            return;
        }
        this.kbd();
      } else {
        this.readline['self-insert'](str);
        this.kbd();
      }
    }

  , keydown: function(event) {
      if (this.commands.hasOwnProperty(event.which))
        this.readline[this.commands[event.which]]();
      else
        this.type(event.which);
      this.kbd();
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
