#include "renderer/debug_ui.hpp"
#include "renderer/debug_ui_console.hpp"

#if !USING( PG_DEBUG_UI )

namespace PG::Gfx::UIOverlay
{

bool Init( PixelFormat colorAttachmentFormat )
{
    PG_UNUSED( colorAttachmentFormat );
    return true;
}
void Shutdown() {}

void BeginFrame() {}
void Render( CommandBuffer& cmdBuf ) { PG_UNUSED( cmdBuf ); }
void EndFrame() {}

void AddDrawFunction( const std::function<void()>& func ) { PG_UNUSED( func ); }
bool CapturingMouse() { return false; }
void ToggleConsoleVisibility() {}

bool Header( const char* caption )
{
    PG_UNUSED( caption );
    return false;
}
bool CheckBox( const char* caption, bool* value )
{
    PG_UNUSED( caption );
    PG_UNUSED( value );
    return false;
}
bool CheckBox( const char* caption, int* value )
{
    PG_UNUSED( caption );
    PG_UNUSED( value );
    return false;
}
bool InputFloat( const char* caption, float* value, float step )
{
    PG_UNUSED( caption );
    PG_UNUSED( value );
    PG_UNUSED( step );
    return false;
}
bool SliderFloat( const char* caption, float* value, float min, float max )
{
    PG_UNUSED( caption );
    PG_UNUSED( value );
    PG_UNUSED( min );
    PG_UNUSED( max );
    return false;
}
bool SliderInt( const char* caption, int* value, int min, int max )
{
    PG_UNUSED( caption );
    PG_UNUSED( value );
    PG_UNUSED( min );
    PG_UNUSED( max );
    return false;
}
bool ComboBox( const char* caption, int* itemindex, const std::vector<std::string>& items )
{
    PG_UNUSED( caption );
    PG_UNUSED( itemindex );
    PG_UNUSED( items );
    return false;
}
bool Button( const char* caption )
{
    PG_UNUSED( caption );
    return false;
}
void Text( const char* formatstr, ... ) { PG_UNUSED( formatstr ); }
bool Updated() { return false; }

} // namespace PG::Gfx::UIOverlay

#else // #if !USING( PG_DEBUG_UI )

#include "core/dvars.hpp"
#include "core/input.hpp"
#include "core/time.hpp"
#include "core/window.hpp"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "renderer/graphics_api/pg_to_vulkan_types.hpp"
#include "renderer/r_globals.hpp"
#include "renderer/vulkan.hpp"
#include "shared/logger.hpp"

static PG::Dvar dvarDebugUI( "r_debugUI", true, "Controls whether to allow any 2D debug UI elements (those that use ImGUI) to be drawn" );

