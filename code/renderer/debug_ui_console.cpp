#include "debug_ui_console.hpp"

#if USING( PG_DEBUG_UI )
#include "core/console_commands.hpp"
#include "core/dvars.hpp"
#include "core/window.hpp"
#include "shared/assert.hpp"
#include "shared/string.hpp"
#include <cctype>
#include <cstdio>
#include <cstdlib>

// Portable helpers
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
    m_logItems.reserve( 256 );

    m_commands = GetConsoleCommands();

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
    for ( size_t i = 0; i < m_logItems.size(); i++ )
        free( m_logItems[i].text );
    m_logItems.clear();
}

void Console::HelpCommand( std::string_view cmdName )
{
    if ( cmdName == "clear" )
        AddLog( LogType::LOG, "  clears this log window\n" );
    else if ( cmdName == "help" )
        AddLog( LogType::LOG, "  usage: help [commandName]\n" );
    else
    {
        const auto& dvarMap = GetAllDvars();
        auto it             = dvarMap.find( cmdName );
        if ( it != dvarMap.end() )
        {
            AddLog( LogType::LOG, "  %s\n", it->second->GetDescription() );
            return;
        }
        for ( size_t i = 0; i < m_commands.size(); ++i )
        {
            const ConsoleCmd& cmd = m_commands[i];
            if ( cmd.name == cmdName )
            {
                AddLog( LogType::LOG, "  %s\n", cmd.usage.data() );
                return;
            }
        }

        AddLog( LogType::WARN, "  Unknown command '%s'\n", cmdName.data() );
    }
}

void Console::AddLog( LogType type, const char* fmt, ... )
{
    char buf[1024];
    va_list args;
    va_start( args, fmt );
    vsnprintf( buf, IM_ARRAYSIZE( buf ), fmt, args );
    buf[IM_ARRAYSIZE( buf ) - 1] = 0;
    va_end( args );
    m_logItems.emplace_back( Strdup( buf ), type );
}

