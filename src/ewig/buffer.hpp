//
// ewig - an immutable text editor
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of ewig.
//
// ewig is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ewig is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with ewig.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <ewig/coord.hpp>

#include <lager/store.hpp>
#include <lager/extra/struct.hpp>

#include <immer/box.hpp>
#include <immer/flex_vector.hpp>
#include <immer/flex_vector_transient.hpp>
#include <immer/vector.hpp>
#include <immer/algorithm.hpp>

#include <utf8.h>
#include <boost/range/iterator_range.hpp>

#include <optional>
#include <variant>

namespace ewig {

using line = immer::flex_vector<char>;
using text = immer::flex_vector<line>;

struct no_file
{
    immer::box<std::string> name = "*unnamed*";
    text content = {};
};

struct existing_file
{
    immer::box<std::string> name;
    text content;
};

struct saving_file
{
    immer::box<std::string> name;
    text content;
    std::size_t saved_lines;
};

struct loading_file
{
    immer::box<std::string> name;
    text content;
    std::streamoff loaded_bytes;
    std::streamoff total_bytes;
};

using file = std::variant<no_file,
                          existing_file,
                          loading_file,
                          saving_file>;

struct snapshot
{
    text content;
    coord cursor;
};

struct buffer
{
    file from;
    text content;
    coord cursor;
    coord scroll;
    std::optional<coord> selection_start;
    immer::vector<snapshot> history;
    std::optional<std::size_t> history_pos;
};

struct load_progress_action { loading_file file; };
struct load_done_action { existing_file file; };
struct load_error_action { existing_file file; std::exception_ptr err; };
struct save_progress_action { saving_file file; };
struct save_done_action { existing_file file; };
struct save_error_action { existing_file file; std::exception_ptr err; };

using buffer_action = std::variant<load_progress_action,
                                   load_done_action,
                                   load_error_action,
                                   save_progress_action,
                                   save_done_action,
                                   save_error_action>;

constexpr auto tab_width = 8;

/** Returns the number of actual characters in the line `ln` */
index line_length(const line& ln);

/** Returns the offsets where character `col` is located in `ln` */
std::size_t line_char(const line& ln, index col);

/**
 * Returns the [begin, end) offsets where character `col` is located
 * in the line `ln`.
 */
std::pair<std::size_t, std::size_t> line_char_region(const line& ln, index col);

/**
 * Returns a range to iterate over the code points of a line.
 */
inline auto line_range(const line& ln)
{
    return boost::make_iterator_range(
        utf8::unchecked::iterator(ln.begin()),
        utf8::unchecked::iterator(ln.end()));
}

line get_line(const text& txt, index row);

bool io_in_progress(const buffer&);
bool load_in_progress(const buffer&);
bool is_dirty(const buffer& buf);

std::pair<buffer, std::string> update_buffer(buffer buf, buffer_action ac);

std::pair<buffer, lager::effect<buffer_action>> load_buffer(buffer, const std::string& fname);
std::pair<buffer, lager::effect<buffer_action>> save_buffer(buffer buf);

index expand_tabs(const line& ln, index col);

buffer page_up(buffer buf, coord size);
buffer page_down(buffer buf, coord size);

buffer move_line_start(buffer buf);
buffer move_line_end(buffer buf);
buffer move_buffer_start(buffer buf);
buffer move_buffer_end(buffer buf);

buffer move_cursor_up(buffer buf);
buffer move_cursor_down(buffer buf);
buffer move_cursor_left(buffer buf);
buffer move_cursor_right(buffer buf);

buffer scroll_to_cursor(buffer buf, coord wsize);

buffer delete_char(buffer buf);
buffer delete_char_right(buffer buf);

buffer insert_new_line(buffer buf);
buffer insert_tab(buffer buf);
buffer insert_char(buffer buf, wchar_t value);
buffer insert_text(buffer buf, text value);

std::pair<buffer, text> copy(buffer buf);
std::pair<buffer, text> cut(buffer buf);
std::pair<buffer, text> cut_rest(buffer buf);

buffer select_whole_buffer(buffer buf);
buffer start_selection(buffer buf);
buffer clear_selection(buffer buf);
std::tuple<coord, coord> selected_region(buffer buf);

buffer undo(buffer);
std::pair<buffer, std::string> record(buffer before, buffer after);

} // namespace ewig

LAGER_STRUCT(ewig, no_file, name, content);
LAGER_STRUCT(ewig, existing_file, name, content);
LAGER_STRUCT(ewig, saving_file, name, content, saved_lines);
LAGER_STRUCT(ewig, loading_file, name, content, loaded_bytes, total_bytes);
LAGER_STRUCT(ewig, snapshot, content, cursor);
LAGER_STRUCT(ewig, buffer, from, content, cursor, scroll, selection_start, history, history_pos);
LAGER_STRUCT(ewig, load_progress_action, file);
LAGER_STRUCT(ewig, load_done_action, file);
LAGER_STRUCT(ewig, load_error_action, file, err);
LAGER_STRUCT(ewig, save_progress_action, file);
LAGER_STRUCT(ewig, save_done_action, file);
LAGER_STRUCT(ewig, save_error_action, file, err);
