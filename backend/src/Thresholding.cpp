#include "../include/Thresholding.h"
#include <numeric>
#include <cmath>

std::vector<int> Thresholding::calculateHistogram(const GrayImage& img) {
    std::vector<int> hist(256, 0);
    for (uint8_t pixel : img.data) {
        hist[pixel]++;
    }
    return hist;
}

GrayImage Thresholding::applyThresholdValue(const GrayImage& input, uint8_t threshold) {
    GrayImage output = {input.width, input.height, std::vector<uint8_t>(input.data.size())};
    for (size_t i = 0; i < input.data.size(); ++i) {
        output.data[i] = (input.data[i] >= threshold) ? 255 : 0;
    }
    return output;
}

GrayImage Thresholding::applyOptimalThresholding(const GrayImage& input) {
    uint8_t currentThreshold = 127;
    uint8_t previousThreshold = 0;

    while (currentThreshold != previousThreshold) {
        previousThreshold = currentThreshold;
        long sumBackground = 0, countBackground = 0;
        long sumForeground = 0, countForeground = 0;

        for (uint8_t pixel : input.data) {
            if (pixel < currentThreshold) {
                sumBackground += pixel;
                countBackground++;
            } else {
                sumForeground += pixel;
                countForeground++;
            }
        }

        uint8_t meanBackground = (countBackground == 0) ? 0 : sumBackground / countBackground;
        uint8_t meanForeground = (countForeground == 0) ? 0 : sumForeground / countForeground;

        currentThreshold = (meanBackground + meanForeground) / 2;
    }

    return applyThresholdValue(input, currentThreshold);
}

GrayImage Thresholding::applyOtsuThresholding(const GrayImage& input) {
    std::vector<int> hist = calculateHistogram(input);
    int totalPixels = input.width * input.height;

    float sum = 0;
    for (int i = 0; i < 256; ++i) sum += i * hist[i];

    float sumB = 0;
    int wB = 0;
    int wF = 0;

    float varMax = 0;
    uint8_t threshold = 0;

    for (int i = 0; i < 256; ++i) {
        wB += hist[i];
        if (wB == 0) continue;

        wF = totalPixels - wB;
        if (wF == 0) break;

        sumB += (float)(i * hist[i]);

        float mB = sumB / wB;
        float mF = (sum - sumB) / wF;

        float varBetween = (float)wB * (float)wF * (mB - mF) * (mB - mF);

        if (varBetween > varMax) {
            varMax = varBetween;
            threshold = i;
        }
    }

    return applyThresholdValue(input, threshold);
}
