#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Mask.h"
#include "MyGIS.h"

Mask::Mask(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 初始化UI界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 连接信号和槽
	connect(ui.toolButton, &QToolButton::clicked, this, &Mask::actionOpenFile);
	connect(ui.pushButton, &QPushButton::clicked, this, &Mask::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &Mask::actionNo);
}

// 析构函数，释放资源
Mask::~Mask()
{}

// 打开文件保存对话框的槽函数，供用户选择输出掩膜结果的文件路径
void Mask::actionOpenFile() {
	QString strFilePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save File", // 对话框标题
		"", // 默认路径
		"GeoTiff (*.tif)" // 文件过滤器
	);
	ui.lineEdit->setText(strFilePath); // 将选择的文件路径显示在行编辑框中
}

// 用户确认操作的槽函数，执行掩膜操作并关闭对话框
void Mask::actionYes() {
	if (isVectorGDAL(ui.comboBox->currentText())) {
		// 如果选择的是矢量数据，则执行矢量掩膜操作
		maskVector(ui.comboBox->currentText(), ui.comboBox_2->currentText(), ui.lineEdit->text());
	}
	else if (isRasterGDAL(ui.comboBox->currentText())) {
		// 如果选择的是栅格数据，则执行栅格掩膜操作
		maskRaster(ui.comboBox->currentText(), ui.comboBox_2->currentText(), ui.lineEdit->text());
	}
	this->close(); // 关闭对话框
}

// 用户取消操作的槽函数，关闭对话框
void Mask::actionNo() {
	this->close();
}

// 向对话框中添加图层列表，供用户选择掩膜图层和目标图层
// 参数：
// - strlLayers1: 掩膜图层的QStringList
// - strlLayers2: 目标图层的QStringList
void Mask::addLayer(const QStringList& strlLayers1, const QStringList& strlLayers2) {
	ui.comboBox->addItems(strlLayers1); // 将掩膜图层名称添加到第一个下拉菜单中
	ui.comboBox->addItems(strlLayers2); // 将目标图层名称添加到第一个下拉菜单中
	ui.comboBox_2->addItems(strlLayers2); // 将目标图层名称添加到第二个下拉菜单中
}

// 判断给定文件名是否为矢量数据
// 参数：
// - strFileName: 要检查的文件名
// 返回值：如果是矢量数据，返回true，否则返回false
bool Mask::isVectorGDAL(const QString& strFileName) {
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpenEx(strFileName.toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL));
	if (poDataset != nullptr) {
		GDALClose(poDataset);
		return true;
	}
	return false;
}

// 判断给定文件名是否为栅格数据
// 参数：
// - strFileName: 要检查的文件名
// 返回值：如果是栅格数据，返回true，否则返回false
bool Mask::isRasterGDAL(const QString& strFileName) {
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFileName.toStdString().c_str(), GA_ReadOnly));
	if (poDataset != nullptr) {
		GDALClose(poDataset);
		return true;
	}
	return false;
}

