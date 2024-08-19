#pragma once

#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include "vectorStorage.h"
#include <string>
#include <sqlite3.h>

class dataStorage : public vectorStorage {
public:

	// 构造函数，初始化dataStorage对象
	// 参数：
	// - filePath: 指定数据库文件的路径，用于存储矢量数据
	dataStorage(const std::string& dbFilePath);

	// 重写的虚函数，将矢量数据存储到指定的数据库文件中
	// 参数：
	// - layer: MyVectorLayer对象，表示要存储的矢量图层数据
	void storeVectorData(const MyVectorLayer& layer) override;

	// 析构函数声明
	~dataStorage();

private:
	sqlite3* mDb;
};

#endif // DATASTORAGE_H

