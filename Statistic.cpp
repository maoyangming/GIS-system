#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "Statistic.h"
#include "MyGIS.h"

// 构造函数，初始化 Statistic 对话框
// 参数：
Statistic::Statistic(MyGIS* mainWindow, QWidget* parent)
	: QDialog(parent)
{
	ui.setupUi(this); // 设置 UI 界面
	mpMainWindow = mainWindow; // 保存主窗口指针

	// 连接按钮点击信号到对应的槽函数
	connect(ui.pushButton, &QPushButton::clicked, this, &Statistic::actionYes);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &Statistic::actionNo);
}

// 析构函数，释放资源
Statistic::~Statistic()
{}

// 当用户点击确认按钮时，执行数据分析并将结果显示在表格中
void Statistic::actionYes() {
	// 创建一个新的表格小部件，用于显示分析结果
	QTableWidget* pTable = new QTableWidget(mpMainWindow->getUI().dockWidget_3);
	pTable->setColumnCount(4); // 设置表格的列数
	pTable->setHorizontalHeaderLabels(QStringList() << "要素类型" << "数量" << "面积" << "周长/长度"); // 设置表格的列标题

	// 调用 analyzeVectorData 函数，分析矢量数据并将结果填充到表格中
	analyzeVectorData(ui.comboBox->currentText(), pTable);

	// 将表格设置为 dockWidget_3 的内容，并显示该 dockWidget
	mpMainWindow->getUI().dockWidget_3->setWidget(pTable);
	mpMainWindow->getUI().dockWidget_3->show();
	this->close(); // 关闭对话框
}

// 当用户点击取消按钮时，关闭对话框
void Statistic::actionNo() {
	this->close();
}

// 向对话框中添加图层列表，供用户选择分析的矢量数据图层
// 参数：
// - strlLayers: 包含图层名称的 QStringList
void Statistic::addLayer(const QStringList& strlLayers) {
	ui.comboBox->addItems(strlLayers); // 将图层名称添加到下拉菜单中
}

// 分析指定路径下的矢量数据，并将结果显示在表格中
// 参数：
// - strFilePath: 要分析的矢量数据文件路径
// - pTable: 用于显示分析结果的 QTableWidget 指针
void Statistic::analyzeVectorData(const QString& strFilePath, QTableWidget* pTable) {
	GDALAllRegister(); // 注册所有 GDAL 驱动

	// 设置日志记录
	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpMainWindow->mpFileAppender);
	log.info("开始统计矢量要素");

	// 打开矢量数据集
	GDALDataset* poDS = static_cast<GDALDataset*>(GDALOpenEx(strFilePath.toStdString().c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
	if (poDS == nullptr) {
		log.warn("无法打开数据集");
		return;
	}
	log.info("打开数据集");

	// 获取图层
	OGRLayer* poLayer = poDS->GetLayer(0);
	if (poLayer == nullptr) {
		GDALClose(poDS);
		log.warn("无法获取到图层");
		return;
	}
	log.info("获取到图层");

	// 初始化要素统计数据
	OGRFeature* poFeature;
	QMap<QString, int> featureCounts; // 要素数量统计
	QMap<QString, double> featureAreas; // 要素面积统计
	QMap<QString, double> featurePerimeters; // 要素周长/长度统计

	// 遍历图层中的每个要素，并计算其面积和周长/长度
	poLayer->ResetReading();
	while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
		OGRGeometry* poGeometry = poFeature->GetGeometryRef();
		if (poGeometry != nullptr) {
			QString geometryType = QString::fromStdString(poGeometry->getGeometryName()); // 获取要素类型

			double area = 0.0;
			double perimeter = 0.0;

			// 计算多边形要素的面积和周长
			if (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) {
				OGRPolygon* poPolygon = static_cast<OGRPolygon*>(poGeometry);
				area = poPolygon->get_Area();
				perimeter = poPolygon->getExteriorRing()->get_Length();
			}
			// 计算多重多边形要素的面积和周长
			else if (wkbFlatten(poGeometry->getGeometryType()) == wkbMultiPolygon) {
				OGRMultiPolygon* poMultiPolygon = static_cast<OGRMultiPolygon*>(poGeometry);
				for (int i = 0; i < poMultiPolygon->getNumGeometries(); i++) {
					OGRPolygon* poPolygon = static_cast<OGRPolygon*>(poMultiPolygon->getGeometryRef(i));
					area += poPolygon->get_Area();
					perimeter += poPolygon->getExteriorRing()->get_Length();
				}
			}
			// 计算线要素的长度
			else if (wkbFlatten(poGeometry->getGeometryType()) == wkbLineString) {
				OGRLineString* poLineString = static_cast<OGRLineString*>(poGeometry);
				perimeter = poLineString->get_Length();
			}
			// 计算多重线要素的总长度
			else if (wkbFlatten(poGeometry->getGeometryType()) == wkbMultiLineString) {
				OGRMultiLineString* poMultiLineString = static_cast<OGRMultiLineString*>(poGeometry);
				for (int i = 0; i < poMultiLineString->getNumGeometries(); i++) {
					OGRLineString* poLineString = static_cast<OGRLineString*>(poMultiLineString->getGeometryRef(i));
					perimeter += poLineString->get_Length();
				}
			}

			// 更新要素统计信息
			featureCounts[geometryType] += 1;
			featureAreas[geometryType] += area;
			featurePerimeters[geometryType] += perimeter;
		}
		OGRFeature::DestroyFeature(poFeature); // 销毁要素，释放资源
	}
	log.info("要素统计完毕");
	GDALClose(poDS); // 关闭数据集

	// 将统计结果输出到表格中
	int row = 0;
	log.info("统计结果输出到表格");
	for (auto it = featureCounts.begin(); it != featureCounts.end(); ++it) {
		pTable->insertRow(row); // 插入新行
		pTable->setItem(row, 0, new QTableWidgetItem(it.key())); // 设置要素类型
		pTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value()))); // 设置要素数量
		pTable->setItem(row, 2, new QTableWidgetItem(QString::number(featureAreas[it.key()], 'f', 6))); // 设置要素面积，保留6位小数
		pTable->setItem(row, 3, new QTableWidgetItem(QString::number(featurePerimeters[it.key()], 'f', 6))); // 设置要素周长/长度，保留6位小数
		row++;
	}
}
