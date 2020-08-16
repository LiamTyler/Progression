#include "image.hpp"
#include "gfx_image.hpp"
#include "utils/logger.hpp"

using namespace Progression;

int main( int argc, char** argv )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    
    // Image image;
    // ImageCreateInfo info;
    // info.filename = PG_ROOT_DIR "macaw.exr";
    // image.Load( &info );
    // image.Save( PG_ROOT_DIR "macaw3.exr" );


    // GfxImageCreateInfo createInfo;
    // createInfo.imageType = GfxImageType::TYPE_2D;
    // createInfo.dstPixelFormat = PixelFormat::R8_G8_B8_A8_SRGB;
    // createInfo.filename = PG_ROOT_DIR "macaw.png";
    // GfxImage image;
    // if ( !Load_GfxImage( &image, createInfo ) )
    // {
    //     LOG_ERR( "Failed to load gfx image\n" );
    // }
    
    float f = powf( 2, -149 );
    float x = f;
    int i;
    x = std::frexpf( x, &i );
    LOG( "%f = %f * 2**%d\n", f, x, i );



    //size_t x = 0x12345678;
    //for ( int byte = 0; byte < sizeof( size_t ); ++byte )
    //{
    //    unsigned char y = (reinterpret_cast<unsigned char*>( &x )[byte]);
    //    printf( "x[0] = %x\n", y );
    //}

    Logger_Shutdown();

    return 0;
}