#pragma once

#include <QStandardItem>

#include "MyLayer.h"

// MyItem类继承自QStandardItem，用于在Qt的模型-视图框架中表示图层项
class MyItem : public QStandardItem {
public:
	// 默认构造函数
	MyItem() : QStandardItem() {}

	// 带文本参数的构造函数
	// 参数：
	// - text: 用于初始化QStandardItem的文本
	MyItem(const QString& text) : QStandardItem(text) {}

	// 带矢量图层指针和文本参数的构造函数
	// 参数：
	// - pLayer: 指向MyVectorLayer对象的指针
	// - text: 用于初始化QStandardItem的文本
	MyItem(MyVectorLayer* pLayer, const QString& text) : QStandardItem(text) {
		mpVectorLayer = pLayer;
		mpRasterLayer = nullptr;
	}

	// 带栅格图层指针和文本参数的构造函数
	// 参数：
	// - pLayer: 指向MyRasterLayer对象的指针
	// - text: 用于初始化QStandardItem的文本
	MyItem(MyRasterLayer* pLayer, const QString& text) : QStandardItem(text) {
		mpRasterLayer = pLayer;
		mpVectorLayer = nullptr;
	}

	// 析构函数
	// 负责清理指向的图层对象，避免内存泄漏
	~MyItem() {
		if (mpVectorLayer) {
			delete mpVectorLayer;
		}
		if (mpRasterLayer) {
			delete mpRasterLayer;
		}
		mpVectorLayer = nullptr;
		mpRasterLayer = nullptr;
	}

	// 获取矢量图层指针
	// 返回值：指向MyVectorLayer对象的指针
	MyVectorLayer* getVectorLayer() {
		return mpVectorLayer;
	}

	// 获取栅格图层指针
	// 返回值：指向MyRasterLayer对象的指针
	MyRasterLayer* getRasterLayer() {
		return mpRasterLayer;
	}

	// 设置矢量图层指针
	// 参数：
	// - pVectorLayer: 要设置的MyVectorLayer对象指针
	void setVectorLayer(MyVectorLayer* pVectorLayer) {
		mpVectorLayer = pVectorLayer;
	}

	// 设置栅格图层指针
	// 参数：
	// - pRasterLayer: 要设置的MyRasterLayer对象指针
	void setRasterLayer(MyRasterLayer* pRasterLayer) {
		mpRasterLayer = pRasterLayer;
	}

private:
	// 指向矢量图层对象的指针
	MyVectorLayer* mpVectorLayer;

	// 指向栅格图层对象的指针
	MyRasterLayer* mpRasterLayer;
};
