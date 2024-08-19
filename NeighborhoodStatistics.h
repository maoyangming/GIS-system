#pragma once

#include <QDialog>
#include <vector>
#include <algorithm>
#include <numeric>
#include <omp.h>

#include "ui_NeighborhoodStatistics.h"
#include "gdal_priv.h"
#include "cpl_conv.h"

// 枚举类，用于指定统计方法
enum class HowToStatistic {
	MEAN,  // 计算均值
	MAX,   // 计算最大值
	MIN,   // 计算最小值
	UNKNOW // 未知方法（默认值）
};

class MyGIS;

// NeighborhoodStatistics 类继承自 QDialog，用于执行栅格数据的邻域统计操作
class NeighborhoodStatistics : public QDialog
{
	Q_OBJECT

public:
	// 构造函数
	NeighborhoodStatistics(MyGIS* mainWindow, QWidget* parent = nullptr);

	// 析构函数
	~NeighborhoodStatistics();

	// 向对话框中添加图层列表，供用户选择统计的栅格数据图层
	// 参数：
	// - strlLayers: 包含图层名称的 QStringList
	void addLayer(const QStringList& strlLayers);

	// 计算给定值列表的均值
	// 参数：
	// - vfValues: 包含要计算的值的 std::vector<float>
	// 返回值：均值
	float calculateMean(const std::vector<float>& vfValues);

	// 计算给定值列表的最大值
	// 参数：
	// - vfValues: 包含要计算的值的 std::vector<float>
	// 返回值：最大值
	float calculateMax(const std::vector<float>& vfValues);

	// 计算给定值列表的最小值
	// 参数：
	// - vfValues: 包含要计算的值的 std::vector<float>
	// 返回值：最小值
	float calculateMin(const std::vector<float>& vfValues);

	// 执行邻域统计操作，并将结果保存到指定路径
	// 参数：
	// - strInputRas: 输入的栅格文件路径
	// - strOutputRas: 输出的栅格文件路径
	// - nSize: 邻域窗口的大小（通常为奇数，如 3, 5, 7）
	// - how: 使用的统计方法（均值、最大值、最小值）
	void neighborhoodStatistics(const QString& strInputRas, const QString& strOutputRas, int nSize, HowToStatistic how);

public slots:
	// 打开文件保存对话框的槽函数，供用户选择输出文件路径
	void actionOpenFile();

	// 用户确认操作的槽函数，执行邻域统计并关闭对话框
	void actionYes();

	// 用户取消操作的槽函数，关闭对话框
	void actionNo();

private:
	Ui::NeighborhoodStatisticsClass ui; // UI 对象，用于管理和访问对话框的控件
	MyGIS* mpMainWindow; // 指向主窗口的指针，用于与主窗口交互
};