// 执行栅格数据的掩膜操作，并将结果保存到指定路径
// 参数：
// - strInputMask: 输入的掩膜图层文件路径
// - strInputRas: 输入的目标栅格文件路径
// - strOutput: 输出掩膜结果的文件路径
void Mask::maskRaster(const QString& strInputMask, const QString& strInputRas, const QString& strOutput) {
	GDALAllRegister(); // 注册所有GDAL驱动

	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("栅格掩膜");

	// 打开输入栅格数据集和掩膜数据集
	GDALDataset* poRDS = static_cast<GDALDataset*>(GDALOpen(strInputRas.toStdString().c_str(), GA_ReadOnly));
	GDALDataset* poMaskDS = static_cast<GDALDataset*>(GDALOpen(strInputMask.toStdString().c_str(), GA_ReadOnly));
	if (poRDS == nullptr || poMaskDS == nullptr) {
		// 如果打开失败，则关闭数据集并记录警告信息
		GDALClose(poRDS);
		GDALClose(poMaskDS);
		log.warn("无法打开栅格数据集");
		return;
	}
	log.info("打开栅格数据集");

	// 创建输出数据集
	GDALDataset* poOutput = GetGDALDriverManager()->GetDriverByName("GTiff")->CreateCopy(strOutput.toStdString().c_str(), poRDS, FALSE, NULL, NULL, NULL);
	if (poOutput == nullptr) {
		// 如果创建输出数据集失败，关闭数据集并记录警告信息
		GDALClose(poRDS);
		GDALClose(poMaskDS);
		log.warn("创建输出数据集失败");
		return;
	}

	int nMaskBand = 1;
	int nMaskWidth = poMaskDS->GetRasterXSize();
	int nMaskHeight = poMaskDS->GetRasterYSize();
	int nInputWidth = poRDS->GetRasterXSize();
	int nInputHeight = poRDS->GetRasterYSize();
	int nPixelCount = nInputWidth * nInputHeight;

	// 读取掩膜数据
	log.info("获取掩膜数据");
	std::vector<float> pfMaskData(nMaskWidth * nMaskHeight);
	GDALRasterBand* pMaskBand = poMaskDS->GetRasterBand(nMaskBand);
	pMaskBand->RasterIO(GF_Read, 0, 0, nMaskWidth, nMaskHeight, pfMaskData.data(), nMaskWidth, nMaskHeight, GDT_Float32, 0, 0);

	log.info("开始掩膜");
#pragma omp parallel
	{
		std::vector<float> pfInputData(nPixelCount); // 存储输入栅格数据
		std::vector<float> pfOutputData(nPixelCount); // 存储输出掩膜数据

#pragma omp for
		for (int iBand = 1; iBand <= poRDS->GetRasterCount(); iBand++) {
			GDALRasterBand* pInputBand = poRDS->GetRasterBand(iBand);
			GDALRasterBand* pOutputBand = poOutput->GetRasterBand(iBand);

			// 读取输入栅格数据
			pInputBand->RasterIO(GF_Read, 0, 0, nInputWidth, nInputHeight, pfInputData.data(), nInputWidth, nInputHeight, GDT_Float32, 0, 0);

			// 应用掩膜，将掩膜区域的像素值保留，其他区域置为0
			for (int j = 0; j < nPixelCount; j++) {
				if (pfMaskData[j] != 0) {
					pfOutputData[j] = pfInputData[j];
				}
				else {
					pfOutputData[j] = 0;
				}
			}

			// 写入输出栅格数据
			pOutputBand->RasterIO(GF_Write, 0, 0, nInputWidth, nInputHeight, pfOutputData.data(), nInputWidth, nInputHeight, GDT_Float32, 0, 0);
		}
	}

	// 关闭数据集并释放资源
	GDALClose(poRDS);
	GDALClose(poMaskDS);
	GDALClose(poOutput);

	// 将生成的掩膜文件导入到主窗口
	mpMainWindow->importRaster(strOutput);
}



