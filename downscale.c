#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>
#ifdef _WIN32
#include <winsock.h>
#endif
#include <stdbool.h>
#include <float.h>

#include <stdbool.h>
#include <MagickWand/MagickWand.h>



void downscale_png_to_bmp(const char* input, const char* output) {
    MagickWand *m_wand = NULL;

    MagickWandGenesis();
    
    m_wand = NewMagickWand();

    MagickReadImage(m_wand, input);

    MagickResizeImage(m_wand, 320, 200, LanczosFilter);
    MagickSetImageFormat(m_wand, "BMP3");
    MagickSetImageDepth(m_wand, 8);
    MagickQuantizeImage(m_wand, 208, RGBColorspace, 0, NoDitherMethod, MagickFalse);
    
	MagickGammaImage(m_wand, 2.2);
	
    MagickSetCompression(m_wand, NoCompression);
    MagickStripImage(m_wand);

    MagickWriteImage(m_wand, output);

    if (m_wand) m_wand = DestroyMagickWand(m_wand);
    
    MagickWandTerminus();
}

#ifdef STANDALONE
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input PNG file> <output BMP file>\n", argv[0]);
        return 1;
    }

    downscale_png_to_bmp(argv[1], argv[2]);

    return 0;
}
#endif
