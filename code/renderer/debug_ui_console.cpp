#include "debug_ui_console.hpp"
#include "core/console_commands.hpp"
#include "core/dvars.hpp"
#include "core/window.hpp"
#include "shared/string.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>

// Portable helpers
static int Stricmp( const char* s1, const char* s2 )
{
    int d;
    while ( ( d = toupper( *s2 ) - toupper( *s1 ) ) == 0 && *s1 )
    {
        s1++;
        s2++;
    }
    return d;
}
static int Strnicmp( const char* s1, const char* s2, int n )
{
    int d = 0;
    while ( n > 0 && ( d = toupper( *s2 ) - toupper( *s1 ) ) == 0 && *s1 )
    {
        s1++;
        s2++;
        n--;
    }
    return d;
}
static char* Strdup( const char* s )
{
    IM_ASSERT( s );
    size_t len = strlen( s ) + 1;
    void* buf  = malloc( len );
    IM_ASSERT( buf );
    return (char*)memcpy( buf, (const void*)s, len );
}
static void Strtrim( char* s )
{
    char* str_end = s + strlen( s );
    while ( str_end > s && str_end[-1] == ' ' )
        str_end--;
    *str_end = 0;
}

namespace PG::Gfx::UIOverlay
{

Console::Console()
{
    ClearLog();
    memset( m_inputBuffer, 0, sizeof( m_inputBuffer ) );
    m_historyPos = -1;
    m_items.reserve( 256 );

    m_commands.emplace_back( "clear", "clears this log window", CmdType::LOCAL );
    m_commands.emplace_back( "help", "usage: help [commandName]", CmdType::LOCAL );

    std::vector<ConsoleCmd> cmds = GetConsoleCommands();
    for ( const ConsoleCmd& cmd : cmds )
        m_commands.emplace_back( cmd.name, cmd.usage, CmdType::REGULAR_CMD );

    m_commandNames.resize( m_commands.size() );
    for ( size_t i = 0; i < m_commands.size(); ++i )
        m_commandNames[i] = m_commands[i].name.data();

    m_autoScroll     = true;
    m_scrollToBottom = false;
    m_visible        = false;
}

Console::~Console()
{
    ClearLog();
    for ( size_t i = 0; i < m_history.size(); i++ )
        free( m_history[i] );
}

void Console::ToggleVisibility() { m_visible = !m_visible; }

void Console::ClearLog()
{
    for ( size_t i = 0; i < m_items.size(); i++ )
        free( m_items[i] );
    m_items.clear();
}

void Console::HelpCommand( std::string_view cmd ) const {}

void Console::AddLog( const char* fmt, ... )
{
    // FIXME-OPT
    char buf[1024];
    va_list args;
    va_start( args, fmt );
    vsnprintf( buf, IM_ARRAYSIZE( buf ), fmt, args );
    buf[IM_ARRAYSIZE( buf ) - 1] = 0;
    va_end( args );
    m_items.push_back( Strdup( buf ) );
}

void Console::Draw()
{
    if ( !m_visible )
        return;

    float consoleHeight = 200;
    Window* appWindow   = GetMainWindow();
    ImGui::SetNextWindowSize( ImVec2( appWindow->Width() - 20.0f, consoleHeight ) );
    ImGui::SetNextWindowPos( ImVec2( 10.0f, appWindow->Height() - consoleHeight ) );
    if ( !ImGui::Begin( "console" ) )
    {
        ImGui::End();
        return;
    }

    if ( ImGui::IsWindowFocused( ImGuiFocusedFlags_AnyWindow ) )
    {
        printf( "" );
    }

    ImGui::SameLine();

    ImGui::Separator();

    // Options menu
    if ( ImGui::BeginPopup( "Options" ) )
    {
        ImGui::Checkbox( "Auto-scroll", &m_autoScroll );
        ImGui::EndPopup();
    }

    // Options, Filter
    if ( ImGui::Button( "Options" ) )
        ImGui::OpenPopup( "Options" );
    ImGui::SameLine();
    m_filter.Draw( "Filter", 180 );
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    if ( ImGui::BeginChild(
             "ScrollingRegion", ImVec2( 0, -footer_height_to_reserve ), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar ) )
    {
        if ( ImGui::BeginPopupContextWindow() )
        {
            if ( ImGui::Selectable( "Clear" ) )
                ClearLog();
            ImGui::EndPopup();
        }

        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 4, 1 ) ); // Tighten spacing
        for ( const char* item : m_items )
        {
            if ( !m_filter.PassFilter( item ) )
                continue;

            // Normally you would store more information in your item than just a string.
            // (e.g. make Items[] an array of structure, store color/type etc.)
            ImVec4 color;
            bool has_color = false;
            if ( strstr( item, "[error]" ) )
            {
                color     = ImVec4( 1.0f, 0.4f, 0.4f, 1.0f );
                has_color = true;
            }
            else if ( strncmp( item, "# ", 2 ) == 0 )
            {
                color     = ImVec4( 1.0f, 0.8f, 0.6f, 1.0f );
                has_color = true;
            }
            if ( has_color )
                ImGui::PushStyleColor( ImGuiCol_Text, color );
            ImGui::TextUnformatted( item );
            if ( has_color )
                ImGui::PopStyleColor();
        }

        // Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
        // Using a scrollbar or mouse-wheel will take away from the bottom edge.
        if ( m_scrollToBottom || ( m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() ) )
            ImGui::SetScrollHereY( 1.0f );
        m_scrollToBottom = false;

