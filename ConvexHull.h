#pragma once

#include <QDialog>
#include <QGraphicsItem>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <iostream>
#include <fstream>

#include "ui_ConvexHull.h"
#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "ogr_geometry.h"
#include "omp.h"

// 前向声明MyGIS类，避免循环依赖
class MyGIS;

// ConvexHull类继承自QDialog，表示一个用于计算图层凸包的对话框
class ConvexHull : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	// 参数：
	// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
	// - parent: 父窗口指针，默认值为nullptr
	ConvexHull(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~ConvexHull();

	// 向对话框中添加图层列表，供用户选择
	// 参数：
	// - strlLayers: 包含图层名称的QStringList
	void addLayer(const QStringList& strlLayers);

	// 计算选定图层的凸包并将结果保存到指定文件路径
	// 参数：
	// - strLayer: 选定的图层名称
	// - strFilePath: 结果保存的文件路径
	void calculateConvexHull(const QString& strLayer, const QString& strFilePath);

public slots:
	// 打开文件选择对话框的槽函数，供用户选择保存结果的文件路径
	void actionOpenFile();

	// 用户确认操作的槽函数，开始计算凸包
	void actionYes();


	// 用户取消操作的槽函数，关闭对话框
	void actionNo();

private:
	Ui::ConvexHullClass ui;  // UI对象，管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于在凸包计算中与主窗口交互
};
