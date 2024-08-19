#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "NeighborhoodStatistics.h"
#include "MyGIS.h"

// 构造函数
// 初始化对话框和 UI，并设置默认值和信号槽连接
NeighborhoodStatistics::NeighborhoodStatistics(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 设置 UI 界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 设置窗口大小的最小值
	ui.spinBox->setMinimum(1);

	// 向统计方法的下拉菜单中添加选项
	ui.comboBox_2->addItem("Mean");
	ui.comboBox_2->addItem("Max");
	ui.comboBox_2->addItem("Min");

	// 连接按钮点击信号到对应的槽函数
	connect(ui.toolButton, &QToolButton::clicked, this, &NeighborhoodStatistics::actionOpenFile);
	connect(ui.pushButton, &QPushButton::clicked, this, &NeighborhoodStatistics::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &NeighborhoodStatistics::actionNo);
}

// 析构函数
NeighborhoodStatistics::~NeighborhoodStatistics()
{}

// 打开文件保存对话框，供用户选择输出文件路径
void NeighborhoodStatistics::actionOpenFile() {
	QString strFilePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save File",
		"",
		"GeoTiff (*.tif);;All Files (*)"
	);
	ui.lineEdit->setText(strFilePath);
}

// 当用户点击确认按钮时，执行邻域统计操作并关闭对话框
void NeighborhoodStatistics::actionYes() {
	// 根据用户选择的统计方法设置 howToStatistic
	HowToStatistic how = HowToStatistic::UNKNOW;
	if (ui.comboBox_2->currentText() == "Mean") {
		how = HowToStatistic::MEAN;
	}
	else if (ui.comboBox_2->currentText() == "Max") {
		how = HowToStatistic::MAX;
	}
	else if (ui.comboBox_2->currentText() == "Min") {
		how = HowToStatistic::MIN;
	}

	// 调用邻域统计函数
	neighborhoodStatistics(
		ui.comboBox->currentText(), // 输入栅格文件路径
		ui.lineEdit->text(), // 输出栅格文件路径
		ui.spinBox->value(), // 邻域窗口大小
		how // 统计方法
	);
	this->close(); // 关闭对话框
}

// 当用户点击取消按钮时，关闭对话框
void NeighborhoodStatistics::actionNo() {
	this->close();
}

// 向对话框中添加图层列表，供用户选择统计的栅格数据图层
void NeighborhoodStatistics::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers);
}

// 计算给定值列表的均值
float NeighborhoodStatistics::calculateMean(const std::vector<float>& vfValues) {
	if (vfValues.empty()) return 0.0f;
	float fSum = std::accumulate(vfValues.begin(), vfValues.end(), 0.0f); // 计算总和
	return fSum / vfValues.size(); // 计算均值
}

// 计算给定值列表的最大值
float NeighborhoodStatistics::calculateMax(const std::vector<float>& vfValues) {
	return *std::max_element(vfValues.begin(), vfValues.end());
}

// 计算给定值列表的最小值
float NeighborhoodStatistics::calculateMin(const std::vector<float>& vfValues) {
	return *std::min_element(vfValues.begin(), vfValues.end());
}

// 执行邻域统计操作，并将结果保存为新的栅格文件
void NeighborhoodStatistics::neighborhoodStatistics(const QString& strInputRas, const QString& strOutputRas, int nSize, HowToStatistic how) {
	GDALAllRegister(); // 注册所有 GDAL 驱动

	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("邻域统计");

	// 打开输入栅格数据集
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strInputRas.toStdString().c_str(), GA_ReadOnly));
	if (poDataset == nullptr) {
		log.warn("无法打开栅格数据集");
		return;
	}
	log.info("打开栅格数据集");

	GDALRasterBand* poBand = poDataset->GetRasterBand(1); // 获取第一个波段
	int nXSize = poBand->GetXSize(); // 获取栅格宽度
	int nYSize = poBand->GetYSize(); // 获取栅格高度
	int halfSize = nSize / 2; // 邻域窗口半径

	// 创建输出文件
	log.info("创建驱动");
	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
	log.info("创建输出数据集");
	GDALDataset* poOutput = poDriver->Create(strOutputRas.toStdString().c_str(), nXSize, nYSize, 1, GDT_Float32, nullptr);

	// 获取仿射变换参数并设置到输出数据集
	log.info("获取仿射变换参数");
	double adfGeoTransform[6];
	poDataset->GetGeoTransform(adfGeoTransform);
	poOutput->SetGeoTransform(adfGeoTransform);

	// 复制投影信息到输出数据集
	const char* pszProjection = poDataset->GetProjectionRef();
	if (pszProjection) {
		poOutput->SetProjection(pszProjection);
	}

	// 为每个像素分配缓冲区
	float* pfBuffer = new float[nXSize];

	log.info("开始进行邻域统计");

	// 遍历每一行
	for (int y = 0; y < nYSize; ++y) {
		int startY = (std::max)(0, y - halfSize);
		int endY = (std::min)(nYSize - 1, y + halfSize);

		// 读取窗口数据
		std::vector<float> windowData((endY - startY + 1) * nXSize);
		poBand->RasterIO(GF_Read, 0, startY, nXSize, endY - startY + 1,
			windowData.data(), nXSize, endY - startY + 1, GDT_Float32, 0, 0);

		for (int x = 0; x < nXSize; ++x) {
			int startX = (std::max)(0, x - halfSize);
			int endX = (std::min)(nXSize - 1, x + halfSize);

			std::vector<float> values;

			// 收集邻域窗口内的像素值
			for (int iy = 0; iy <= (endY - startY); ++iy) {
				for (int ix = startX; ix <= endX; ++ix) {
					values.push_back(windowData[iy * nXSize + ix]);
				}
			}

			// 根据选择的统计方法计算结果
			if (how == HowToStatistic::MEAN) {
				pfBuffer[x] = calculateMean(values);
			}
			else if (how == HowToStatistic::MAX) {
				pfBuffer[x] = calculateMax(values);
			}
			else if (how == HowToStatistic::MIN) {
				pfBuffer[x] = calculateMin(values);
			}
		}

		// 将计算结果写入输出栅格
		poOutput->GetRasterBand(1)->RasterIO(GF_Write, 0, y, nXSize, 1, pfBuffer, nXSize, 1, GDT_Float32, 0, 0);
	}

	delete[] pfBuffer; // 释放缓冲区
	GDALClose(poOutput); // 关闭输出数据集
	GDALClose(poDataset); // 关闭输入数据集
	mpMainWindow->importRaster(strOutputRas); // 导入输出栅格到主窗口
}

