#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "Thresholding.h"

// --- Added for Part 2: Color Image Support ---
struct RGB {
    uint8_t r, g, b;
};

struct ColorImage {
    int width;
    int height;
    std::vector<RGB> data;
};

// --- Grayscale IO (Existing) ---
class PgmIO {
public:
    static bool readPGM(const std::string& filename, GrayImage& img) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        std::string magic;
        file >> magic;
        if (magic != "P5") {
            std::cerr << "Error: Only P5 binary PGM files are supported." << std::endl;
            return false;
        }

        // Skip comments
        char c;
        file >> c;
        while (c == '#') {
            std::string dummy;
            std::getline(file, dummy);
            file >> c;
        }
        file.putback(c);

        int maxVal;
        file >> img.width >> img.height >> maxVal;
        file.ignore(256, '\n'); // Skip to the binary pixel data

        img.data.resize(img.width * img.height);
        file.read(reinterpret_cast<char*>(img.data.data()), img.data.size());

        return file.good();
    }

    static bool writePGM(const std::string& filename, const GrayImage& img) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Write PGM Header
        file << "P5\n" << img.width << " " << img.height << "\n255\n";

        // Write binary pixel data
        file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());

        return file.good();
    }
};

// --- Color IO for Segmentation ---
class PpmIO {
public:
    static bool readPPM(const std::string& filename, ColorImage& img) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        std::string magic;
        file >> magic;
        if (magic != "P6") {
            std::cerr << "Error: Only P6 binary PPM files are supported." << std::endl;
            return false;
        }

        char c;
        file >> c;
        while (c == '#') {
            std::string dummy;
            std::getline(file, dummy);
            file >> c;
        }
        file.putback(c);

        int maxVal;
        file >> img.width >> img.height >> maxVal;
        file.ignore(256, '\n');

        img.data.resize(img.width * img.height);
        file.read(reinterpret_cast<char*>(img.data.data()), img.data.size() * 3);

        return file.good();
    }

    static bool writePPM(const std::string& filename, const ColorImage& img) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        file << "P6\n" << img.width << " " << img.height << "\n255\n";
        file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size() * 3);

        return file.good();
    }
};










































































// #pragma once
// #include <string>
// #include <fstream>
// #include <iostream>
// #include <sstream>
// #include "Thresholding.h"

// class PgmIO {
// public:
//     static bool readPGM(const std::string& filename, GrayImage& img) {
//         std::ifstream file(filename, std::ios::binary);
//         if (!file.is_open()) return false;

//         std::string magic;
//         file >> magic;
//         if (magic != "P5") {
//             std::cerr << "Error: Only P5 binary PGM files are supported." << std::endl;
//             return false;
//         }

//         // Skip comments
//         char c;
//         file >> c;
//         while (c == '#') {
//             std::string dummy;
//             std::getline(file, dummy);
//             file >> c;
//         }
//         file.putback(c);

//         int maxVal;
//         file >> img.width >> img.height >> maxVal;
//         file.ignore(256, '\n'); // Skip to the binary pixel data

//         img.data.resize(img.width * img.height);
//         file.read(reinterpret_cast<char*>(img.data.data()), img.data.size());

//         return file.good();
//     }

//     static bool writePGM(const std::string& filename, const GrayImage& img) {
//         std::ofstream file(filename, std::ios::binary);
//         if (!file.is_open()) return false;

//         // Write PGM Header
//         file << "P5\n" << img.width << " " << img.height << "\n255\n";
        
//         // Write binary pixel data
//         file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());

//         return file.good();
//     }
// };