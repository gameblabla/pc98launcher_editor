#include <stdio.h>
#include <stdlib.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


// Helper function to convert to grayscale and downscale
unsigned char* convert_to_grayscale_and_downscale(unsigned char* data, int width, int height, int channels, int new_width, int new_height) {
    unsigned char* new_data = (unsigned char*)malloc(new_width * new_height);
    if (!new_data) {
        perror("Memory allocation failed");
        exit(1);
    }

    for (int y = 0; y < new_height; ++y) {
        for (int x = 0; x < new_width; ++x) {
            // Averaging 2x2 blocks for downscaling
            unsigned long gray = 0;
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    int px = (2 * x + dx) * channels;
                    int py = (2 * y + dy) * width * channels;
                    gray += 0.3 * data[py + px] + 0.59 * data[py + px + 1] + 0.11 * data[py + px + 2];
                }
            }
            new_data[y * new_width + x] = (unsigned char)(gray / 4);
        }
    }

    return new_data;
}

void write_bmp_file_header(FILE *file, int filesize) {
    unsigned char file_header[14] = {
        'B', 'M',         // Signature
        0, 0, 0, 0,       // Image file size in bytes
        0, 0, 0, 0,       // Reserved
        54, 4, 0, 0       // Start position of pixel data
    };

    // Write file size
    file_header[2] = (unsigned char)(filesize);
    file_header[3] = (unsigned char)(filesize >> 8);
    file_header[4] = (unsigned char)(filesize >> 16);
    file_header[5] = (unsigned char)(filesize >> 24);

    fwrite(file_header, 1, 14, file);
}

// Function to write the BMP info header
void write_bmp_info_header(FILE *file, int width, int height) {
    unsigned char info_header[40] = {
        40, 0, 0, 0,    // Size of this header
        0, 0, 0, 0,     // Bitmap width
        0, 0, 0, 0,     // Bitmap height
        1, 0,           // Number of color planes
        8, 0,           // Bits per pixel
        0, 0, 0, 0,     // Compression
        0, 0, 0, 0,     // Image size (no compression)
        0, 0, 0, 0,     // Horizontal resolution
        0, 0, 0, 0,     // Vertical resolution
        0, 0, 0, 0,     // Number of colors in the palette
        0, 0, 0, 0      // Important colors
    };

    // Width
    info_header[4] = (unsigned char)(width);
    info_header[5] = (unsigned char)(width >> 8);
    info_header[6] = (unsigned char)(width >> 16);
    info_header[7] = (unsigned char)(width >> 24);

    // Height
    info_header[8] = (unsigned char)(height);
    info_header[9] = (unsigned char)(height >> 8);
    info_header[10] = (unsigned char)(height >> 16);
    info_header[11] = (unsigned char)(height >> 24);

    fwrite(info_header, 1, 40, file);
}

// Function to write the BMP color palette
void write_bmp_color_palette(FILE *file) {
    for (int i = 0; i < 256; i++) {
        unsigned char palette[4] = {i, i, i, 0};
        fwrite(palette, 1, 4, file);
    }
}


int main(int argc, char** argv) {
    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);

    if(img == NULL) {
        printf("Error in loading the image\n");
        exit(1);
    }

    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

    // Convert and downscale the image
    int new_width = 320;
    int new_height = 200;
    unsigned char *new_img = convert_to_grayscale_and_downscale(img, width, height, channels, new_width, new_height);

    // Calculate the BMP file size
    int image_size = new_width * new_height;
    int filesize = 54 + 1024 + image_size; // Header (54 bytes) + color palette (1024 bytes) + image data

    // Open file
    FILE *file = fopen("output.bmp", "wb");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    // Write the headers and color palette
    write_bmp_file_header(file, filesize);
    write_bmp_info_header(file, new_width, new_height);
    write_bmp_color_palette(file);

    // Write the image data in bottom-up order
    for (int y = new_height - 1; y >= 0; y--) {
        fwrite(new_img + y * new_width, 1, new_width, file);
    }


    fclose(file);

    stbi_image_free(img);
    free(new_img);

    return 0;
}
