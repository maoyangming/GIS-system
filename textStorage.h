#pragma once

#ifndef TEXTSTORAGE_H
#define TEXTSTORAGE_H

#include "vectorStorage.h"
#include <string>

// textStorage类继承自vectorStorage，负责将矢量数据存储到文本文件中
class textStorage : public vectorStorage {
public:
	// 构造函数，初始化textStorage对象
	// 参数：
	// - filePath: 指定文本文件的路径，用于存储矢量数据
	textStorage(const std::string& filePath);

	// 重写的虚函数，将矢量数据存储到指定的文本文件中
	// 参数：
	// - layer: MyVectorLayer对象，表示要存储的矢量图层数据
	void storeVectorData(const MyVectorLayer& layer) override;

private:
	std::string mFilePath; // 用于存储矢量数据的文本文件路径
};

#endif //TEXTSTORAGE_H


