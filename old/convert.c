#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <limits.h>  // For ULONG_MAX
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    unsigned char r, g, b;
} Color;

typedef struct {
    Color centroid;
    long total_r, total_g, total_b;
    int count;
} Cluster;



#define min(a, b) ((a) < (b) ? (a) : (b))

unsigned char palette[256][4];


// Calculate Euclidean distance between two colors
unsigned long color_distance(unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2) {
    long r_diff = (long)r1 - (long)r2;
    long g_diff = (long)g1 - (long)g2;
    long b_diff = (long)b1 - (long)b2;
    return r_diff * r_diff + g_diff * g_diff + b_diff * b_diff;
}

// Find the closest color in the palette for a given RGB color
unsigned char find_closest_palette_color(unsigned char r, unsigned char g, unsigned char b, unsigned char palette[][4], int palette_size) {
    unsigned long min_distance = ULONG_MAX;
    unsigned char closest_color = 0;

    for (int i = 0; i < palette_size; ++i) {
        unsigned long distance = color_distance(r, g, b, palette[i][0], palette[i][1], palette[i][2]);
        if (distance < min_distance) {
            min_distance = distance;
            closest_color = i;
        }
    }

    return closest_color;
}



// Helper function to quantize colors and downscale
unsigned char* convert_and_downscale(unsigned char* data, int width, int height, int channels, int new_width, int new_height, unsigned char palette[][4], int palette_size) {
    unsigned char* new_data = (unsigned char*)malloc(new_width * new_height);
    if (!new_data) {
        perror("Memory allocation failed");
        exit(1);
    }

    for (int y = 0; y < new_height; ++y) {
        for (int x = 0; x < new_width; ++x) {
            unsigned long r_total = 0, g_total = 0, b_total = 0;
            for (int dy = 0; dy < 2; ++dy) {
                for (int dx = 0; dx < 2; ++dx) {
                    int srcX = min(2 * x + dx, width - 1);
                    int srcY = min(2 * y + dy, height - 1);
                    int index = (srcY * width + srcX) * channels;

                    r_total += data[index];
                    g_total += data[index + 1];
                    b_total += data[index + 2];
                }
            }

            unsigned char avg_r = (unsigned char)(r_total / 4);
            unsigned char avg_g = (unsigned char)(g_total / 4);
            unsigned char avg_b = (unsigned char)(b_total / 4);

            // Find the closest color in the palette
            unsigned char color_index = find_closest_palette_color(avg_b, avg_g, avg_r, palette, palette_size);
            new_data[y * new_width + x] = color_index;
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


// Function to write the entire 256-color palette
void write_256_color_palette(FILE *file) {
    // Write the entire palette array to the file
    fwrite(palette, 1, sizeof(palette), file);
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

void sample_image_colors(unsigned char *data, int width, int height, int channels, int sample_size) {
    int pixel_count = width * height;
    int step = pixel_count / sample_size;

    for (int i = 0; i < sample_size; i++) {
        int pixel_index = (i * step) % pixel_count;
        int data_index = pixel_index * channels;

        palette[i][0] = data[data_index];       // Red
        palette[i][1] = data[data_index + 1];   // Green
        palette[i][2] = data[data_index + 2];   // Blue
        palette[i][3] = 0;                      // Alpha (not used)
    }
}

void generate_palette(unsigned char *data, int width, int height, int channels) {
    int sampled_colors = 240; // Number of colors to sample from the image
    sample_image_colors(data, width, height, channels, sampled_colors);

    // Fill the remaining palette with shades of gray or other colors
    for (int i = sampled_colors; i < 256; i++) {
        unsigned char gray = (i - sampled_colors) * 4;
        palette[i][0] = gray;
        palette[i][1] = gray;
        palette[i][2] = gray;
        palette[i][3] = 0;
    }
}

// Function to calculate the squared Euclidean distance between two colors
double color_distance_squared(Color c1, Color c2) {
    long r_diff = (long)c1.r - (long)c2.r;
    long g_diff = (long)c1.g - (long)c2.g;
    long b_diff = (long)c1.b - (long)c2.b;
    return r_diff * r_diff + g_diff * g_diff + b_diff * b_diff;
}

void kmeans(unsigned char* data, int width, int height, int channels, Cluster* clusters, int k) {
    // Step 1: Initialize clusters with random colors from the image
    for (int i = 0; i < k; i++) {
        int random_pixel = (rand() % (width * height)) * channels;
        clusters[i].centroid.r = data[random_pixel];
        clusters[i].centroid.g = data[random_pixel + 1];
        clusters[i].centroid.b = data[random_pixel + 2];
    }

    bool changed;
    do {
        // Reset cluster data
        for (int i = 0; i < k; i++) {
            clusters[i].total_r = 0;
            clusters[i].total_g = 0;
            clusters[i].total_b = 0;
            clusters[i].count = 0;
        }

        changed = false;

        // Step 2: Assign each pixel to the nearest cluster
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixel_index = (y * width + x) * channels;
                Color pixel_color = {data[pixel_index], data[pixel_index + 1], data[pixel_index + 2]};
                
                // Find nearest cluster
                int nearest_cluster = 0;
                double min_distance = color_distance_squared(pixel_color, clusters[0].centroid);
                for (int j = 1; j < k; j++) {
                    double distance = color_distance_squared(pixel_color, clusters[j].centroid);
                    if (distance < min_distance) {
                        min_distance = distance;
                        nearest_cluster = j;
                    }
                }

                // Update cluster totals
                clusters[nearest_cluster].total_r += pixel_color.r;
                clusters[nearest_cluster].total_g += pixel_color.g;
                clusters[nearest_cluster].total_b += pixel_color.b;
                clusters[nearest_cluster].count++;
            }
        }

        // Step 3: Update each cluster's centroid
        for (int i = 0; i < k; i++) {
            if (clusters[i].count > 0) {
                Color new_centroid = {
                    (unsigned char)(clusters[i].total_r / clusters[i].count),
                    (unsigned char)(clusters[i].total_g / clusters[i].count),
                    (unsigned char)(clusters[i].total_b / clusters[i].count)
                };

                // Check if the centroid has changed
                if (color_distance_squared(clusters[i].centroid, new_centroid) > 1) {
                    changed = true;
                    clusters[i].centroid = new_centroid;
                }
            }
        }
    } while (changed);
}


void generate_palette_with_kmeans(unsigned char *data, int width, int height, int channels) {
    int k = 256; // Number of clusters
    Cluster clusters[k];

    kmeans(data, width, height, channels, clusters, k);

    // Fill the palette with the centroids of the clusters
    for (int i = 0; i < k; i++) {
        palette[i][0] = clusters[i].centroid.r;
        palette[i][1] = clusters[i].centroid.g;
        palette[i][2] = clusters[i].centroid.b;
        palette[i][3] = 0;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <image file>\n", argv[0]);
        return 1;
    }

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);

    if (img == NULL) {
        fprintf(stderr, "Error in loading the image\n");
        return 1;
    }

    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);

	generate_palette_with_kmeans(img, width, height, channels);

    // Convert and downscale the image
    int new_width = 320;
    int new_height = 200;
    unsigned char *new_img = convert_and_downscale(img, width, height, channels, new_width, new_height, palette, 256);

    // Calculate the BMP file size
    int image_size = new_width * new_height;
    int filesize = 54 + 1024 + image_size; // Header (54 bytes) + color palette (1024 bytes) + image data

    // Open file for writing
    FILE *file = fopen("output.bmp", "wb");
    if (!file) {
        fprintf(stderr, "Error opening file for writing\n");
        stbi_image_free(img);
        free(new_img);
        return 1;
    }

    // Write the headers and color palette
    write_bmp_file_header(file, filesize);
    write_bmp_info_header(file, new_width, new_height);
    write_256_color_palette(file);  // Writes the same palette to the BMP file

    // Write the image data in bottom-up order
    for (int y = new_height - 1; y >= 0; y--) {
        fwrite(new_img + y * new_width, 1, new_width, file);
    }

    fclose(file);

    stbi_image_free(img);
    free(new_img);

    printf("Conversion complete. Output saved as 'output.bmp'\n");

    return 0;
}


