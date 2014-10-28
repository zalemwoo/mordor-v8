// Copyright 2012 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>  // NOLINT
#include <string.h> // NOLINT
#include <readline/readline.h> // NOLINT
#include <readline/history.h> // NOLINT

// The readline includes leaves RETURN defined which breaks V8 compilation.
#undef RETURN

#include "mordor/streams/buffer.h"
#include "md_runner.h"

// There are incompatibilities between different versions and different
// implementations of readline.  This smooths out one known incompatibility.
#if RL_READLINE_VERSION >= 0x0500
#define completion_matches rl_completion_matches
#endif

namespace Mordor
{

namespace Test
{

class ReadLineEditor: public LineEditor
{
public:
    ReadLineEditor() :
            LineEditor(LineEditor::READLINE, "readline")
    {
    }
    virtual std::string Prompt(const char* prompt);
    virtual bool Open();
    virtual bool Close();
    virtual void AddHistory(const char* str);

    static const char* kHistoryFileName;
    static const int kMaxHistoryEntries;

private:
    static char kWordBreakCharacters[];
    Mordor::Buffer buf;
};

static ReadLineEditor read_line_editor;
char ReadLineEditor::kWordBreakCharacters[] = { ' ', '\t', '\n', '"', '\\', '\'', '`', '@', '.', '>', '<', '=', ';',
        '|', '&', '{', '(', '\0' };

const char* ReadLineEditor::kHistoryFileName = ".mordor_v8_history";
const int ReadLineEditor::kMaxHistoryEntries = 1000;

bool ReadLineEditor::Open()
{
    rl_initialize();
    rl_completer_word_break_characters = kWordBreakCharacters;
    rl_bind_key('\t', rl_complete);
    using_history();
    stifle_history(kMaxHistoryEntries);
    return read_history(kHistoryFileName) == 0;
}

bool ReadLineEditor::Close()
{
    bool ret = ( write_history(kHistoryFileName) == 0 );
    clear_history();
    return ret;
}

std::string ReadLineEditor::Prompt(const char* prompt)
{
    buf.clear(true);
    char* result = NULL;
    result = readline(prompt);
    if (result == NULL)
        return std::string();
    AddHistory(result);
    buf.copyIn(result);
    free(result);
    return buf.toString();
}

void ReadLineEditor::AddHistory(const char* str)
{
    // Do not record empty input.
    if (strlen(str) == 0)
        return;
    // Remove duplicate history entry.
    history_set_pos(history_length - 1);
    if (current_history()) {
        do {
            if (strcmp(current_history()->line, str) == 0) {
                remove_history(where_history());
                break;
            }
        } while (previous_history());
    }
    add_history(str);
}

} } // namespace Mordor::Test