namespace PG::Gfx::UIOverlay
{

static bool s_updated;
static std::vector<std::function<void()>> s_drawFunctions;
static VkDescriptorPool s_descriptorPool;
static Console* s_console;

static void CheckVkResult( VkResult err )
{
    if ( err == VK_SUCCESS )
        return;

    LOG_ERR( "ImGui: VkResult = %d\n", err );
    if ( err < 0 )
        abort();
}

bool Init( PixelFormat colorAttachmentFormat )
{
    s_updated = false;
    s_drawFunctions.reserve( 64 );

    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets                    = 1000;
    poolInfo.poolSizeCount              = ARRAY_COUNT( poolSizes );
    poolInfo.pPoolSizes                 = poolSizes;

    VK_CHECK( vkCreateDescriptorPool( rg.device, &poolInfo, nullptr, &s_descriptorPool ) );
    PG_DEBUG_MARKER_SET_DESC_POOL_NAME( s_descriptorPool, "ImGui pool" );

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan( GetMainWindow()->GetGLFWHandle(), true );
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = rg.instance;
    init_info.PhysicalDevice            = rg.physicalDevice;
    init_info.Device                    = rg.device;
    init_info.QueueFamily               = rg.device.GetQueue().familyIndex;
    init_info.Queue                     = rg.device.GetQueue();
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = s_descriptorPool;
    init_info.Subpass                   = 0;
    init_info.MinImageCount             = rg.swapchain.GetNumImages(); // ?
    init_info.ImageCount                = rg.swapchain.GetNumImages(); // ?
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = CheckVkResult;
    init_info.UseDynamicRendering       = true;
    init_info.ColorAttachmentFormat     = PGToVulkanPixelFormat( colorAttachmentFormat );
    ImGui_ImplVulkan_Init( &init_info, VK_NULL_HANDLE );

    PG_DEBUG_MARKER_SET_SHADER_NAME( ImGui_ImplVulkan_GetVertShaderModule(), "ImGui Vert" );
    PG_DEBUG_MARKER_SET_SHADER_NAME( ImGui_ImplVulkan_GetFragShaderModule(), "ImGui Frag" );
    PG_DEBUG_MARKER_SET_DESC_SET_LAYOUT_NAME( ImGui_ImplVulkan_GetDescSetLayout(), "ImGui" );
    PG_DEBUG_MARKER_SET_PIPELINE_LAYOUT_NAME( ImGui_ImplVulkan_GetPipelineLayout(), "ImGui" );
    PG_DEBUG_MARKER_SET_PIPELINE_NAME( ImGui_ImplVulkan_GetPipeline(), "ImGui" );

    ImGui_ImplVulkan_CreateFontsTexture();

    PG_DEBUG_MARKER_SET_COMMAND_POOL_NAME( ImGui_ImplVulkan_GetFontCommandPool(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_COMMAND_BUFFER_NAME( ImGui_ImplVulkan_GetFontCommandBuffer(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_SAMPLER_NAME( ImGui_ImplVulkan_GetFontSampler(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_IMAGE_NAME( ImGui_ImplVulkan_GetFontImage(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_IMAGE_VIEW_NAME( ImGui_ImplVulkan_GetFontImageView(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_MEMORY_NAME( ImGui_ImplVulkan_GetFontMemory(), "ImGui Font" );
    PG_DEBUG_MARKER_SET_DESC_SET_NAME( ImGui_ImplVulkan_GetFontDescSet(), "ImGui Font" );

    // there are also 2 buffers and 2 buffer memories that ImGui uses (the vertex and index buffers), but they are re-created as needed
    // during ImGui_ImplVulkan_RenderDrawData, so we aren't bothering to update their names each time

    s_console = new Console;

    return true;
}

void Shutdown()
{
    delete s_console;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool( rg.device, s_descriptorPool, nullptr );
    s_drawFunctions.clear();
}

void AddDrawFunction( const std::function<void()>& func ) { s_drawFunctions.push_back( func ); }

void BeginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Render( CommandBuffer& cmdBuf )
{
    if ( !dvarDebugUI.GetBool() )
        return;

    for ( const auto& drawFunc : s_drawFunctions )
    {
        drawFunc();
    }
    s_drawFunctions.clear();

    // if ( ImGui::IsWindowFocused( ImGuiFocusedFlags_AnyWindow ) )
    //{
    //     LOG( "IMGUI WINDOW FOCUSED" );
    // }
    // else
    //{
    //     LOG_ERR( "no focus" );
    // }

    s_console->Draw();

    // ImGui::ShowDemoWindow();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData( draw_data, cmdBuf );
}

void EndFrame() { ImGui::EndFrame(); }

bool CapturingMouse() { return dvarDebugUI.GetBool() && ImGui::GetIO().WantCaptureMouse; }

void ToggleConsoleVisibility() { s_console->ToggleVisibility(); }

bool Header( const char* caption ) { return ImGui::CollapsingHeader( caption, ImGuiTreeNodeFlags_DefaultOpen ); }

bool CheckBox( const char* caption, bool* value )
{
    bool res = ImGui::Checkbox( caption, value );
    if ( res )
        s_updated = true;
    return res;
}

bool CheckBox( const char* caption, int* value )
{
    bool val = ( *value == 1 );
    bool res = ImGui::Checkbox( caption, &val );
    *value   = val;
    if ( res )
        s_updated = true;
    return res;
}

bool InputFloat( const char* caption, float* value, float step )
{
    bool res = ImGui::InputFloat( caption, value, step, step * 10.0f );
    if ( res )
        s_updated = true;
    return res;
}

bool SliderFloat( const char* caption, float* value, float min, float max )
{
    bool res = ImGui::SliderFloat( caption, value, min, max );
    if ( res )
        s_updated = true;
    return res;
}

bool SliderInt( const char* caption, int* value, int min, int max )
{
    bool res = ImGui::SliderInt( caption, value, min, max );
    if ( res )
        s_updated = true;
    return res;
}

bool ComboBox( const char* caption, int* itemIndex, const std::vector<std::string>& items )
{
    if ( items.empty() )
    {
        return false;
    }
    std::vector<const char*> charItems;
    charItems.reserve( items.size() );
    for ( size_t i = 0; i < items.size(); i++ )
    {
        charItems.push_back( items[i].c_str() );
    }
    uint32_t itemCount = static_cast<uint32_t>( charItems.size() );
    bool res           = ImGui::Combo( caption, itemIndex, &charItems[0], itemCount, itemCount );
    if ( res )
        s_updated = true;
    return res;
}

bool Button( const char* caption )
{
    bool res = ImGui::Button( caption );
    if ( res )
        s_updated = true;
    return res;
}

void Text( const char* formatstr, ... )
{
    va_list args;
    va_start( args, formatstr );
    ImGui::TextV( formatstr, args );
    va_end( args );
}

bool Updated() { return s_updated; }

} // namespace PG::Gfx::UIOverlay

#endif // #else // #if !USING( PG_DEBUG_UI )
