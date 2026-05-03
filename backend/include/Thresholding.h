#pragma once
#include <vector>
#include <cstdint>

struct GrayImage {
    int width;
    int height;
    std::vector<uint8_t> data;
};

class Thresholding {
public:
    static GrayImage applyOptimalThresholding(const GrayImage& input);
    static GrayImage applyOtsuThresholding(const GrayImage& input);
    static GrayImage applyLocalThresholding(const GrayImage& input, int windowSize, int C);
    // Added for Spectral Thresholding (more than 2 modes)
    static GrayImage applySpectralThresholding(const GrayImage& input, int modes);

private:
    static std::vector<int> calculateHistogram(const GrayImage& img);
    static GrayImage applyThresholdValue(const GrayImage& input, uint8_t threshold);
};