        ImGui::PopStyleVar();
    }
    ImGui::EndChild();
    ImGui::Separator();

    // Command-line
    bool reclaim_focus                   = false;
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll |
                                           ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    if ( ImGui::IsWindowAppearing() )
        ImGui::SetKeyboardFocusHere();
    if ( ImGui::InputText( "Input", m_inputBuffer, IM_ARRAYSIZE( m_inputBuffer ), input_text_flags, &TextEditCallbackStub, (void*)this ) )
    {
        char* s = m_inputBuffer;
        Strtrim( s );
        if ( s[0] )
            ExecCommand( s );
        strcpy( s, "" );
        reclaim_focus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();
    if ( reclaim_focus )
        ImGui::SetKeyboardFocusHere( -1 ); // Auto focus previous widget

    ImGui::End();
}

void Console::ExecCommand( const char* fullCommand )
{
    AddLog( "# %s\n", fullCommand );
    m_scrollToBottom = true;

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    m_historyPos = -1;
    for ( int i = (int)m_history.size() - 1; i >= 0; i-- )
    {
        if ( Stricmp( m_history[i], fullCommand ) == 0 )
        {
            free( m_history[i] );
            m_history.erase( m_history.begin() + i );
            break;
        }
    }
    m_history.push_back( Strdup( fullCommand ) );

    std::vector<std::string_view> args = SplitString( fullCommand, " " );
    PG_ASSERT( args.size() >= 1 );
    std::string_view cmd = args[0];

    const auto& dvarMap = GetAllDvars();
    {
        auto it = dvarMap.find( cmd );
        if ( it != dvarMap.end() )
        {
            Dvar* dvar = it->second;
            AddLog( "  %s\n", dvar->GetValueAsString().c_str() );
            return;
        }
    }

    if ( cmd == "clear" )
    {
        ClearLog();
    }
    else if ( cmd == "help" )
    {
        if ( args.size() < 2 )
            AddLog( "  Usage: help [commandName]\n" );
        else
            HelpCommand( args[1] );
    }
    else if ( cmd == "set" )
    {
        if ( args.size() != 3 )
        {
            AddLog( "  Usage: set [commandName] [value]\n" );
            return;
        }
        auto it = dvarMap.find( args[1] );
        if ( it == dvarMap.end() )
            AddLog( "  No dvar named '%s' found\n", args[1].data() );
        else
            AddLog( "  Dvar %s = %s\n", args[1].data(), it->second->GetValueAsString().c_str() );
    }
    else
    {
        // for ( size_t i = 0; i < m_commands.size(); ++i )
        //{
        // }
        AddLog( "Unknown command: '%s'\n", cmd.data() );
    }
}

int Console::TextEditCallbackStub( ImGuiInputTextCallbackData* data )
{
    Console* console = (Console*)data->UserData;
    return console->TextEditCallback( data );
}

int Console::TextEditCallback( ImGuiInputTextCallbackData* data )
{
    // AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
    switch ( data->EventFlag )
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Example of TEXT COMPLETION

        // Locate beginning of current word
        const char* word_end   = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while ( word_start > data->Buf )
        {
            const char c = word_start[-1];
            if ( c == ' ' || c == '\t' || c == ',' || c == ';' )
                break;
            word_start--;
        }

        // Build a list of candidates
        ImVector<const char*> candidates;
        candidates.reserve( 32 );
        for ( size_t i = 0; i < m_commandNames.size(); i++ )
            if ( Strnicmp( m_commandNames[i], word_start, (int)( word_end - word_start ) ) == 0 )
                candidates.push_back( m_commandNames[i] );

        if ( candidates.Size == 0 )
        {
            // No match
            AddLog( "No match for \"%.*s\"!\n", (int)( word_end - word_start ), word_start );
        }
        else if ( candidates.Size == 1 )
        {
            // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
            data->DeleteChars( (int)( word_start - data->Buf ), (int)( word_end - word_start ) );
            data->InsertChars( data->CursorPos, candidates[0] );
            data->InsertChars( data->CursorPos, " " );
        }
        else
        {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
            int match_len = (int)( word_end - word_start );
            for ( ;; )
            {
                int c                       = 0;
                bool all_candidates_matches = true;
                for ( int i = 0; i < candidates.Size && all_candidates_matches; i++ )
                    if ( i == 0 )
                        c = toupper( candidates[i][match_len] );
                    else if ( c == 0 || c != toupper( candidates[i][match_len] ) )
                        all_candidates_matches = false;
                if ( !all_candidates_matches )
                    break;
                match_len++;
            }

            if ( match_len > 0 )
            {
                data->DeleteChars( (int)( word_start - data->Buf ), (int)( word_end - word_start ) );
                data->InsertChars( data->CursorPos, candidates[0], candidates[0] + match_len );
            }

            // List matches
            AddLog( "Possible matches:\n" );
            for ( int i = 0; i < candidates.Size; i++ )
                AddLog( "- %s\n", candidates[i] );
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        // Example of HISTORY
        const int prev_history_pos = m_historyPos;
        if ( data->EventKey == ImGuiKey_UpArrow )
        {
            if ( m_historyPos == -1 )
                m_historyPos = (int)m_history.size() - 1;
            else if ( m_historyPos > 0 )
                m_historyPos--;
        }
        else if ( data->EventKey == ImGuiKey_DownArrow )
        {
            if ( m_historyPos != -1 )
                if ( ++m_historyPos >= (int)m_history.size() )
                    m_historyPos = -1;
        }

        // A better implementation would preserve the data on the current input line along with cursor position.
        if ( prev_history_pos != m_historyPos )
        {
            const char* history_str = ( m_historyPos >= 0 ) ? m_history[m_historyPos] : "";
            data->DeleteChars( 0, data->BufTextLen );
            data->InsertChars( 0, history_str );
        }
    }
    }
    return 0;
}

} // namespace PG::Gfx::UIOverlay
