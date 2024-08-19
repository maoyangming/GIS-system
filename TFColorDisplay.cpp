#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "TFColorDisplay.h"
#include "MyGIS.h"

// 构造函数，初始化 UI 界面和主要变量
TFColorDisplay::TFColorDisplay(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)  // 调用父类 QDialog 的构造函数
{
	ui.setupUi(this);  // 初始化 UI 界面
	mpMainWindow = mainWindow;  // 将主窗口指针赋值给 mpMainWindow

	// 设置三个波段选择 SpinBox 的最小值为 1
	ui.spinBox->setMinimum(1);
	ui.spinBox_2->setMinimum(1);
	ui.spinBox_3->setMinimum(1);

	// 初始化 SpinBox，根据当前选中的图层设置其范围
	initialiseSpinBox(ui.comboBox->currentText());

	// 连接 "Yes" 按钮的点击信号到 actionYes 槽函数
	connect(ui.pushButton_2, &QPushButton::clicked, this, &TFColorDisplay::actionYes);

	// 连接 "No" 按钮的点击信号到 actionNo 槽函数
	connect(ui.pushButton, &QPushButton::clicked, this, &TFColorDisplay::actionNo);

	// 连接 ComboBox 的索引改变信号到 actionReread 槽函数，用于重新读取数据
	connect(ui.comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TFColorDisplay::actionReread);
}

// 析构函数
TFColorDisplay::~TFColorDisplay()
{}

// "Yes" 按钮的槽函数，显示合成图像并关闭对话框
void TFColorDisplay::actionYes() {
	// 调用 displayCompositeImage 显示选定波段的合成图像
	displayCompositeImage(ui.comboBox->currentText(),
		ui.spinBox->value(),   // 获取红波段选择的值
		ui.spinBox_2->value(), // 获取绿波段选择的值
		ui.spinBox_3->value()); // 获取蓝波段选择的值
	this->close();  // 关闭对话框
}

// "No" 按钮的槽函数，关闭对话框
void TFColorDisplay::actionNo() {
	this->close();  // 关闭对话框
}

// 重新读取图层数据的槽函数，更新 SpinBox 的范围
void TFColorDisplay::actionReread() {
	initialiseSpinBox(ui.comboBox->currentText());  // 重新初始化 SpinBox
}

// 添加图层到 ComboBox 的函数
void TFColorDisplay::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers);  // 将图层列表添加到 ComboBox 中
}


void TFColorDisplay::initialiseSpinBox(const QString& strFilePath) {
	GDALAllRegister();  // 注册所有GDAL驱动，以支持各种栅格格式

	// 打开指定文件路径的栅格数据集，只读方式
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFilePath.toStdString().c_str(), GA_ReadOnly));
	if (!poDataset) {
		return;  // 如果数据集无法打开，直接返回
	}

	// 获取栅格数据集中的波段数量
	int nCount = poDataset->GetRasterCount();

	// 设置三个SpinBox的最大值为波段数量，确保用户选择的波段在有效范围内
	ui.spinBox->setMaximum(nCount);
	ui.spinBox_2->setMaximum(nCount);
	ui.spinBox_3->setMaximum(nCount);

	GDALClose(poDataset);  // 关闭数据集，释放资源
}

