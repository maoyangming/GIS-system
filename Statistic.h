#pragma once

#include <QDialog>
#include <QTableWidget>

#include "ui_Statistic.h"

// 前向声明 MyGIS 类，避免循环依赖
class MyGIS;

// Statistic 类继承自 QDialog，用于提供统计和分析矢量数据的功能
class Statistic : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	// 参数：
	Statistic(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~Statistic();

	// 向对话框中添加图层列表，供用户选择分析的矢量数据图层
	// 参数：
	// - strlLayers: 包含图层名称的 QStringList
	void addLayer(const QStringList& strlLayers);

	// 分析指定路径下的矢量数据，并将结果显示在表格中
	// 参数：
	// - strFilePath: 要分析的矢量数据文件路径
	// - pTable: 用于显示分析结果的 QTableWidget 指针
	void analyzeVectorData(const QString& strFilePath, QTableWidget* pTable);

public slots:
	// 用户确认操作的槽函数，执行数据分析
	void actionYes();

	// 用户取消操作的槽函数，关闭对话框
	void actionNo();

private:
	Ui::StatisticClass ui; // UI 对象，用于管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于与主窗口进行交互
};

