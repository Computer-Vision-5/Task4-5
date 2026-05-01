#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "include/Thresholding.h"
#include "include/image.h"

namespace {
GrayImage createFallbackImage() {
    GrayImage img;
    img.width = 64;
    img.height = 64;
    img.data.resize(static_cast<size_t>(img.width) * static_cast<size_t>(img.height));

    for (int y = 0; y < img.height; ++y) {
        for (int x = 0; x < img.width; ++x) {
            const size_t index = static_cast<size_t>(y) * static_cast<size_t>(img.width) + static_cast<size_t>(x);
            img.data[index] = static_cast<uint8_t>((x * 4 + y * 2) % 256);
        }
    }

    return img;
}
} // namespace

int main(int argc, char* argv[]) {
    const std::string inputPath = (argc > 1) ? argv[1] : "test_image.pgm";
    const std::string otsuOutputPath = (argc > 2) ? argv[2] : "output_otsu.pgm";
    const std::string optimalOutputPath = (argc > 3) ? argv[3] : "output_optimal.pgm";

    GrayImage inputImg;

    std::cout << "Loading image from '" << inputPath << "'..." << std::endl;
    if (!PgmIO::readPGM(inputPath, inputImg)) {
        std::cerr << "Warning: could not load '" << inputPath
                  << "'. Using a generated fallback image so the app can still run." << std::endl;
        inputImg = createFallbackImage();
    }

    std::cout << "Image ready: " << inputImg.width << "x" << inputImg.height << std::endl;

    std::cout << "Applying Otsu Thresholding..." << std::endl;
    GrayImage otsuResult = Thresholding::applyOtsuThresholding(inputImg);

    std::cout << "Applying Optimal Thresholding..." << std::endl;
    GrayImage optimalResult = Thresholding::applyOptimalThresholding(inputImg);

    std::cout << "Saving results..." << std::endl;
    const bool otsuSaved = PgmIO::writePGM(otsuOutputPath, otsuResult);
    const bool optimalSaved = PgmIO::writePGM(optimalOutputPath, optimalResult);

    if (otsuSaved && optimalSaved) {
        std::cout << "Success! Wrote '" << otsuOutputPath << "' and '" << optimalOutputPath << "'." << std::endl;
        return 0;
    }

    std::cerr << "Failed to save one or more output files." << std::endl;
    return 1;
}