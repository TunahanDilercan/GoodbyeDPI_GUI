#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple BMP file header structures
#pragma pack(push, 1)
typedef struct {
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
} BITMAPFILEHEADER;

typedef struct {
    DWORD   biSize;
    LONG    biWidth;
    LONG    biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    LONG    biXPelsPerMeter;
    LONG    biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

// Function to create a simple gradient bitmap
void create_gradient_bitmap(const char *filename, int width, int height, COLORREF startColor, COLORREF endColor, const char *text) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        printf("Could not create file: %s\n", filename);
        return;
    }

    // Calculate bitmap size
    int paddedWidth = ((width * 3 + 3) / 4) * 4;  // BMP rows are padded to 4-byte boundary
    int imageSize = paddedWidth * height;

    // Setup file header
    BITMAPFILEHEADER fileHeader = {0};
    fileHeader.bfType = 0x4D42; // "BM" signature
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + imageSize;

    // Setup info header
    BITMAPINFOHEADER infoHeader = {0};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height; // Negative for top-down
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24; // 24 bits per pixel (RGB)
    infoHeader.biCompression = 0; // No compression
    infoHeader.biSizeImage = imageSize;
    infoHeader.biXPelsPerMeter = 2835; // 72 DPI
    infoHeader.biYPelsPerMeter = 2835; // 72 DPI

    // Write headers
    fwrite(&fileHeader, sizeof(fileHeader), 1, fp);
    fwrite(&infoHeader, sizeof(infoHeader), 1, fp);

    // Extract color components
    unsigned char startR = GetRValue(startColor);
    unsigned char startG = GetGValue(startColor);
    unsigned char startB = GetBValue(startColor);
    
    unsigned char endR = GetRValue(endColor);
    unsigned char endG = GetGValue(endColor);
    unsigned char endB = GetBValue(endColor);

    // Allocate memory for pixel data
    unsigned char *pixelData = (unsigned char*)malloc(imageSize);
    if (!pixelData) {
        printf("Memory allocation failed\n");
        fclose(fp);
        return;
    }

    // Generate gradient
    for (int y = 0; y < height; y++) {
        float ratio = (float)y / (height - 1);
        
        // Interpolate colors
        unsigned char r = startR + (endR - startR) * ratio;
        unsigned char g = startG + (endG - startG) * ratio;
        unsigned char b = startB + (endB - startB) * ratio;

        for (int x = 0; x < width; x++) {
            int index = y * paddedWidth + x * 3;
            pixelData[index + 0] = b;  // B (BGR order in BMP)
            pixelData[index + 1] = g;  // G
            pixelData[index + 2] = r;  // R
        }

        // Add padding bytes
        for (int p = width * 3; p < paddedWidth; p++) {
            pixelData[y * paddedWidth + p] = 0;
        }
    }

    // Write simple text - just a visual representation
    if (text && strlen(text) > 0) {
        int textLen = strlen(text);
        int textX = (width - textLen * 8) / 2;  // Approximate center
        int textY = height / 2;
        
        for (int i = 0; i < textLen; i++) {
            for (int dy = -10; dy < 10; dy++) {
                for (int dx = -4; dx < 4; dx++) {
                    int x = textX + i * 12 + dx;
                    int y = textY + dy;
                    
                    if (x >= 0 && x < width && y >= 0 && y < height) {
                        int index = y * paddedWidth + x * 3;
                        // Make text white
                        pixelData[index + 0] = 255;  // B
                        pixelData[index + 1] = 255;  // G
                        pixelData[index + 2] = 255;  // R
                    }
                }
            }
        }
    }

    // Write pixel data
    fwrite(pixelData, 1, imageSize, fp);
    
    // Clean up
    free(pixelData);
    fclose(fp);
    
    printf("Created %s (%dx%d)\n", filename, width, height);
}

int main(void) {
    printf("Generating logo files for GoodbyeDPI...\n");
    
    // Create the resources directory if it doesn't exist
    CreateDirectory("resources", NULL);
    
    // Create banner logo (400x100)
    create_gradient_bitmap("resources/logo_banner.bmp", 400, 100, RGB(0, 32, 128), RGB(0, 92, 220), "GoodbyeDPI");
    
    // Create splash screen (500x300)
    create_gradient_bitmap("resources/splash_screen.bmp", 500, 300, RGB(0, 32, 128), RGB(0, 64, 196), "GoodbyeDPI");
    
    // Create small logo (64x64)
    create_gradient_bitmap("resources/logo_small.bmp", 64, 64, RGB(0, 32, 128), RGB(0, 92, 220), "GDP");
    
    printf("All logo files generated successfully.\n");
    return 0;
}