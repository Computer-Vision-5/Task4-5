#include "../include/Thresholding.h"
#include <numeric>
#include <cmath>
#include <algorithm>

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

// --- ADDED: Local Thresholding ---
GrayImage Thresholding::applyLocalThresholding(const GrayImage& input, int windowSize, int C) {
    GrayImage output = {input.width, input.height, std::vector<uint8_t>(input.data.size())};
    int halfWindow = windowSize / 2;

    for (int y = 0; y < input.height; ++y) {
        for (int x = 0; x < input.width; ++x) {
            long sum = 0;
            int count = 0;

            // Look at the local neighborhood
            for (int wy = -halfWindow; wy <= halfWindow; ++wy) {
                for (int wx = -halfWindow; wx <= halfWindow; ++wx) {
                    int ny = y + wy;
                    int nx = x + wx;

                    // Check image borders
                    if (ny >= 0 && ny < input.height && nx >= 0 && nx < input.width) {
                        sum += input.data[ny * input.width + nx];
                        count++;
                    }
                }
            }

            int localMean = sum / count;
            int pixelValue = input.data[y * input.width + x];

            // Apply local formula: pixel > (local_mean - C)
            output.data[y * input.width + x] = (pixelValue > (localMean - C)) ? 255 : 0;
        }
    }
    return output;
}

// --- ADDED: Spectral Thresholding (Multi-mode) ---
// We use 1D K-means on the pixel intensities to find multiple threshold groups
GrayImage Thresholding::applySpectralThresholding(const GrayImage& input, int modes) {
    if (modes < 2) modes = 2; // Need at least 2 modes

    std::vector<int> centroids(modes);
    for(int i = 0; i < modes; i++) {
        centroids[i] = i * 255 / (modes - 1);
    }

    bool changed = true;
    while(changed) {
        std::vector<long> sums(modes, 0);
        std::vector<int> counts(modes, 0);

        for(uint8_t p : input.data) {
            int minDist = 256;
            int bestCluster = 0;
            for(int i = 0; i < modes; i++) {
                int dist = std::abs(p - centroids[i]);
                if(dist < minDist) {
                    minDist = dist;
                    bestCluster = i;
                }
            }
            sums[bestCluster] += p;
            counts[bestCluster]++;
        }

        changed = false;
        for(int i = 0; i < modes; i++) {
            if(counts[i] > 0) {
                int newC = sums[i] / counts[i];
                if(newC != centroids[i]) {
                    changed = true;
                    centroids[i] = newC;
                }
            }
        }
    }

    // Create the final output image with the new clustered gray levels
    GrayImage output = {input.width, input.height, std::vector<uint8_t>(input.data.size())};
    for(size_t i = 0; i < input.data.size(); i++) {
        int p = input.data[i];
        int minDist = 256;
        int bestCluster = 0;
        for(int c = 0; c < modes; c++) {
            int dist = std::abs(p - centroids[c]);
            if(dist < minDist) {
                minDist = dist;
                bestCluster = c;
            }
        }
        output.data[i] = (uint8_t)centroids[bestCluster];
    }
    return output;
}