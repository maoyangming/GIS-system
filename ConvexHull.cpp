#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "ConvexHull.h"
#include "MyGIS.h"

// 构造函数，初始化ConvexHull对话框
// 参数：
// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
// - parent: 父窗口指针，默认为nullptr
ConvexHull::ConvexHull(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 设置UI界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 连接信号和槽
	connect(ui.toolButton, &QToolButton::clicked, this, &ConvexHull::actionOpenFile);
	connect(ui.pushButton, &QPushButton::clicked, this, &ConvexHull::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &ConvexHull::actionNo);
}

// 析构函数，释放资源
ConvexHull::~ConvexHull()
{}

// 打开文件保存对话框的槽函数，供用户选择保存凸包结果的文件路径
void ConvexHull::actionOpenFile() {
	QString strFilePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save File", // 对话框标题
		"", // 默认路径
		"Shapefile (*.shp)" // 文件过滤器
	);
	ui.lineEdit->setText(strFilePath); // 将选择的文件路径显示在行编辑框中
}

// 用户确认操作的槽函数，开始计算凸包并关闭对话框
void ConvexHull::actionYes() {
	calculateConvexHull(ui.comboBox->currentText(), ui.lineEdit->text());
	this->close();
}

// 用户取消操作的槽函数，关闭对话框
void ConvexHull::actionNo() {
	this->close();
}

// 向对话框中添加图层列表，供用户选择
// 参数：
// - strlLayers: 包含图层名称的QStringList
void ConvexHull::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers); // 将图层名称添加到下拉菜单中
}

// 计算选定图层的凸包，并将结果保存到指定文件路径
// 参数：
// - strLayer: 选定的图层名称
// - strFilePath: 结果保存的文件路径
void ConvexHull::calculateConvexHull(const QString& strLayer, const QString& strFilePath) {
	GDALAllRegister(); // 注册所有GDAL驱动
	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("开始计算凸包");

	// 打开选定的矢量数据集
	GDALDataset* poDS = static_cast<GDALDataset*>(GDALOpenEx(strLayer.toStdString().c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
	if (poDS == nullptr) {
		log.warn("无法打开数据集");
		return;
	}
	log.info("打开数据集");

	// 获取数据集中的第一个图层
	OGRLayer* poLayer = poDS->GetLayer(0);
	if (poLayer == nullptr) {
		GDALClose(poDS);
		log.warn("无法获取到图层");
		return;
	}
	log.info("获取到图层");

	std::vector<OGRGeometry*> geometries;
	OGRFeature* poFeature;
	poLayer->ResetReading();

	// 收集图层中的所有几何体
	while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
		OGRGeometry* poGeometry = poFeature->GetGeometryRef();
		if (poGeometry != nullptr) {
			geometries.push_back(poGeometry->clone());
		}
		OGRFeature::DestroyFeature(poFeature);
	}
	log.info("几何体收集完成");

	// 使用OpenMP并行处理，按批次合并几何体
	OGRGeometry* poMergedGeometry = nullptr;
	int batchSize = 100;  // 批次大小，可根据数据集大小调整
#pragma omp parallel for
	for (int i = 0; i < geometries.size(); i += batchSize) {
		OGRGeometry* poBatchGeometry = nullptr;
		for (int j = i; j < min(i + batchSize, static_cast<int>(geometries.size())); ++j) {
			if (poBatchGeometry == nullptr) {
				poBatchGeometry = geometries[j]->clone();
			}
			else {
				OGRGeometry* poTemp = poBatchGeometry->Union(geometries[j]);
				delete poBatchGeometry;
				poBatchGeometry = poTemp;
			}
		}

		// 合并批次结果（线程安全）
#pragma omp critical
		{
			if (poMergedGeometry == nullptr) {
				poMergedGeometry = poBatchGeometry->clone();
			}
			else {
				OGRGeometry* poTemp = poMergedGeometry->Union(poBatchGeometry);
				delete poMergedGeometry;
				poMergedGeometry = poTemp;
			}
		}
		delete poBatchGeometry;
	}
	log.info("几何体合并完成");

	// 计算合并几何体的凸包
	OGRGeometry* poConvexHull = poMergedGeometry ? poMergedGeometry->ConvexHull() : nullptr;
	delete poMergedGeometry;
	log.info("凸包计算成功");

	// 如果凸包计算成功，将其保存为Shapefile
	if (poConvexHull != nullptr) {
		const char* pszDriverName = "ESRI Shapefile";
		GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(pszDriverName);
		if (poDriver == nullptr) {
			delete poConvexHull;
			GDALClose(poDS);
			log.warn("输出驱动创建失败");
			return;
		}

		GDALDataset* poOutDS = poDriver->Create(strFilePath.toStdString().c_str(), 0, 0, 0, GDT_Unknown, nullptr);
		if (poOutDS == nullptr) {
			delete poConvexHull;
			GDALClose(poDS);
			log.warn("输出数据集创建失败");
			return;
		}

		// 创建输出图层
		OGRLayer* poOutLayer = poOutDS->CreateLayer(poLayer->GetName(), nullptr, wkbPolygon, nullptr);
		if (poOutLayer == nullptr) {
			delete poConvexHull;
			GDALClose(poOutDS);
			GDALClose(poDS);
			log.warn("图层创建失败");
			return;
		}

		// 创建新要素，并设置其几何体为计算的凸包
		OGRFeature* poOutFeature = OGRFeature::CreateFeature(poOutLayer->GetLayerDefn());
		poOutFeature->SetGeometry(poConvexHull);
		poOutLayer->CreateFeature(poOutFeature);

		// 清理资源并关闭数据集
		OGRFeature::DestroyFeature(poOutFeature);
		GDALClose(poOutDS);
		log.info("Shapefile输出成功");
	}

	// 清理资源并关闭输入数据集
	delete poConvexHull;
	GDALClose(poDS);
	mpMainWindow->importVector(strFilePath); // 将生成的矢量文件导入到主窗口
}
