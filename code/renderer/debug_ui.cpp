#include "renderer/debug_ui.hpp"

#if USING( PG_DEBUG_UI )
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

namespace PG::Gfx::UIOverlay
{

static Dvar dvarDebugUI( "r_debugUI", false, "Controls whether to allow any 2D debug UI elements (those that use ImGUI) to be drawn" );

static bool s_updated;
static std::vector<std::function<void()>> s_drawFunctions;

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan( GetMainWindow()->GetGLFWHandle(), true );
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = rg.instance;
    init_info.PhysicalDevice            = rg.physicalDevice.GetHandle();
    init_info.Device                    = rg.device.GetHandle();
    init_info.QueueFamily               = rg.physicalDevice.GetGraphicsQueueFamily();
    init_info.Queue                     = rg.device.GetQueue();
    init_info.PipelineCache             = VK_NULL_HANDLE;
    init_info.DescriptorPool            = rg.descriptorPool.GetHandle();
    init_info.Subpass                   = 0;
    init_info.MinImageCount             = 2;                           // ?
    init_info.ImageCount                = rg.swapchain.GetNumImages(); // ?
    init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = CheckVkResult;
    init_info.UseDynamicRendering       = true;
    init_info.ColorAttachmentFormat     = PGToVulkanPixelFormat( colorAttachmentFormat );
    ImGui_ImplVulkan_Init( &init_info, VK_NULL_HANDLE );

    return true;
}

void Shutdown()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    s_drawFunctions.clear();
    s_drawFunctions.shrink_to_fit();
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

    //ImGui::ShowDemoWindow();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData( draw_data, cmdBuf.GetHandle() );
}

void EndFrame() { ImGui::EndFrame(); }

bool CapturingMouse() { return dvarDebugUI.GetBool() && ImGui::GetIO().WantCaptureMouse; }

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

#endif // #if USING( PG_DEBUG_UI )
