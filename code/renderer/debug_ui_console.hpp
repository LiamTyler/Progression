#pragma once

#include "core/console_commands.hpp"
#include "core/feature_defines.hpp"
#include "imgui/imgui.h"
#include <string>
#include <vector>

namespace PG::Gfx::UIOverlay
{

class Console
{
    enum LogType : uint8_t
    {
        LOG,
        WARN,
        ERR
    };

    struct LogItem
    {
        char* text;
        LogType type;
    };

public:
    char m_inputBuffer[256];
    std::vector<LogItem> m_logItems;
    std::vector<ConsoleCmd> m_commands;
    std::vector<char*> m_history;
    int m_historyPos; // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter m_filter;
    bool m_autoScroll;
    bool m_scrollToBottom;
    bool m_visible;

    Console();
    ~Console();

    void ToggleVisibility();
    void ClearLog();
    void HelpCommand( std::string_view cmd );
    void AddLog( LogType type, const char* fmt, ... );
    void Draw();
    void ExecCommand( const char* command_line );
    static int TextEditCallbackStub( ImGuiInputTextCallbackData* data );
    int TextEditCallback( ImGuiInputTextCallbackData* data );
};

} // namespace PG::Gfx::UIOverlay
