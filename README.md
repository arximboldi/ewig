[![Build Status](https://github.com/arximboldi/zug/workflows/test/badge.svg)](https://github.com/arximboldi/zug/actions?query=workflow%3Atest+branch%3Amaster)
<a href="https://sinusoid.al"><img align="right" src="https://cdn.rawgit.com/arximboldi/immer/355a113782aedc2ea22463444014809269c2376d/doc/_static/sinusoidal-badge.svg"></a>

<p align="center">
  <img src="https://cdn.rawgit.com/arximboldi/ewig/36d00237/doc/logo-front.svg">
</p>

**ewig** is a simple text editor
(an [Ersatz Emacs](https://www.emacswiki.org/emacs/ErsatzEmacs)) written
using [immutable data-structures](https://sinusoid.es/immer/) in C++.

The code is written in a simple style to showcase a value-based
functional architecture.  We invite you to
[study the code](https://github.com/arximboldi/ewig/tree/master/src/ewig).
Learn more in the **CppCon'17 Talk**:
_[Postmodern Immutable Data Structures](https://www.youtube.com/watch?v=sPhpelUfu8Q)_.

> <a href="https://www.patreon.com/sinusoidal">
>     <img align="right" src="https://cdn.rawgit.com/arximboldi/immer/master/doc/_static/patreon.svg">
> </a>
>
> This project is part of a long-term vision helping interactive and
> concurrent C++ programs become easier to write. **Help this project's
> long term sustainability by becoming a patron or buying a
> sponsorship package:** juanpe@sinusoid.al

---

<p align="center">
  <a href="https://asciinema.org/a/135452">
    <img src="https://cdn.rawgit.com/arximboldi/ewig/d32b8a391c4a9f788175bf982bbdc5150d3f5a96/doc/ewig.gif">
  </a>
</p>

Try it out
----------

If you are using the [Nix package manager](https://nixos.org/nix) (we
strongly recommend it) you can just install the software with.
```
    nix-env -if https://github.com/arximboldi/ewig/archive/master.tar.gz
```

Development
-----------

To build the code you need a C++17 compiler, `cmake`, and `ncurses`
with unicode support (package `libncursesw5-dev` in Debian and
friends).

You can install those manually, but the easiest way to get a
development environment up and running is by using
the [Nix package manager](https://nixos.org/nix).  At the root of the
repository just type:
```
    nix-shell
```
This will download all required dependencies and create an isolated
environment in which you can use these dependencies, without polluting
your system.

Then you can **generate a development** project using [CMake](https://cmake.org/).
```
    mkdir build && cd build
    cmake ..
```

To configure an optimized build and **compile** do:
```
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make
```

To **install** the compiled software globally:
```
    sudo make install
```

Keybindings
-----------

Excerpt from
[`main.cpp`](https://github.com/arximboldi/ewig/blob/master/src/ewig/main.cpp):
```cpp
const auto key_map_emacs = make_key_map(
{
    {key::seq(key::ctrl('p')), "move-up"},
    {key::seq(key::up),        "move-up"},
    {key::seq(key::down),      "move-down"},
    {key::seq(key::ctrl('n')), "move-down"},
    {key::seq(key::ctrl('b')), "move-left"},
    {key::seq(key::left),      "move-left"},
    {key::seq(key::ctrl('f')), "move-right"},
    {key::seq(key::right),     "move-right"},
    {key::seq(key::page_down), "page-down"},
    {key::seq(key::page_up),   "page-up"},
    {key::seq(key::backspace), "delete-char"},
    {key::seq(key::backspace_),"delete-char"},
    {key::seq(key::delete_),   "delete-char-right"},
    {key::seq(key::home),      "move-beginning-of-line"},
    {key::seq(key::ctrl('a')), "move-beginning-of-line"},
    {key::seq(key::end),       "move-end-of-line"},
    {key::seq(key::ctrl('e')), "move-end-of-line"},
    {key::seq(key::ctrl('i')), "insert-tab"}, // tab
    {key::seq(key::ctrl('j')), "new-line"}, // enter
    {key::seq(key::ctrl('k')), "kill-line"},
    {key::seq(key::ctrl('w')), "cut"},
    {key::seq(key::ctrl('y')), "paste"},
    {key::seq(key::ctrl('@')), "start-selection"}, // ctrl-space
    {key::seq(key::ctrl('_')), "undo"},
    {key::seq(key::ctrl('x'), key::ctrl('C')), "quit"},
    {key::seq(key::ctrl('x'), key::ctrl('S')), "save"},
    {key::seq(key::ctrl('x'), 'h'), "select-whole-buffer"},
    {key::seq(key::ctrl('x'), '['), "move-beginning-buffer"},
    {key::seq(key::ctrl('x'), ']'), "move-end-buffer"},
    {key::seq(key::alt('w')),  "copy"},
});
```

License
-------

This software is licensed under the
[GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.en.html).

    Copyright (C) 2016 Juan Pedro Bolivar Puente

    This file is part of ewig.

    ewig is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ewig is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ewig.  If not, see <http://www.gnu.org/licenses/>.

[![GPL3 Logo](https://www.gnu.org/graphics/gplv3-127x51.png)](https://www.gnu.org/licenses/gpl-3.0.en.html)
