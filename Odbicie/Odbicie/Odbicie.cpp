#include "pch.h"
#include "Odbicie.h"


// Funkcja do odbicia lustrzanego bitmapy pionowo
void flipVertical(uint8_t* bitmap, uint32_t width,  uint32_t startRow, uint32_t endRow) {
    uint32_t rowSize = width * 3; // 3 bytes per pixel (RGB)
    uint8_t temp[3]; // Temporary array for pixel swapping

    for (uint32_t y = startRow; y < endRow; ++y) {
        for (uint32_t x = 0; x < width / 2; ++x) {
            uint32_t leftIndex = (y * rowSize) + (x * 3);
            uint32_t rightIndex = (y * rowSize) + ((width - 1 - x) * 3);

            // Swap left and right pixels
            temp[0] = bitmap[leftIndex];
            temp[1] = bitmap[leftIndex + 1];
            temp[2] = bitmap[leftIndex + 2];

            bitmap[leftIndex] = bitmap[rightIndex];
            bitmap[leftIndex + 1] = bitmap[rightIndex + 1];
            bitmap[leftIndex + 2] = bitmap[rightIndex + 2];

            bitmap[rightIndex] = temp[0];
            bitmap[rightIndex + 1] = temp[1];
            bitmap[rightIndex + 2] = temp[2];
        }
    }
}

