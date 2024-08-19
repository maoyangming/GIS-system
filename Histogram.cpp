#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Histogram.h"
#include "MyGIS.h"

Histogram::Histogram(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 设置UI界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 连接信号和槽
	connect(ui.pushButton, &QPushButton::clicked, this, &Histogram::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &Histogram::actionNo);
	connect(ui.toolButton, &QToolButton::clicked, this, &Histogram::actionOpenFile);
}

// 析构函数，释放资源
Histogram::~Histogram()
{}

// 向对话框中添加图层列表，供用户选择
// 参数：
// - strlLayers: 包含图层名称的QStringList
void Histogram::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers); // 将图层名称添加到下拉菜单中
}

// 打开文件保存对话框的槽函数，供用户选择输出均衡化结果的文件路径
void Histogram::actionOpenFile() {
	QString strFilePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save File", // 对话框标题
		"", // 默认路径
		"GeoTiff (*.tif);;All Files (*)" // 文件过滤器
	);
	ui.lineEdit->setText(strFilePath); // 将选择的文件路径显示在行编辑框中
}

// 用户确认操作的槽函数，执行直方图均衡化处理和直方图绘制，并关闭对话框
void Histogram::actionYes() {
	// 创建一个新的QGraphicsScene用于绘制直方图
	QGraphicsScene* pNewScene = new QGraphicsScene(mpMainWindow->getUI().graphicsView_2);
	mpMainWindow->getUI().graphicsView_2->setScene(pNewScene); // 将新场景设置到视图中

	// 处理图像并绘制直方图
	processAndDrawHistogram(ui.comboBox->currentText(), pNewScene, 256);

	// 执行直方图均衡化处理并将结果保存
	HistogramEqualization(ui.comboBox->currentText(), ui.lineEdit->text());

	// 显示结果
	mpMainWindow->getUI().dockWidget_4->show();
	this->close(); // 关闭对话框
}

// 用户取消操作的槽函数，关闭对话框
void Histogram::actionNo() {
	this->close();
}

// 处理图像数据并绘制直方图
// 参数：
// - strFilename: 输入图像文件路径
// - pScene: QGraphicsScene对象，用于绘制直方图
// - nBins: 直方图的分箱数量
void Histogram::processAndDrawHistogram(const QString& strFilename, QGraphicsScene* pScene, int nBins) {
	GDALAllRegister();  // 注册所有GDAL驱动

	// 设置日志记录
	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("准备绘制灰度直方图");

	// 打开输入栅格数据集
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFilename.toStdString().c_str(), GA_ReadOnly));
	if (!poDataset) {
		log.warn("无法打开栅格数据集");
		return;
	}
	log.info("打开栅格数据集");

	// 获取栅格的宽度和高度
	int nWidth = poDataset->GetRasterXSize();
	int nHeight = poDataset->GetRasterYSize();
	GDALRasterBand* poBand = poDataset->GetRasterBand(1); // 获取第一波段
	log.info("获取波段");

	// 读取波段数据到缓冲区
	QVector<uchar> vData(nWidth * nHeight);
	poBand->RasterIO(GF_Read, 0, 0, nWidth, nHeight, vData.data(), nWidth, nHeight, GDT_Byte, 0, 0);
	GDALClose(poDataset); // 关闭数据集

	// 计算直方图
	QVector<int> vnHistogram(nBins, 0);
	log.info("计算直方图");

	// 使用OpenMP并行计算直方图
#pragma omp parallel
	{
		QVector<int> localHistogram(nBins, 0); // 每个线程的局部直方图

#pragma omp for
		for (int i = 0; i < vData.size(); ++i) {
			localHistogram[vData[i]]++;
		}

#pragma omp critical
		{
			for (int i = 0; i < nBins; ++i) {
				vnHistogram[i] += localHistogram[i];
			}
		}
	}

	log.info("开始绘制灰度直方图");

	// 绘制直方图
	int nBinWidth = 1;  // 每个条形的宽度
	int nMaxCount = *std::max_element(vnHistogram.begin(), vnHistogram.end()); // 直方图中的最大值

	for (int i = 0; i < vnHistogram.size(); ++i) {
		int nHeight = static_cast<int>(static_cast<double>(vnHistogram[i]) / nMaxCount * 100); // 归一化高度
		QGraphicsRectItem* pRect = new QGraphicsRectItem(i * nBinWidth, 100 - nHeight, nBinWidth, nHeight);
		pRect->setBrush(QBrush(Qt::blue)); // 设置矩形的填充颜色
		pScene->addItem(pRect); // 将矩形添加到场景中
	}
	log.info("绘制结束");
}


void Histogram::HistogramEqualization(const QString& strInputPath, const QString& strOutputPath) {
	GDALAllRegister();

	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("直方图均衡化增强显示");

	// 打开输入栅格
	GDALDataset* poDataset = (GDALDataset*)GDALOpen(strInputPath.toStdString().c_str(), GA_ReadOnly);
	if (poDataset == nullptr) {
		log.warn("无法打开栅格数据集");
		return;
	}
	log.info("打开栅格数据集");

	int nBands = poDataset->GetRasterCount();
	int nXSize = poDataset->GetRasterXSize();
	int nYSize = poDataset->GetRasterYSize();

	log.info("创建驱动");
	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	log.info("创建输出数据集");
	GDALDataset* poOutputDataset = poDriver->Create(strOutputPath.toStdString().c_str(), nXSize, nYSize, nBands, GDT_Byte, NULL);

	// 获取仿射变换参数
	double adfGeoTransform[6];
	poDataset->GetGeoTransform(adfGeoTransform);
	poOutputDataset->SetGeoTransform(adfGeoTransform);
	if (poDataset->GetProjectionRef()) {
		poOutputDataset->SetProjection(poDataset->GetProjectionRef());
	}

	std::vector<int> histogram(256, 0);  // 假设像素值为0到255的8位灰度图

	log.info("开始计算");
	for (int iBand = 1; iBand <= nBands; iBand++) {
		GDALRasterBand* poBand = poDataset->GetRasterBand(iBand);
		std::vector<unsigned char> imageData(nXSize * nYSize);
		poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, imageData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

		// 计算直方图
		std::fill(histogram.begin(), histogram.end(), 0);
		for (size_t i = 0; i < imageData.size(); i++) {
			histogram[imageData[i]]++;
		}

		// 计算累积分布函数（CDF）
		std::vector<int> cdf(256, 0);
		std::partial_sum(histogram.begin(), histogram.end(), cdf.begin());

		// CDF最小值
		int cdfMin = *std::find_if(cdf.begin(), cdf.end(), [](int value) { return value > 0; });

		// 应用直方图均衡化
		for (size_t i = 0; i < imageData.size(); i++) {
			int pixelValue = imageData[i];
			imageData[i] = static_cast<unsigned char>((cdf[pixelValue] - cdfMin) * 255 / (nXSize * nYSize - cdfMin));
		}

		// 将均衡化后的图像数据写入输出栅格
		GDALRasterBand* poOutputBand = poOutputDataset->GetRasterBand(iBand);
		poOutputBand->RasterIO(GF_Write, 0, 0, nXSize, nYSize, imageData.data(), nXSize, nYSize, GDT_Byte, 0, 0);
	}
	log.info("均衡化显示设置完毕");

	GDALClose(poDataset);
	GDALClose(poOutputDataset);
	mpMainWindow->importRaster(strOutputPath);
}

