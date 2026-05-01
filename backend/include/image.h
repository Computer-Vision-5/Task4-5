#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Thresholding.h"

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