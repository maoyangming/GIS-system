#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <vector>
#include <algorithm>
#include <numeric>

#include "ui_Histogram.h"
#include "omp.h"
#include "gdal_priv.h"

// 前向声明MyGIS类，避免循环依赖
class MyGIS;

// Histogram类继承自QDialog，用于展示图像直方图和进行直方图均衡化处理
class Histogram : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	// 参数：
	// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
	// - parent: 父窗口指针，默认值为nullptr
	Histogram(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~Histogram();

	// 向对话框中添加图层列表，供用户选择
	// 参数：
	// - strlLayers: 包含图层名称的QStringList
	void addLayer(const QStringList& strlLayers);

	// 对选定的栅格图像进行直方图均衡化处理，并保存结果
	// 参数：
	// - strInputPath: 输入图像文件路径
	// - strOutputPath: 输出均衡化结果文件路径
	void HistogramEqualization(const QString& strInputPath, const QString& strOutputPath);

	// 处理图像数据并绘制直方图
	// 参数：
	// - strFilename: 输入图像文件路径
	// - pScene: QGraphicsScene对象，用于绘制直方图
	// - nBins: 直方图的分箱数量
	void processAndDrawHistogram(const QString& strFilename, QGraphicsScene* pScene, int nBins);

public slots:
	// 打开文件选择对话框的槽函数，供用户选择输入或输出文件路径
	void actionOpenFile();

	// 用户确认操作的槽函数，执行直方图均衡化处理并关闭对话框
	void actionYes();

	// 用户取消操作的槽函数，关闭对话框
	void actionNo();

private:
	Ui::HistogramClass ui;  // UI对象，管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于在直方图处理和显示中与主窗口交互
	QVector<uchar> mvBuffer; // 缓存用于直方图处理的数据
};

