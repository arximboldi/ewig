
Ewig
====

Ewig is a text editor written
using [immutable data-structures](https://sinusoid.es/immer/) in C++.

The code is written in a simple style to showcase a value-based
functional architecture.  We invite you to [study the code](https://github.com/arximboldi/ewig/tree/master/src/ewig).

<p align="center">
  <a href="https://asciinema.org/a/3i7smcgv5g52314lklmksqdxt">
    <img src="https://cdn.rawgit.com/arximboldi/ewig/9ffbe95f/doc/ewig.gif">
  </a>
</p>

Try it out
----------

To build the code you need a C++14, `cmake`, and `ncurses` with
unicode support (package `libncursesw5-dev` in Debian and friends).
Then type:

#### Prepare
```
git submodule update --recursive --init
mkdir build
cd build
```

#### Configure and build
```
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

#### Install
```
sudo make install
```

Keybindings
-----------

Excerpt from
[`tui.cpp`](https://github.com/arximboldi/ewig/blob/master/src/ewig/tui.cpp):
```cpp
const auto key_map_emacs = make_key_map(
{
    {key::seq(key::up),        "move-up"},
    {key::seq(key::down),      "move-down"},
    {key::seq(key::left),      "move-left"},
    {key::seq(key::right),     "move-right"},
    {key::seq(key::page_down), "page-down"},
    {key::seq(key::page_up),   "page-up"},
    {key::seq(key::backspace), "delete-char"},
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
