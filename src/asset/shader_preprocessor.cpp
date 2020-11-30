#include "asset/shader_preprocessor.hpp"
#include "asset/types/shader.hpp"
#include "core/assert.hpp"
#include "utils/filesystem.hpp"
#include "utils/logger.hpp"
#include <fstream>
#include <sstream>
#include <stack>
#include <unordered_map>


bool IsPreprocLine( const std::string& line, int& index )
{
    int len = static_cast< int >( line.length() );
    index = 0;
    while ( index < len && isspace( line[index] ) ) ++index;

    if ( index < len && line[index] == '#' )
    {
        ++index;
        return true;
    }

    return false;
}


std::string GetNextToken( const char* str, int len, int& index )
{
    while ( index < len && isspace( str[index] ) ) ++index;

    int start = index;
    if ( start == len )
    {
        return "";
    }
    while ( index < len && !isspace( str[index] ) ) ++index;

    return std::string( str + start, index - start );
}


std::string GetNextToken( const std::string& str, int& index )
{
    return GetNextToken( str.c_str(), static_cast< int >( str.length() ), index );
}


bool GetStringInteger( const std::string& str, int& result )
{
    if ( str.length() == 1 && std::isalpha( str[0] ) )
    {
        result = str[0];
        return true;
    }
    size_t i = 0;
    if ( str[0] == '-' )
    {
        if ( str.length() > 1 )
        {
            if ( std::isdigit( str[1] ) )
            {
                i = 1;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    for ( ; i < str.length(); ++i )
    {
        if ( !std::isdigit( str[i] ) )
        {
            return false;
        }
    }

    result = std::stoi( str );
    return true;
}


bool EvaluateBinaryOp( int a, const std::string& op, int b, int& result )
{
    size_t opLen = op.length();
    switch ( op[0] )
    {
        case '=':
            result = a == b;
            break;
        case '!':
            result = a != b;
            break;
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            result = a / b;
            break;
        case '<':
            if ( opLen == 1 )
            {
                result = a < b;
            }
            else if ( opLen == 2 && op[1] == '=' )
            {
                result = a <= b;
            }
            else
            {
                return false;
            }
            break;
        case '>':
            if ( opLen == 1 )
            {
                result = a > b;
            }
            else if ( opLen == 2 && op[1] == '=' )
            {
                result = a >= b;
            }
            else
            {
                return false;
            }
            break;
        default:
            return false;
    }

    return true;
}


namespace PG
{


class ShaderPreprocessor
{
public:
    ShaderPreprocessor() = default;

    template< bool includesOnly >
    bool Preprocess( const std::string& pathToShader, const DefineList& defineList )
    {
        Init( pathToShader, defineList );
        bool success = PreprocessInternal< includesOnly >( pathToShader );
        if ( !m_ifStatementStack.empty() )
        {
            LOG_ERR( "Mismatched #if / #endifs, expecting %d more #endif's in shader '%s'\n", m_ifStatementStack.size(), pathToShader.c_str() );
            return false;
        }

        return success;
    }

    std::string outputShader;
    std::vector< std::string > includedFiles;

private:

    enum class IfStatus : uint8_t
    {
        CURRENTLY_FALSE_PREVIOUSLY_FALSE,
        CURRENTLY_FALSE_PREVIOUSLY_TRUE,
        CURRENTLY_TRUE_PREVIOUSLY_FALSE,
    };

    std::string m_rootFilename;
    std::string m_currentFilename;
    uint32_t m_currentLineNumber;
    std::vector< std::string > m_includeSearchDirs;
    std::unordered_map< std::string, std::string > m_defines;
    std::stack< IfStatus > m_ifStatementStack;
    int m_inactiveIfs;


    void Init( const std::string& pathToShader, const DefineList& defineList )
    {
        m_rootFilename = pathToShader;
        m_includeSearchDirs =
        {
            PG_ASSET_DIR "shaders/",
            GetParentPath( pathToShader ),
        };
        for ( const auto& [define, value] : defineList )
        {
            AddDefine( define, value );
        }

        m_ifStatementStack = {};
        m_inactiveIfs = 0;
    }


    void AddDefine( const std::string& define, const std::string& value )
    {
        auto it = m_defines.find( define );
        if ( it != m_defines.end() )
        {
            LOG_WARN( "Already have #define '%s' '%s'. Updating value with '%s'\n", define.c_str(), it->second.c_str(), value.c_str() );
        }
        m_defines[define] = value;
    }


    bool RemoveDefine( const std::string& define )
    {
        auto it = m_defines.find( define );
        if ( it != m_defines.end() )
        {
            m_defines.erase( define );
            return true;
        }
        else
        {
            LOG_ERR( "No previous #define for '%s'\n", define.c_str() );
            return false;
        }
    }


    bool GetDefine( const std::string& define, std::string& value )
    {
        auto it = m_defines.find( define );
        if ( it != m_defines.end() )
        {
            value = it->second;
            return true;
        }
        else
        {
            value = "0";
            return false;
        }
    }


    bool GetIncludePath( const std::string& shaderIncludeLine, std::string& absolutePath, int startPos = 0 ) const
    {
        size_t includeStartPos = shaderIncludeLine.find( '"', startPos ) + 1;
        size_t includeEndPos = shaderIncludeLine.find( '"', includeStartPos );
        if ( includeEndPos == std::string::npos )
        {
            LOG_ERR( "Invalid include on line '%s'\n", shaderIncludeLine.c_str() );
            return false;
        }
        std::string relativeInclude = shaderIncludeLine.substr( includeStartPos, includeEndPos - includeStartPos );

        std::string absoluteIncludePath;
        for ( const auto& dir : m_includeSearchDirs )
        {
            std::string filename = dir + relativeInclude;
            if ( PathExists( filename ) )
            {
                absolutePath = filename;
                return true;
            }
        }
        LOG_ERR( "Could not find include '%s'\n", shaderIncludeLine.c_str() );

        return false;
    }


    bool Active() const
    {
        return m_inactiveIfs == 0;
    }


    // could have nested defines. #define A 1 -> #define B A -> #if B == 2
    bool GetDefineNum( std::string define, int& result )
    {
        std::string value;
        while ( GetDefine( define, value ) )
        {
            define = value;
        }

        return GetStringInteger( define, result );
    };

    // returns true if expression was able to be parsed and evaluated, false if errors.
    // result holds the actual result of the expression
    // Only works for simple expressions "A op B" or just "A"
    bool EvaulateExpression( const std::string& str, int startIndex, bool& result )
    {
        constexpr int MAX_TOKENS = 6;
        std::string tokens[MAX_TOKENS];
        tokens[0] = GetNextToken( str, startIndex );
        int numValidTokens = 0;
        bool success = true;
        int expressionIntResult = 0;
        while ( !tokens[numValidTokens].empty() && numValidTokens < MAX_TOKENS - 1 )
        {
            numValidTokens += 1;
            tokens[numValidTokens] = GetNextToken( str, startIndex );
        }        

        if ( numValidTokens == 1 )
        {
            success = GetDefineNum( tokens[0], expressionIntResult );
        }
        else if ( numValidTokens == 3 )
        {
            int value1;
            int value2;
            success = GetDefineNum( tokens[0], value1 );
            success = success && GetDefineNum( tokens[2], value2 );            
            success = success && EvaluateBinaryOp( value1, tokens[1], value2, expressionIntResult );
        }

        result = expressionIntResult != 0;
        if ( !success )
        {
            LOG_ERR( "Could not evaluate expression '%s' on line %d of file '%s'\n", str.c_str(), m_currentLineNumber, m_currentFilename.c_str() );
        }
        return success;
    }


    template< bool includesOnly >
    bool ProcessActivePreprocStatment( const std::string& line, uint32_t lineNumber, const std::string& command, const std::string& path, int endOfTokenIndex )
    {
        constexpr int CASE_DEFINE       = 1000000 * 'd' + 1000 * 'e' + 'f';
        constexpr int CASE_ELSE         = 1000000 * 'e' + 1000 * 'l' + 's';
        constexpr int CASE_ELIF         = 1000000 * 'e' + 1000 * 'l' + 'i';
        constexpr int CASE_ENDIF        = 1000000 * 'e' + 1000 * 'n' + 'd';
        constexpr int CASE_ERROR        = 1000000 * 'e' + 1000 * 'r' + 'r';
        constexpr int CASE_EXTENSION    = 1000000 * 'e' + 1000 * 'x' + 't';
        constexpr int CASE_IF           = 1000000 * 'i' + 1000 * 'f';
        constexpr int CASE_IFDEF        = 1000000 * 'i' + 1000 * 'f' + 'd';
        constexpr int CASE_IFNDEF       = 1000000 * 'i' + 1000 * 'f' + 'n';
        constexpr int CASE_INCLUDE      = 1000000 * 'i' + 1000 * 'n' + 'c';
        constexpr int CASE_UNDEF        = 1000000 * 'u' + 1000 * 'n' + 'd';
        constexpr int CASE_VERSION      = 1000000 * 'v' + 1000 * 'e' + 'r';
        constexpr int CASE_WARNING      = 1000000 * 'w' + 1000 * 'a' + 'r';

        int label = 1000000 * command[0] + 1000 * command[1];
        label += command.length() > 2 ? command[2] : 0;

        if ( !Active() )
        {
            switch ( label )
            {
                case CASE_ELSE:
                {
                    if ( m_inactiveIfs > 1 )
                    {
                        break;
                    }

                    IfStatus status = m_ifStatementStack.top();
                    m_ifStatementStack.pop();
                    if ( status == IfStatus::CURRENTLY_FALSE_PREVIOUSLY_FALSE )
                    {
                        m_ifStatementStack.push( IfStatus::CURRENTLY_TRUE_PREVIOUSLY_FALSE );
                        m_inactiveIfs -= 1;
                        outputShader += line + '\n';
                    }
                    break;
                }
                case CASE_ELIF:
                {
                    if ( m_inactiveIfs > 1 )
                    {
                        break;
                    }

                    if ( m_ifStatementStack.top() == IfStatus::CURRENTLY_FALSE_PREVIOUSLY_FALSE )
                    {
                        bool expressionResult;
                        if ( !EvaulateExpression( line, endOfTokenIndex, expressionResult ) )
                        {
                            return false;
                        }
                        
                        if ( expressionResult )
                        {
                            m_inactiveIfs -= 1;
                            m_ifStatementStack.pop();
                            m_ifStatementStack.push( IfStatus::CURRENTLY_TRUE_PREVIOUSLY_FALSE );
                        }
                        outputShader += line + '\n';
                    }

                    break;
                }
                case CASE_ENDIF:
                {
                    m_inactiveIfs -= 1;
                    if ( m_inactiveIfs == 0 )
                    {
                        m_ifStatementStack.pop();
                        outputShader += line + '\n';
                    }
                    break;
                }
                case CASE_IF:
                case CASE_IFDEF:
                case CASE_IFNDEF:
                {
                    m_inactiveIfs++;
                    break;
                }
                default:
                    // Skip all other preproc directives that don't change the active/inactive status
                    break;
            }

            return true;
        }

        switch ( label )
        {
            case CASE_DEFINE:
            {
                outputShader += line + '\n';
                std::string key = GetNextToken( line, endOfTokenIndex );
                if ( key == "" )
                {
                    LOG_ERR( "Invalud #define command '%s'\n", line.c_str() );
                    return false;
                }
                std::string value = GetNextToken( line, endOfTokenIndex );

                AddDefine( key, value );
                break;
            }
            case CASE_ELSE:
            {
                outputShader += line + '\n';
                if ( m_ifStatementStack.empty() )
                {
                    LOG_ERR( "#else statement without matching #if/#ifdef/#ifndef. Line '%s'\n", line.c_str() );
                    return false;
                }

                m_inactiveIfs += 1;
                m_ifStatementStack.push( IfStatus::CURRENTLY_FALSE_PREVIOUSLY_TRUE );
                break;
            }
            case CASE_ELIF:
            {
                outputShader += line + '\n';
                if ( m_ifStatementStack.empty() )
                {
                    LOG_ERR( "#elif statement without matching #if/#ifdef/#ifndef. Line '%s'\n", line.c_str() );
                    return false;
                }

                m_inactiveIfs += 1;
                m_ifStatementStack.pop();
                m_ifStatementStack.push( IfStatus::CURRENTLY_FALSE_PREVIOUSLY_TRUE );
                break;
            }
            case CASE_ENDIF:
            {
                outputShader += line + '\n';
                m_ifStatementStack.pop();
                break;
            }
            case CASE_ERROR:
            {
                outputShader += line + '\n';
                LOG_ERR( "'%s'\n", line.c_str() );
                return false;
            }
            case CASE_EXTENSION:
            {
                outputShader += line + '\n';
                break;
            }
            case CASE_IF:
            {
                outputShader += line + '\n';
                bool expressionResult;
                if ( !EvaulateExpression( line, endOfTokenIndex, expressionResult ) )
                {
                    return false;
                }
                        
                if ( expressionResult )
                {
                    m_ifStatementStack.push( IfStatus::CURRENTLY_TRUE_PREVIOUSLY_FALSE );
                    outputShader += line + '\n';
                }
                else
                {
                    m_inactiveIfs += 1;
                    m_ifStatementStack.push( IfStatus::CURRENTLY_FALSE_PREVIOUSLY_FALSE );
                }
                
                break;
            }
            case CASE_IFDEF:
            {
                outputShader += line + '\n';
                std::string key = GetNextToken( line, endOfTokenIndex );
                if ( key == "" )
                {
                    LOG_ERR( "Invalud #ifndef command '%s'\n", line.c_str() );
                    return false;
                }
                std::string defineValue;
                if ( GetDefine( key, defineValue ) )
                {
                    m_ifStatementStack.push( IfStatus::CURRENTLY_TRUE_PREVIOUSLY_FALSE );
                }
                else
                {
                    m_ifStatementStack.push( IfStatus::CURRENTLY_FALSE_PREVIOUSLY_FALSE );
                    m_inactiveIfs += 1;
                }
                break;
            }
            case CASE_IFNDEF:
            {
                std::string key = GetNextToken( line, endOfTokenIndex );
                if ( key == "" )
                {
                    LOG_ERR( "Invalud #ifndef command '%s'\n", line.c_str() );
                    return false;
                }
                std::string defineValue;
                if ( !GetDefine( key, defineValue ) )
                {
                    m_ifStatementStack.push( IfStatus::CURRENTLY_TRUE_PREVIOUSLY_FALSE );
                }
                else
                {
                    m_ifStatementStack.push( IfStatus::CURRENTLY_FALSE_PREVIOUSLY_FALSE );
                    m_inactiveIfs += 1;
                }
                break;
            }
            case CASE_INCLUDE:
            {
                std::string absoluteIncludePath;
                if ( !GetIncludePath( line, absoluteIncludePath, endOfTokenIndex ) )
                {
                    return false;
                }
                includedFiles.push_back( absoluteIncludePath );

                if constexpr( !includesOnly )
                {
                    outputShader += "#line 1 \"" + absoluteIncludePath + "\"\n";
                }
                if ( !PreprocessInternal< includesOnly >( absoluteIncludePath ) )
                {
                    return false;
                }
                m_currentFilename = path;
                m_currentLineNumber = lineNumber;
                if constexpr( !includesOnly )
                {
                    outputShader += "#line " + std::to_string( lineNumber + 2 ) + " \"" + path + "\"\n";
                }
                
                return true;
            }
            case CASE_UNDEF:
            {
                outputShader += line + '\n';
                std::string key = GetNextToken( line, endOfTokenIndex );
                if ( key == "" )
                {
                    LOG_ERR( "Invalid #undef command '%s'\n", line.c_str() );
                    return false;
                }
                if ( !RemoveDefine( key ) )
                {
                    return false;
                }
                break;
            }
            case CASE_VERSION:
            {
                outputShader += line + '\n';
                break;
            }
            case CASE_WARNING:
            {
                outputShader += line + '\n';
                LOG_WARN( "'%s'\n", line.c_str() );
                break;
            }
            default:
                outputShader += line + '\n';
                LOG_WARN( "Unknown preproc command '%s' on line '%s' in shader '%s'\n", command.c_str(), line.c_str(), path.c_str() );
        }

        return true;
    }


    template< bool includesOnly >
    bool PreprocessInternal( const std::string& path )
    {
        std::ifstream inFile( path );
        if ( !inFile )
        {
            LOG_ERR( "Could not open shader file '%s'\n", path.c_str() );
            return false;
        }
        m_currentFilename = path;
        m_currentLineNumber = 0;

        uint32_t lineNumber = 0;
        std::string line;
        int preprocIndex;
        while ( std::getline( inFile, line ) )
        {
            if ( IsPreprocLine( line, preprocIndex ) )
            {
                int endOfTokenIndex = preprocIndex;
                std::string command = GetNextToken( line, endOfTokenIndex );
                if ( command == "" )
                {
                    LOG_ERR( "Invalid preproc command '%s'\n", path.c_str(), line.c_str() );
                    return false;
                }

                if ( !ProcessActivePreprocStatment< includesOnly >( line, lineNumber, command, path, endOfTokenIndex ) )
                {
                    return false;
                }
            }
            else
            {
                if constexpr ( !includesOnly )
                {
                    if ( Active() )
                    {
                        outputShader += line + '\n';
                    }
                }
            }

            ++m_currentLineNumber;
            ++lineNumber;
        }

        return true;
    }

};


ShaderPreprocessOutput PreprocessShader( const std::string& filename, const DefineList& defines )
{
    ShaderPreprocessOutput output;
    ShaderPreprocessor preprocessor;
    if ( !preprocessor.Preprocess< false >( filename, defines ) )
    {
        LOG_ERR( "Preprocessing for shader '%s' failed\n", filename.c_str() );
        output.success = false;
    }
    else
    {
        output.success = true;
        output.outputShader = std::move( preprocessor.outputShader );
        output.includedFiles = std::move( preprocessor.includedFiles );
    }

    return output;
}


ShaderPreprocessOutput PreprocessShaderForIncludeListOnly( const std::string& filename, const DefineList& defines )
{
    ShaderPreprocessOutput output;
    ShaderPreprocessor preprocessor;
    if ( !preprocessor.Preprocess< true >( filename, defines ) )
    {
        LOG_ERR( "Preprocessing the includes for shader '%s' failed\n", filename.c_str() );
        output.success = false;
    }
    else
    {
        output.success = true;
        output.includedFiles = std::move( preprocessor.includedFiles );
    }

    return output;
}

} // namespace PG
