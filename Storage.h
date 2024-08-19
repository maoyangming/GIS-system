#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "MyLayer.h"

class VectorStorage {
public:
    virtual ~VectorStorage() {}

    // 存储矢量数据
    virtual bool storeVectorData(const MyVectorLayer& vectorLayer) = 0;
};

class TextFileVectorStorage : public VectorStorage {
private:
    std::string mFilePath;

public:
    TextFileVectorStorage(const std::string& filePath) : mFilePath(filePath) {}

    bool storeVectorData(const MyVectorLayer& vectorLayer) override {
        std::ofstream file(mFilePath);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open file for writing." << std::endl;
            return false;
        }

        // 写入图层基本信息
        file << vectorLayer.getFilePath().toStdString() << std::endl;
        file << vectorLayer.getName().toStdString() << std::endl;
        file << static_cast<int>(vectorLayer.getType()) << std::endl;

        file.close();
        return true;
    }
};

class BinaryFileVectorStorage : public VectorStorage {
private:
    std::string filePath;

public:
    BinaryFileVectorStorage(const std::string& filePath) : filePath(filePath) {}

    bool storeVectorData(const MyVectorLayer& vectorLayer) override {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Failed to open file for writing." << std::endl;
            return false;
        }

        // 写入图层基本信息
        std::string layerPath = vectorLayer.getFilePath().toStdString();
        size_t pathLength = layerPath.size();
        file.write(reinterpret_cast<const char*>(&pathLength), sizeof(pathLength));
        file.write(layerPath.c_str(), pathLength);

        std::string layerName = vectorLayer.getName().toStdString();
        size_t nameLength = layerName.size();
        file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
        file.write(layerName.c_str(), nameLength);

        int type = static_cast<int>(vectorLayer.getType());
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));

        file.close();
        return true;
    }
};