void Console::Draw()
{
    if ( !m_visible )
        return;

    f32 pad           = 10;
    f32 consoleHeight = 200;
    Window* appWindow = GetMainWindow();
    ImGui::SetNextWindowSize( ImVec2( appWindow->Width() - 2 * pad, consoleHeight ) );
    ImGui::SetNextWindowPos( ImVec2( pad, appWindow->Height() - consoleHeight - pad ) );
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
    const f32 footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
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
        for ( const LogItem& item : m_logItems )
        {
            if ( !m_filter.PassFilter( item.text ) )
                continue;

            if ( item.type == LogType::LOG )
            {
                ImGui::TextUnformatted( item.text );
                continue;
            }

            ImVec4 color = item.type == LogType::ERR ? ImVec4( 1.0f, 0.4f, 0.4f, 1.0f ) : ImVec4( 1.0f, 0.8f, 0.6f, 1.0f );
            ImGui::PushStyleColor( ImGuiCol_Text, color );
            ImGui::TextUnformatted( item.text );
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
    AddLog( LogType::LOG, "# %s\n", fullCommand );
    m_scrollToBottom = true;

    // Insert into history. First find match and delete it so it can be pushed to the back.
    // This isn't trying to be smart or optimal.
    m_historyPos = -1;
    for ( i32 i = (i32)m_history.size() - 1; i >= 0; i-- )
    {
        if ( !strcmp( m_history[i], fullCommand ) )
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
            AddLog( LogType::LOG, "  Dvar %s = %s\n", dvar->GetName(), dvar->GetValueAsString().c_str() );
            if ( args.size() != 1 )
                AddLog( LogType::WARN, "  Hint: use the 'set' command to set dvars. Usage: set [dvarName] [value]\n" );
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
            AddLog( LogType::WARN, "  Usage: help [commandName]\n" );
        else
            HelpCommand( args[1] );
    }
    else if ( cmd == "set" )
    {
        if ( args.size() < 2 )
        {
            AddLog( LogType::WARN, "  Usage: set [dvarName] [value]\n" );
            return;
        }
        auto it = dvarMap.find( args[1] );
        if ( it == dvarMap.end() )
        {
            AddLog( LogType::WARN, "  No dvar named '%s' found\n", args[1].data() );
            return;
        }
        Dvar* dvar = it->second;
        if ( args.size() == 2 )
        {
            AddLog( LogType::WARN, "  Missing a value. Usage: set [dvarName] [value]. Description:%s\n", dvar->GetDescription() );
            return;
        }

        if ( args.size() == 3 || dvar->GetType() != Dvar::Type::VEC4 )
        {
            dvar->SetFromString( std::string( args[2] ) );
        }
        else
        {
            std::string combinedArgs = std::string( args[2] );
            for ( size_t argIdx = 3; argIdx < args.size(); ++argIdx )
                combinedArgs += " " + std::string( args[argIdx] );
            dvar->SetFromString( combinedArgs );
        }
        AddLog( LogType::LOG, "  Dvar %s = %s\n", dvar->GetName(), args[2].data() );
    }
    else
    {
        for ( size_t i = 0; i < m_commands.size(); ++i )
        {
            if ( cmd == m_commands[i].name )
            {
                AddConsoleCommand( fullCommand );
                return;
            }
        }
        AddLog( LogType::WARN, "Unknown command: '%s'\n", cmd.data() );
    }
}

i32 Console::TextEditCallbackStub( ImGuiInputTextCallbackData* data )
{
    Console* console = (Console*)data->UserData;
    return console->TextEditCallback( data );
}

i32 Console::TextEditCallback( ImGuiInputTextCallbackData* data )
{
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
        i32 wordLen = (i32)( word_end - word_start );
        for ( size_t i = 0; i < m_commands.size(); i++ )
            if ( Strncmp( m_commands[i].name.data(), word_start, wordLen ) == 0 )
                candidates.push_back( m_commands[i].name.data() );
        const auto& dvarMap = GetAllDvars();
        for ( const auto& [dvarName, _] : dvarMap )
        {
            if ( Strncmp( dvarName.data(), word_start, wordLen ) == 0 )
                candidates.push_back( dvarName.data() );
        }

        if ( candidates.Size == 0 )
        {
            AddLog( LogType::LOG, "No match for \"%.*s\"!\n", wordLen, word_start );
        }
        else if ( candidates.Size == 1 )
        {
            // Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
            data->DeleteChars( (i32)( word_start - data->Buf ), wordLen );
            data->InsertChars( data->CursorPos, candidates[0] );
            data->InsertChars( data->CursorPos, " " );
        }
        else
        {
            // Multiple matches. Complete as much as we can..
            // So inputing "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
            i32 match_len = wordLen;
            for ( ;; )
            {
                i32 c                       = 0;
                bool all_candidates_matches = true;
                for ( i32 i = 0; i < candidates.Size && all_candidates_matches; i++ )
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
                data->DeleteChars( (i32)( word_start - data->Buf ), wordLen );
                data->InsertChars( data->CursorPos, candidates[0], candidates[0] + match_len );
            }

            // List matches
            AddLog( LogType::LOG, "Possible matches:\n" );
            for ( i32 i = 0; i < candidates.Size; i++ )
                AddLog( LogType::LOG, "- %s\n", candidates[i] );
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        // Example of HISTORY
        const i32 prev_history_pos = m_historyPos;
        if ( data->EventKey == ImGuiKey_UpArrow )
        {
            if ( m_historyPos == -1 )
                m_historyPos = (i32)m_history.size() - 1;
            else if ( m_historyPos > 0 )
                m_historyPos--;
        }
        else if ( data->EventKey == ImGuiKey_DownArrow )
        {
            if ( m_historyPos != -1 )
                if ( ++m_historyPos >= (i32)m_history.size() )
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
#endif // #if USING( PG_DEBUG_UI )
