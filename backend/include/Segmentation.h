#pragma once
#include "image.h"
#include <vector>

class Segmentation {
public:
    // 1. K-Means Clustering
    static ColorImage applyKMeans(const ColorImage& input, int k, int maxIterations = 10);

    // 2. Region Growing 
    static GrayImage applyRegionGrowing(const GrayImage& input, int seedX, int seedY, int threshold);

    // 3. Mean Shift Method
    static ColorImage applyMeanShift(const ColorImage& input, float spatialRadius, float colorRadius, int maxIterations = 5);

    // 4. Agglomerative Clustering
    static ColorImage applyAgglomerative(const ColorImage& input, int targetClusters);

private:
    // Helper function for 3D Euclidean distance between RGB pixels
    static float colorDistance(const RGB& c1, const RGB& c2);
};