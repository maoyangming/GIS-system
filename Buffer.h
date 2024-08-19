#pragma once

#include <QDialog>
#include <iostream>

#include "ui_Buffer.h"
#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "ogr_geometry.h"
#include "omp.h"

// 前向声明MyGIS类，避免循环依赖
class MyGIS;

// Buffer类继承自QDialog，表示一个用于创建缓冲区的对话框
class Buffer : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	// 参数：
	// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
	// - parent: 父窗口指针，默认值为nullptr
	Buffer(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~Buffer();

	// 向对话框中添加图层列表，供用户选择
	// 参数：
	// - strlLayers: 包含图层名称的QStringList
	void addLayer(const QStringList& strlLayers);

	// 创建缓冲区功能
	// 参数：
	// - strInput: 输入图层的文件路径
	// - strOutput: 输出缓冲区结果的文件路径
	// - dDistance: 缓冲区的距离参数
	void createBuffer(const QString& strInput, const QString& strOutput, double dDistance);

public slots:
	// 打开文件选择对话框的槽函数
	void actionOpenFile();

	// 确认操作的槽函数
	void actionYes();

	// 取消操作的槽函数
	void actionNo();

private:
	Ui::BufferClass ui;  // UI对象，管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于在缓冲区操作中与主窗口交互
};
