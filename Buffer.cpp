#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Buffer.h"
#include "MyGIS.h"

// 构造函数，初始化Buffer对话框
// 参数：
// - mainWindow: 指向主窗口（MyGIS）的指针，用于与主窗口进行交互
// - parent: 父窗口指针，默认为nullptr
Buffer::Buffer(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 设置UI界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 连接信号和槽
	connect(ui.toolButton, &QToolButton::clicked, this, &Buffer::actionOpenFile);
	connect(ui.pushButton, &QPushButton::clicked, this, &Buffer::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &Buffer::actionNo);
}

// 析构函数，释放资源
Buffer::~Buffer()
{}

// 打开文件保存对话框的槽函数，供用户选择输出Shapefile文件的位置
void Buffer::actionOpenFile() {
	QString strFilePath = QFileDialog::getSaveFileName(
		nullptr,
		"Save File", // 对话框标题
		"", // 默认路径
		"Shapefile (*.shp);;All Files (*)" // 文件过滤器
	);
	ui.lineEdit->setText(strFilePath); // 将选择的文件路径显示在行编辑框中
}

// 当用户点击“确定”按钮时，创建缓冲区并关闭对话框
void Buffer::actionYes() {
	// 调用createBuffer函数，使用用户输入的参数创建缓冲区
	createBuffer(ui.comboBox->currentText(), ui.lineEdit->text(), ui.doubleSpinBox->value() / 100);
	this->close(); // 关闭对话框
}

// 当用户点击“取消”按钮时，关闭对话框
void Buffer::actionNo() {
	this->close();
}

// 添加图层到下拉菜单中，供用户选择
// 参数：
// - strlLayers: 包含图层名称的QStringList
void Buffer::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers); // 将图层名称添加到下拉菜单中
}

// 创建缓冲区功能
// 参数：
// - strInput: 输入图层的文件路径
// - strOutput: 输出缓冲区结果的文件路径
// - dDistance: 缓冲区的距离参数
void Buffer::createBuffer(const QString& strInput, const QString& strOutput, double dDistance) {
	// 注册所有GDAL驱动
	GDALAllRegister();

	// 设置日志记录
	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("开始计算缓冲区");

	// 打开输入矢量数据集
	GDALDataset* poDS = (GDALDataset*)GDALOpenEx(strInput.toStdString().c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr);
	if (poDS == nullptr) {
		log.warn("无法打开数据集");
		return;
	}
	log.info("打开数据集");

	// 获取输入数据集中的图层
	OGRLayer* poLayer = poDS->GetLayer(0);
	if (poLayer == nullptr) {
		GDALClose(poDS);
		log.warn("无法获取到图层");
		return;
	}
	log.info("获取到图层");

	// 创建输出矢量数据集
	GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");
	if (poDriver == nullptr) {
		GDALClose(poDS);
		log.warn("输出驱动创建失败");
		return;
	}
	log.info("创建驱动");

	// 创建输出数据集，保存缓冲区结果
	GDALDataset* poODS = poDriver->Create(strOutput.toStdString().c_str(), 0, 0, 0, GDT_Unknown, nullptr);
	if (poODS == nullptr) {
		GDALClose(poDS);
		log.warn("输出数据集创建失败");
		return;
	}
	log.info("创建输出数据集");

	// 创建输出图层，类型为多边形
	OGRLayer* poOutLayer = poODS->CreateLayer(poLayer->GetName(), nullptr, wkbPolygon, nullptr);
	if (poOutLayer == nullptr) {
		GDALClose(poDS);
		GDALClose(poODS);
		log.warn("图层创建失败");
		return;
	}
	log.info("创建图层");

	// 初始化一个空的几何对象，用于存储合并后的缓冲区
	OGRGeometry* poMergedBuffer = nullptr;

	// 读取输入图层的每个要素，并计算缓冲区
	OGRFeature* poFeature = nullptr;
	poLayer->ResetReading(); // 重置图层读取，准备逐个要素读取

	while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
		OGRGeometry* poGeometry = poFeature->GetGeometryRef(); // 获取要素的几何对象
		if (poGeometry != nullptr) {
			OGRGeometry* poBuffer = poGeometry->Buffer(dDistance); // 计算缓冲区
			if (poBuffer != nullptr && wkbFlatten(poBuffer->getGeometryType()) == wkbPolygon) {
				if (poMergedBuffer == nullptr) {
					// 初始时，直接赋值缓冲区
					poMergedBuffer = poBuffer;
				}
				else {
					// 合并缓冲区几何对象
					OGRGeometry* poTemp = poMergedBuffer->Union(poBuffer);
					delete poBuffer;  // 删除临时缓冲区几何对象
					delete poMergedBuffer; // 删除之前的合并结果
					poMergedBuffer = poTemp; // 更新合并结果
				}
			}
		}
		OGRFeature::DestroyFeature(poFeature); // 销毁要素对象，释放内存
	}
	log.info("创建缓冲区图形");

	// 将合并后的缓冲区结果保存到输出图层
	if (poMergedBuffer != nullptr && wkbFlatten(poMergedBuffer->getGeometryType()) == wkbPolygon) {
		OGRFeature* poOutFeature = OGRFeature::CreateFeature(poOutLayer->GetLayerDefn()); // 创建输出要素
		poOutFeature->SetGeometry(poMergedBuffer); // 设置要素几何
		if (poOutLayer->CreateFeature(poOutFeature) != OGRERR_NONE) {
			log.warn("要素创建错误");
		}
		OGRFeature::DestroyFeature(poOutFeature); // 销毁输出要素，释放内存
	}
	log.info("Shapefile输出成功");

	// 清理资源
	if (poMergedBuffer != nullptr) {
		delete poMergedBuffer; // 删除合并后的缓冲区几何对象
	}
	GDALClose(poDS); // 关闭输入数据集
	GDALClose(poODS); // 关闭输出数据集

	// 将生成的矢量文件导入到主窗口
	mpMainWindow->importVector(strOutput);
	return;
}