void Mask::maskVector(const QString& strInputMask, const QString& strInputRas, const QString& strOutput) {
	GDALAllRegister();
	OGRRegisterAll();

	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("矢量掩膜");

	// 打开输入栅格
	GDALDataset* poRDS = static_cast<GDALDataset*>(GDALOpen(strInputRas.toStdString().c_str(), GA_ReadOnly));
	if (poRDS == nullptr) {
		log.warn("无法打开栅格数据集");
		return;
	}
	log.info("打开栅格数据集");

	// 打开矢量文件
	GDALDataset* poVDS = static_cast<GDALDataset*>(GDALOpenEx(strInputMask.toStdString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL));
	if (poVDS == nullptr) {
		log.warn("无法打开矢量数据集");
		GDALClose(poRDS);
		return;
	}
	log.info("打开矢量数据集");

	// 获取矢量图层
	OGRLayer* poLayer = poVDS->GetLayer(0);
	if (poLayer == nullptr) {
		log.warn("获取矢量图层失败");
		GDALClose(poRDS);
		GDALClose(poVDS);
		return;
	}
	log.info("获取矢量数据集");

	// 创建内存栅格，用于存储掩膜
	log.info("创建内存栅格");
	int nInputWidth = poRDS->GetRasterXSize();
	int nInputHeight = poRDS->GetRasterYSize();
	GDALDriver* poMemDriver = GetGDALDriverManager()->GetDriverByName("MEM");
	GDALDataset* poMaskDS = poMemDriver->Create("", nInputWidth, nInputHeight, 1, GDT_Byte, NULL);

	// 获取仿射变换参数
	log.info("获取仿射变换参数");
	double adfGeoTransform[6];
	poRDS->GetGeoTransform(adfGeoTransform);
	poMaskDS->SetGeoTransform(adfGeoTransform);
	if (poRDS->GetProjectionRef()) {
		poMaskDS->SetProjection(poRDS->GetProjectionRef());
	}

	// 设置栅格化的参数
	int nBands = 1;  // 目标栅格波段数
	int bandList[] = { 1 };  // 波段列表（只针对波段1）
	double burnValues[] = { 255.0 };  // “烧入”值，将矢量图层的内容以值255写入栅格

	char** papszOptions = NULL;
	papszOptions = CSLSetNameValue(papszOptions, "ALL_TOUCHED", "TRUE"); // 涉及的所有像素都标记
	//papszOptions = CSLSetNameValue(papszOptions, "BURN_VALUE_FROM", "Z"); // 根据某个属性值栅格化

	// 栅格化矢量图层为掩膜
	log.info("栅格化矢量图层");
	OGRLayerH hLayerArray[] = { poLayer };
	CPLErr err = GDALRasterizeLayers(
		poMaskDS,            // 目标栅格数据集
		nBands,              // 栅格波段数
		bandList,            // 波段列表
		1,                   // 矢量图层数量
		hLayerArray,         // 矢量图层句柄数组
		NULL,                // 变换函数（无）
		NULL,                // 变换参数（无）
		burnValues,          // “烧入”值数组
		papszOptions,        // 选项
		NULL,                // 进度回调函数（无）
		NULL                 // 进度回调参数（无）
	);

	if (err != CE_None) {
		log.warn("栅格化失败");
		GDALClose(poRDS);
		GDALClose(poVDS);
		GDALClose(poMaskDS);
		return;
	}

	// 创建输出栅格文件
	log.info("创建输出栅格数据集");
	GDALDataset* poOutput = GetGDALDriverManager()->GetDriverByName("GTiff")->CreateCopy(strOutput.toStdString().c_str(), poRDS, FALSE, NULL, NULL, NULL);
	if (poOutput == nullptr) {
		log.warn("创建输出数据集失败");
		GDALClose(poRDS);
		GDALClose(poVDS);
		GDALClose(poMaskDS);
		return;
	}

	// 读取掩膜数据并应用到输出栅格
	log.info("读取掩膜数据");
	GDALRasterBand* pMaskBand = poMaskDS->GetRasterBand(1);
	std::vector<unsigned char> maskData(nInputWidth * nInputHeight);
	pMaskBand->RasterIO(GF_Read, 0, 0, nInputWidth, nInputHeight, maskData.data(), nInputWidth, nInputHeight, GDT_Byte, 0, 0);

	std::vector<float> inputData(nInputWidth * nInputHeight);
	std::vector<float> outputData(nInputWidth * nInputHeight);

	log.info("掩膜数据应用到输出栅格");
	// 使用 OpenMP 并行化处理每个波段的数据
#pragma omp parallel
	{
		std::vector<float> localInputData(nInputWidth * nInputHeight);
		std::vector<float> localOutputData(nInputWidth * nInputHeight);

#pragma omp for
		for (int iBand = 1; iBand <= poRDS->GetRasterCount(); iBand++) {
			GDALRasterBand* pInputBand = poRDS->GetRasterBand(iBand);
			GDALRasterBand* pOutputBand = poOutput->GetRasterBand(iBand);

			pInputBand->RasterIO(GF_Read, 0, 0, nInputWidth, nInputHeight, localInputData.data(), nInputWidth, nInputHeight, GDT_Float32, 0, 0);

			// 应用掩膜
			for (int j = 0; j < nInputWidth * nInputHeight; j++) {
				if (maskData[j] == 255) {  // 255代表被掩膜区域
					localOutputData[j] = localInputData[j];
				}
				else {
					localOutputData[j] = 0;  // 掩膜外区域填充为0
				}
			}

			pOutputBand->RasterIO(GF_Write, 0, 0, nInputWidth, nInputHeight, localOutputData.data(), nInputWidth, nInputHeight, GDT_Float32, 0, 0);
		}
	}

	// 关闭所有打开的GDAL对象
	GDALClose(poRDS);
	GDALClose(poVDS);
	GDALClose(poOutput);
	GDALClose(poMaskDS);
	mpMainWindow->importRaster(strOutput);
}