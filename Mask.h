#pragma once

#include <QDialog>

#include "gdal_priv.h"
#include "gdal_alg.h"
#include "gdal_alg_priv.h"
#include "gdal_utils.h"
#include "ogr_spatialref.h"
#include "ogr_geometry.h"
#include "ogr_api.h"
#include "cpl_conv.h"
#include "cpl_error.h"
#include "omp.h"
#include "ui_Mask.h"

// 前向声明MyGIS类，避免循环依赖
class MyGIS;

// Mask类继承自QDialog，用于执行栅格和矢量数据的掩膜操作
class Mask : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	// 参数：
	// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
	// - parent: 父窗口指针，默认值为nullptr
	Mask(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~Mask();

	// 向对话框中添加图层列表，供用户选择掩膜图层和目标图层
	// 参数：
	// - strlLayers1: 掩膜图层的QStringList
	// - strlLayers2: 目标图层的QStringList
	void addLayer(const QStringList& strlLayers1, const QStringList& strlLayers2);

	// 执行栅格数据的掩膜操作，并将结果保存到指定路径
	// 参数：
	// - strInputMask: 输入的掩膜图层文件路径
	// - strInputRas: 输入的目标栅格文件路径
	// - strOutput: 输出掩膜结果的文件路径
	void maskRaster(const QString& strInputMask, const QString& strInputRas, const QString& strOutput);

	// 执行矢量数据的掩膜操作，并将结果保存到指定路径
	// 参数：
	// - strInputMask: 输入的掩膜图层文件路径
	// - strInputRas: 输入的目标矢量文件路径
	// - strOutput: 输出掩膜结果的文件路径
	void maskVector(const QString& strInputMask, const QString& strInputRas, const QString& strOutput);

	// 判断给定文件名是否为矢量数据
	// 参数：
	// - strFileName: 要检查的文件名
	// 返回值：如果是矢量数据，返回true，否则返回false
	bool isVectorGDAL(const QString& strFileName);

	// 判断给定文件名是否为栅格数据
	// 参数：
	// - strFileName: 要检查的文件名
	// 返回值：如果是栅格数据，返回true，否则返回false
	bool isRasterGDAL(const QString& strFileName);

public slots:
	// 打开文件选择对话框的槽函数，供用户选择输入或输出文件路径
	void actionOpenFile();

	// 用户确认操作的槽函数，执行掩膜操作并关闭对话框
	void actionYes();

	// 用户取消操作的槽函数，关闭对话框
	void actionNo();

private:
	Ui::MaskClass ui;  // UI对象，管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于在掩膜操作中与主窗口交互
};
