#include "shared/logger.hpp"
#include "core/time.hpp"
//#define MSDFGEN_PUBLIC __declspec(dllimport) // for DLL import/export
#include "msdfgen.h"
#include "ext/import-font.h"
#include "ft2build.h"
#include "ImageLib/bc_compression.hpp"
#include "ImageLib/image.hpp"
#include <fstream>
#include <iostream>


int TestImageCompression()
{
    RawImage2D exrImg;
    //exrImg.Load( PG_ASSET_DIR "images/skyboxes/kloppenheim_06_8k.exr" );
    {
        FloatImage2D floatImg;
        floatImg.Load( PG_ASSET_DIR "images/skyboxes/kloppenheim_06_8k.exr" );
        floatImg = floatImg.Resize( floatImg.width / 2, floatImg.height / 2 );
        exrImg = RawImage2DFromFloatImage( floatImg );
        //exrImg.Save( PG_ASSET_DIR "images/skyboxes/kloppenheim_06_8k_v1.exr" );
    }

    BCCompressorSettings settings;
    settings.format = ImageFormat::BC6H_U16F;
    //auto startTime = Time::GetTimePoint();
    RawImage2D compressed = CompressToBC( exrImg, settings );
    //LOG( "Compression done in %.2f seconds", Time::GetDuration( startTime ) / 1000.0f );
    LOG( "Compression done" );

    RawImage2D uncompressed = compressed.Convert( exrImg.format );
    uncompressed.Save( PG_ASSET_DIR "images/skyboxes/kloppenheim_06_8k_bc6.exr" );

    FloatImage2D original = FloatImageFromRawImage2D( exrImg );
    FloatImage2D uncompressedF = FloatImageFromRawImage2D( uncompressed );

    double mse = FloatImageMSE( original, uncompressedF );
    LOG( "MSE: %f, PSNR: %f", mse, MSEToPSNR( mse ) );

    return 0;
}


int main( int argc, char* argv[] )
{
    Logger_Init();
    Logger_AddLogLocation( "stdout", stdout );
    
    using namespace msdfgen;
    FreetypeHandle *ft = initializeFreetype();
    if (ft) {
        FontHandle *font = loadFont(ft, PG_ASSET_DIR "fonts/arial.ttf" );
        if (font) {
            Shape shape;
            if (loadGlyph(shape, font, 'A')) {
                shape.normalize();
                //                      max. angle
                edgeColoringSimple(shape, 3.0);
                //           image width, height
                Bitmap<float, 4> msdf(32, 32);
                Bitmap<float, 1> sdf(32, 32);
                //                     range, scale, translation
                generateMTSDF(msdf, shape, 4.0, 1.0, Vector2(4.0, 4.0));
                generateSDF(sdf, shape, 4.0, 1.0, Vector2(4.0, 4.0));
                FloatImage2D fImg( msdf.width(), msdf.height(), 3 );
                for ( uint32_t r = 0; r < fImg.height; ++r )
                {
                    for ( uint32_t c = 0; c < fImg.width; ++c )
                    {
                        float* p = msdf( c, 31 - r );
                        float* s = sdf( c, 31 - r );
                        fImg.SetFromFloat4( r, c, glm::vec4( p[0], p[1], p[2], s[0] ) );
                    }
                }
                RawImage2D img = RawImage2DFromFloatImage( fImg, ImageFormat::R8_G8_B8_A8_UNORM );
                img.Save( PG_ROOT_DIR "test.png" );
                //savePng(msdf, "output.png");
            }
            destroyFont(font);
        }
        deinitializeFreetype(ft);
    }
    return 0;
    
    FloatImage2D img;
    img.Load( PG_ASSET_DIR "images/skies/kloppenheim_06_8k.exr" );
    img = img.Resize( img.width / 4, img.height / 4 );
    img.Save( PG_ASSET_DIR "images/skies/kloppenheim_06_2k.hdr" );
    return 0;

    //FloatImageCubemap cubemap;
    //if ( !cubemap.LoadFromEquirectangular( PG_ASSET_DIR "images/skies/kloppenheim_06_2k.exr" ) )
    //    return 0;
    //
    //FloatImage2D equiImg = CubemapToEquirectangular( cubemap );
    //equiImg.Save( PG_ASSET_DIR "images/skies/kloppenheim_06_2k_ConvEqui.exr" );
    //return 0;

    //FloatImageCubemap cubemap;
    //if ( !cubemap.LoadFromEquirectangular( PG_ASSET_DIR "images/skies/kloppenheim_06_8k.exr" ) )
    //{
    //    LOG_ERR( "Failed to load cubemap" );
    //    return false;
    //}
    //
    //for ( int i = 0; i < 6; ++i ) HDRImageToLDR( cubemap.faces[i] );
    //cubemap.SaveUnfoldedFaces( PG_ASSET_DIR "images/skies/kloppenheim_06_8k_unfolded2.png" );
    //return 0;
	
	LOG( "Hello World" );

    return true;
}