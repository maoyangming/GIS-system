#pragma once

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>

#include "ui_TFColorDisplay.h"
#include "omp.h"
#include "gdal_priv.h"

// 前向声明 MyGIS 类，避免循环依赖
class MyGIS;

// TFColorDisplay 类，用于显示和处理多波段合成图像
class TFColorDisplay : public QDialog
{
	Q_OBJECT

public:
	// 构造函数，初始化主窗口指针和父窗口
	TFColorDisplay(MyGIS* mainWindow, QWidget* parent = nullptr);
	// 析构函数
	~TFColorDisplay();

	// 添加图层到显示列表中
	void addLayer(const QStringList& strlLayers);

	// 显示多波段合成图像，指定文件路径和 RGB 波段
	void displayCompositeImage(const QString& strFilePath, int nRedBand, int nGreenBand, int nBlueBand);

	// 初始化 SpinBox（选择波段的输入框），根据文件路径读取波段信息
	void initialiseSpinBox(const QString& strFilePath);

public slots:
	// "Yes" 按钮的槽函数，执行相关操作
	void actionYes();

	// "No" 按钮的槽函数，取消操作
	void actionNo();

	// "Reread" 按钮的槽函数，重新读取图像或数据
	void actionReread();

private:
	Ui::TFColorDisplayClass ui;  // UI 类实例，用于管理界面元素
	MyGIS* mpMainWindow;         // 指向主窗口的指针，方便与主应用程序交互
};