void TFColorDisplay::displayCompositeImage(const QString& strFilePath, int nRedBand, int nGreenBand, int nBlueBand) {
	GDALAllRegister();  // 注册所有GDAL驱动，以支持各种栅格格式

	// 获取日志记录器并添加文件日志记录器
	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("进行波段组合");

	// 打开指定文件路径的栅格数据集，只读方式
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFilePath.toStdString().c_str(), GA_ReadOnly));
	if (!poDataset) {
		log.warn("无法打开栅格数据集");
		return;  // 如果数据集无法打开，记录警告信息并返回
	}
	log.info("打开栅格数据集");

	// 使用 QFileInfo 获取文件名
	QFileInfo fileInfo(strFilePath);
	QString strFileName = fileInfo.fileName();

	// 获取栅格图像的宽度和高度
	int nXSize = poDataset->GetRasterXSize();
	int nYSize = poDataset->GetRasterYSize();

	// 创建用于存储RGB波段数据的数组
	QVector<uchar> vRedBandData(nXSize * nYSize);
	QVector<uchar> vGreenBandData(nXSize * nYSize);
	QVector<uchar> vBlueBandData(nXSize * nYSize);

	// 确保 SpinBox 的最大值设置为当前数据集的波段数量
	int nCount = poDataset->GetRasterCount();
	ui.spinBox->setMaximum(nCount);
	ui.spinBox_2->setMaximum(nCount);
	ui.spinBox_3->setMaximum(nCount);

	// 读取指定的红色波段数据到数组
	GDALRasterBand* poRedBand = poDataset->GetRasterBand(nRedBand);
	poRedBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vRedBandData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

	// 读取指定的绿色波段数据到数组
	GDALRasterBand* poGreenBand = poDataset->GetRasterBand(nGreenBand);
	poGreenBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vGreenBandData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

	// 读取指定的蓝色波段数据到数组
	GDALRasterBand* poBlueBand = poDataset->GetRasterBand(nBlueBand);
	poBlueBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vBlueBandData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

	// 检查所有波段是否有效，如果无效则记录警告信息并显示错误消息
	if (!poRedBand || !poGreenBand || !poBlueBand) {
		log.warn("获取某一波段错误");
		QMessageBox::critical(this, "错误", "波段不存在");
		GDALClose(poDataset);  // 关闭数据集，释放资源
		return;
	}
	log.info("获取波段");

	// 组合波段生成RGB图像
	QImage* pImage = new QImage(nXSize, nYSize, QImage::Format_RGB32);
#pragma omp parallel for  // 使用OpenMP并行处理，提升处理速度
	for (int y = 0; y < nYSize; ++y) {
		for (int x = 0; x < nXSize; ++x) {
			int idx = y * nXSize + x;
			pImage->setPixel(x, y, qRgb(vRedBandData[idx], vGreenBandData[idx], vBlueBandData[idx]));
		}
	}
	log.info("Image设置成功");

	// 获取栅格数据集的仿射变换参数
	double adfGeoTransform[6];
	if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
		double originX = adfGeoTransform[0];  // 左上角X坐标
		double originY = adfGeoTransform[3];  // 左上角Y坐标
		double pixelWidth = adfGeoTransform[1];  // 每个像素的宽度
		double pixelHeight = adfGeoTransform[5];  // 每个像素的高度（通常为负值）

		// 将生成的图像添加到场景中
		QGraphicsPixmapItem* pPixmapItem = mpMainWindow->getScene()->addPixmap(QPixmap::fromImage(*pImage));
		QTransform transform;
		transform.scale(1, -1);  // 垂直翻转图像以适应地理坐标系

		// 设置图像的位置和缩放比例
		pPixmapItem->setPos(originX, originY);
		pPixmapItem->setScale(pixelWidth);
		pPixmapItem->setTransform(transform);

		// 创建新的栅格图层对象并设置其属性
		MyRasterLayer* pNewLayer = new MyRasterLayer(strFilePath, strFileName, nXSize, nYSize);
		pNewLayer->setRasterData(pPixmapItem);
		pNewLayer->setOriginX(originX);
		pNewLayer->setOriginY(originY);
		pNewLayer->setPixelWidth(pixelWidth);
		pNewLayer->setPixelHeight(pixelHeight);

		// 将新图层添加到图层管理器中
		mpMainWindow->getLayerManager()->addRasterLayer(pNewLayer);
		mpMainWindow->getLayerManager()->mlRasterLayers.append(strFilePath);

		// 获取地理范围，并调整视图以适应图像
		QRectF boundingRect(QPointF(originX, originY + nYSize * pixelHeight),
			QSizeF(nXSize * pixelWidth, nYSize * std::abs(pixelHeight)));
		mpMainWindow->getUI().graphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
	}
	else {
		log.warn("获取仿射变换参数失败");  // 记录警告信息
	}

	GDALClose(poDataset);  // 关闭数据集，释放资源
	return;
}

