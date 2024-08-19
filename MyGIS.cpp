#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "MyGIS.h"

MyGIS::MyGIS(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    mbIsEditing = false;

    // 初始化存储
    mpTextStorage = new TextFileVectorStorage("./storage/vector_layer.txt");
    mpBinaryStorage = new BinaryFileVectorStorage("./storage/vector_layer.bin");

	// 创建附加器并设置布局
	mpFileAppender = new log4cpp::FileAppender("log", "Log.log");
	log4cpp::PatternLayout* layout = new log4cpp::PatternLayout();
	layout->setConversionPattern("%d [%p] %m%n"); // 设置日志格式，包含时间、级别、消息
	mpFileAppender->setLayout(layout);

	// 配置日志类别
	log4cpp::Category& root = log4cpp::Category::getRoot();
	root.addAppender(mpFileAppender);
	root.setPriority(log4cpp::Priority::INFO);  // 设置日志级别

    // 初始化treeView
    ui.treeView->header()->hide();
    ui.treeView->setIndentation(0);

    // 初始化地图场景
    mpGraphicsScene = new QGraphicsScene(this);
    mpGraphicsScene->setSceneRect(-50000000000, -40000000000, 100000000000, 80000000000);
    ui.graphicsView->setScene(mpGraphicsScene);
    ui.graphicsView->setEditing(false);

    // 初始化图层管理器
    mpLayerManager = new MyLayerManager(ui.treeView, 
        ui.graphicsView, 
        ui.actionStartEditing, 
        ui.actionAttributesView);

    // 初始化工具栏
    ui.actionStartEditing->setEnabled(false);
    ui.actionStopEditing->setEnabled(false);
    ui.actionAttributesView->setEnabled(false);

    // 隐藏 dock widget
    ui.dockWidget_2->hide();
    ui.dockWidget_3->hide();
    ui.dockWidget_4->hide();

    // 视窗翻转
    mTransform.scale(1, -1);
    ui.graphicsView->setTransform(mTransform);

    // 初始化激活图层指针
    mpOpenedLayer = nullptr;

	

    // 连接信号和槽
    // 视图窗口相关
    connect(ui.actionLayerManager, &QAction::triggered, this, &MyGIS::openLayerManager);
    connect(ui.actionAttributesView, &QAction::triggered, this, &MyGIS::openAttributesView);
    connect(ui.actionStatisticView, &QAction::triggered, this, &MyGIS::openStatisticView);
    connect(ui.actionHistogramView, &QAction::triggered, this, &MyGIS::openHistogramView);
    // 导入文件相关
    connect(ui.actionVectorLayer, &QAction::triggered, this, &MyGIS::loadVectorLayer);
    connect(ui.actionRasterLayer, &QAction::triggered, this, &MyGIS::loadRasterLayer);
    connect(ui.action_SBRD, &QAction::triggered, this, &MyGIS::loadLargeRaster);
    connect(ui.action_WKT, &QAction::triggered, this, &MyGIS::loadVectorLayer);
    connect(ui.action_Shapefile, &QAction::triggered, this, &MyGIS::loadVectorLayer);
    connect(ui.action_GeoJSON, &QAction::triggered, this, &MyGIS::loadVectorLayer);
    connect(ui.action_Raster, &QAction::triggered, this, &MyGIS::loadRasterLayer);
    // 视窗调整相关
    connect(ui.actionPan, &QAction::triggered, this, &MyGIS::actionPan);
    connect(ui.actionSelect, &QAction::triggered, this, &MyGIS::actionSelect);
    connect(ui.actionZoomIn, &QAction::triggered, this, &MyGIS::actionZoomIn);
    connect(ui.actionZoomOut, &QAction::triggered, this, &MyGIS::actionZoomOut);
    connect(ui.actionLeftRotate, &QAction::triggered, this, &MyGIS::actionLeftRotate);
    connect(ui.actionRightRotate, &QAction::triggered, this, &MyGIS::actionRightRotate);
    //保存相关
    connect(ui.actionStxt, &QAction::triggered, this, &MyGIS::saveTxt);
    connect(ui.actionSdata, &QAction::triggered, this, &MyGIS::saveDataBase);

    // 编辑相关
    connect(ui.treeView, &MyTreeView::clicked, this, &MyGIS::onItemClicked);
    connect(ui.actionStartEditing, &QAction::triggered, this, &MyGIS::startEditing);
    connect(ui.actionStopEditing, &QAction::triggered, this, &MyGIS::stopEditing);
    // 打开 Dialog 相关
    connect(ui.actionConvexHull, &QAction::triggered, this, &MyGIS::calConvexHull);
    connect(ui.actionBuffer, &QAction::triggered, this, &MyGIS::calBuffer);
    connect(ui.actionColorDisplay, &QAction::triggered, this, &MyGIS::TFColorDisplays);
    connect(ui.actionStatistic, &QAction::triggered, this, &MyGIS::doStatistic);
    connect(ui.actionHistogram, &QAction::triggered, this, &MyGIS::drawHistogram);
    connect(ui.actionMask, &QAction::triggered, this, &MyGIS::rasterMask);
    connect(ui.actionNeighbor, &QAction::triggered, this, &MyGIS::calNeighbor);
    // 保存工程相关
    connect(ui.actionOpenProject, &QAction::triggered, this, &MyGIS::openProject);
    connect(ui.actionSaveProject, &QAction::triggered, this, &MyGIS::saveProject);
}

MyGIS::~MyGIS()
{
    delete mpLayerManager;
    mpLayerManager = nullptr;
}

Ui::MyGISClass MyGIS::getUI() {
    return ui;
}

QGraphicsScene* MyGIS::getScene() {
    return mpGraphicsScene;
}

MyLayerManager* MyGIS::getLayerManager() {
    return mpLayerManager;
}

void MyGIS::openLayerManager() {
    if (ui.dockWidget->isHidden()) {
        ui.dockWidget->show();
    }
}

void MyGIS::saveTxt() {
	// 弹出文件保存对话框，让用户选择文件路径
	QString strFilePath = QFileDialog::getSaveFileName(this, "保存为文本文件", QDir::homePath(), "Text Files (*.txt)");

	if (strFilePath.isEmpty()) {
		// 如果用户取消了文件选择，直接返回
		return;
	}

	try {
		// 创建文本存储对象
		textStorage storage(strFilePath.toStdString());

		// 调用 storeVectorData 函数存储当前图层数据到文本文件中
		if (mpOpenedLayer) {
			storage.storeVectorData(*mpOpenedLayer);
		}
		else {
			QMessageBox::warning(this, "保存失败", "当前没有打开的图层。");
            return;
		}

		QMessageBox::information(this, "保存成功", "数据已成功保存为文本文件。");
	}
	catch (const std::exception& e) {
		QMessageBox::critical(this, "保存失败", e.what());
	}
    return;
}

void MyGIS::saveDataBase() {
	// 弹出文件保存对话框，让用户选择数据库文件路径
	QString strFilePath = QFileDialog::getSaveFileName(this, "保存为数据库文件", QDir::homePath(), "SQLite Database (*.db)");

	if (strFilePath.isEmpty()) {
		// 如果用户取消了文件选择，直接返回
		return;
	}

	try {
		// 创建数据库存储对象
		dataStorage storage(strFilePath.toStdString());

		// 存储当前打开的图层数据到数据库中
		if (mpOpenedLayer) {
			storage.storeVectorData(*mpOpenedLayer);
			QMessageBox::information(this, "保存成功", "数据已成功保存到数据库文件。");
		}
		else {
			QMessageBox::warning(this, "保存失败", "当前没有打开的图层。");
            return;
		}
	}
	catch (const std::exception& e) {
		QMessageBox::critical(this, "保存失败", e.what());
	}
    return;
}

void MyGIS::openStatisticView() {
    if (ui.dockWidget_3->isHidden()) {
        ui.dockWidget_3->show();
    }
}

void MyGIS::openHistogramView() {
    if (ui.dockWidget_4->isHidden()) {
        ui.dockWidget_4->show();
    }
}

void MyGIS::openAttributesView() {
    if (!mpOpenedLayer) {
        return;
    }

    QTableWidget* pTable = new QTableWidget(ui.dockWidget_2);

    // 设置表头
    QVector<QString> vstrFieldNames = mpOpenedLayer->getFieldNames();
    pTable->setColumnCount(vstrFieldNames.size());
    pTable->setHorizontalHeaderLabels(vstrFieldNames.toList());

    // 填充表格数据
    QVector<MyVectorLayer::AttributeRecord> records = mpOpenedLayer->getRecords();
    for (int row = 0; row < records.size(); row++)
    {
        pTable->insertRow(row);
        for (int col = 0; col < vstrFieldNames.size(); col++)
        {
            QVariant value = records[row].values[col];
            pTable->setItem(row, col, new QTableWidgetItem(value.toString()));
        }
    }

    ui.dockWidget_2->setWidget(pTable);
    if (ui.dockWidget_2->isHidden()) {
        ui.dockWidget_2->show();
    }
    ui.actionAttributesView->setEnabled(false);
}

void MyGIS::actionPan() {
    ui.graphicsView->setEditing(false);
}

void MyGIS::actionSelect() {
    ui.graphicsView->setEditing(true);
}

void MyGIS::actionZoomIn() {
    // 获取当前视图的中心点
    QPointF viewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    // 执行缩放
    ui.graphicsView->scale(1.2, 1.2);
    // 缩放后重置视图的中心点
    QPointF newCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    QPointF deltaCenter = newCenter - viewCenter;
    ui.graphicsView->translate(deltaCenter.x(), deltaCenter.y());
}

void MyGIS::actionZoomOut() {
    // 获取当前视图的中心点
    QPointF viewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    // 执行缩放
    ui.graphicsView->scale(1.0 / 1.2, 1.0 / 1.2);
    // 缩放后重置视图的中心点
    QPointF newCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    QPointF deltaCenter = newCenter - viewCenter;
    ui.graphicsView->translate(deltaCenter.x(), deltaCenter.y());
}

void MyGIS::actionLeftRotate() {
    // 获取当前视图的中心点
    QPointF viewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    // 进行旋转，以中心为原点
    qreal angle = -30;  // 旋转角度，顺时针方向为正
    ui.graphicsView->rotate(angle);

    // 旋转后保持视图中心不变
    QPointF newViewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    QPointF offset = newViewCenter - viewCenter;
    ui.graphicsView->translate(offset.x(), offset.y());
}

void MyGIS::actionRightRotate() {
    // 获取当前视图的中心点
    QPointF viewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    // 进行旋转，以中心为原点
    qreal angle = 30;  // 旋转角度
    ui.graphicsView->rotate(angle);

    // 旋转后保持视图中心不变
    QPointF newViewCenter = ui.graphicsView->mapToScene(ui.graphicsView->viewport()->rect().center());
    QPointF offset = newViewCenter - viewCenter;
    ui.graphicsView->translate(offset.x(), offset.y());
}

//计算凸包函数
void MyGIS::calConvexHull() {
    mpConvexHull = new ConvexHull(this);
    mpConvexHull->addLayer(mpLayerManager->mlVectorLayers);
    mpConvexHull->show();
}
//计算缓冲区函数
void MyGIS::calBuffer() {
    mpBuffer = new Buffer(this);
    mpBuffer->addLayer(mpLayerManager->mlVectorLayers);
    mpBuffer->show();
}
//统计分析
void MyGIS::doStatistic() {
    mpStatistic = new Statistic(this);
    mpStatistic->addLayer(mpLayerManager->mlVectorLayers);
    mpStatistic->show();
}
//真假色展示
void MyGIS::TFColorDisplays() {
    mpTFColorDisplay = new TFColorDisplay(this);
    mpTFColorDisplay->addLayer(mpLayerManager->mlRasterLayers);
    mpTFColorDisplay->show();
}
//绘制直方图
void MyGIS::drawHistogram() {
    mpHistogram = new Histogram(this);
    mpHistogram->addLayer(mpLayerManager->mlRasterLayers);
    mpHistogram->show();
}
//栅格掩膜
void MyGIS::rasterMask() {
    mpMask = new Mask(this);
    mpMask->addLayer(mpLayerManager->mlVectorLayers, mpLayerManager->mlRasterLayers);
    mpMask->show();
}
//计算最近邻
void MyGIS::calNeighbor() {
    mpNeighbor = new NeighborhoodStatistics(this);
    mpNeighbor->addLayer(mpLayerManager->mlRasterLayers);
    mpNeighbor->show();
}
//加载矢量图层
void MyGIS::loadVectorLayer() {
    // 打开文件选择对话框，只允许选择特定类型的文件
    QString strFilePath = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(), "Vector File (*.shp *.geojson *.csv)");

    if (!strFilePath.isEmpty()) {
        importVector(strFilePath);
    }
}
//加载栅格图层
void MyGIS::loadRasterLayer() {
    // 打开文件选择对话框，只允许选择特定类型的文件
    QString strFilePath = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(), "Raster File (*.tif *.img *.png *.jpg)");

    if (!strFilePath.isEmpty()) {
        importRaster(strFilePath);
    }
}

void MyGIS::loadLargeRaster() {
    // 打开文件选择对话框，只允许选择特定类型的文件
    QString strFilePath = QFileDialog::getOpenFileName(this, "选择文件", QDir::homePath(), "Raster File (*.tif *.img *.png *.jpg *.tiff)");

    if (!strFilePath.isEmpty()) {
        importLargeRaster(strFilePath);
    }
}
//选中图层事件
void MyGIS::onItemClicked(const QModelIndex& index) {
   //确保只有一个图层在编辑
    if (mbIsEditing == false) {
        if (!index.isValid()) {
            return;
        }

        QStandardItemModel* pModel = static_cast<QStandardItemModel*>(ui.treeView->model());
        MyItem* pOpenedItem = static_cast<MyItem*>(pModel->itemFromIndex(index));
        //查找指向的图层
        QList<QStandardItem*> lpItems = pModel->findItems(pOpenedItem->text(), Qt::MatchExactly);
        if (lpItems.isEmpty()) {
            mpOpenedLayer = nullptr;
        }
        else {
            //更新控件状态，更新指针信息
            ui.actionStartEditing->setEnabled(true);
            ui.actionAttributesView->setEnabled(true);
            mpOpenedLayer = pOpenedItem->getVectorLayer();
        }
    }
    else {
        return;
    }
}

void MyGIS::startEditing() {
    if (!mpOpenedLayer) {
        return;
    }

    log4cpp::Category& log = log4cpp::Category::getRoot();
    log.addAppender(mpFileAppender);
    log.info("开始编辑");
    //更新控件状态
    mpOpenedLayer->setEditing(true);
    ui.actionStartEditing->setEnabled(false);
    ui.actionStopEditing->setEnabled(true);
    ui.graphicsView->setEditing(true);
    mbIsEditing = true;     

    for (QGraphicsItem* item : mpOpenedLayer->getGraphicsItem()) {
        if (QGraphicsEllipseItem* ellipse = dynamic_cast<QGraphicsEllipseItem*>(item)) {
            // 将 QGraphicsEllipseItem 替换为 MyPointItem
            QPointF center = ellipse->rect().center();
            MyPointItem* editablePoint = new MyPointItem(center);
            editablePoint->setBrush(ellipse->brush());    // 设置点的填充颜色
            editablePoint->setPen(QPen(Qt::blue, 0.0004)); // 设置点的边框
            editablePoint->setPos(ellipse->pos());        // 保留位置
            mpGraphicsScene->removeItem(ellipse);
            mpGraphicsScene->addItem(editablePoint);
            mlpEditingPoint.append(editablePoint);
        }
        else if (QGraphicsPathItem* line = dynamic_cast<QGraphicsPathItem*>(item)) {
            // 将 QGraphicsLineItem 替换为 MyPathItem
            QPainterPath path = line->path();
            MyPathItem* editablePath = new MyPathItem(path);
            editablePath->setPen(QPen(Qt::blue, 0.0004));  // 设置路径的颜色和宽度
            editablePath->setPos(line->pos());            // 保留位置
            mpGraphicsScene->removeItem(line);
            mpGraphicsScene->addItem(editablePath);
            mlpEditingPath.append(editablePath);
        }
        else if (QGraphicsPolygonItem* polygon = dynamic_cast<QGraphicsPolygonItem*>(item)) {
            // 将 QGraphicsPolygonItem 替换为 MyPolygonItem
            QPolygonF polygonF = polygon->polygon();
            MyPolygonItem* editablePolygon = new MyPolygonItem(polygonF);
            editablePolygon->setPen(QPen(Qt::blue, 0.0004));  // 设置多边形的边框颜色和宽度
            editablePolygon->setBrush(polygon->brush());     // 设置多边形的填充颜色
            editablePolygon->setPos(polygon->pos());         // 保留位置
            mpGraphicsScene->removeItem(polygon);
            mpGraphicsScene->addItem(editablePolygon);
            mlpEditingPolygon.append(editablePolygon);
        }
    }
    mlpEditedItem.clear();
}

void MyGIS::stopEditing() {
	if (!mpOpenedLayer) {
		return;
	}

	mpOpenedLayer->setEditing(false);
	ui.actionStopEditing->setEnabled(false);
	ui.graphicsView->setEditing(false);

	// 清空当前图层的要素
	mpOpenedLayer->clearLayerFeatures();

	// 保存编辑的点
	for (MyPointItem* editingPoint : mlpEditingPoint) {
		QRectF center = editingPoint->rect();
		QPointF pos = editingPoint->pos();
		double x = pos.x();
		double y = pos.y();

		// 添加点到当前图层
		mpOpenedLayer->addPointFeature(x, y);

		// 替换为 QGraphicsEllipseItem 并添加到场景
		QGraphicsEllipseItem* editedPoint = new QGraphicsEllipseItem(center);
		editedPoint->setBrush(editingPoint->brush()); // 保留刷
		editedPoint->setPen(QPen(Qt::black, 0.0004));
		editedPoint->setPos(pos); // 保留位置
		mpGraphicsScene->removeItem(editingPoint);
		mpGraphicsScene->addItem(editedPoint);
		mlpEditedItem.append(editedPoint);
	}

	// 保存编辑的路径
	for (MyPathItem* editingPath : mlpEditingPath) {
		QPainterPath path = editingPath->path();
		// 创建一个QGraphicsPathItem，将构建好的完整路径设置进去
		QGraphicsPathItem* editedPath = new QGraphicsPathItem(path);
		editedPath->setPen(QPen(Qt::black, 0.0004));
		editedPath->setPos(editingPath->pos());     // 保留位置
		// 添加折线到当前图层
		mpOpenedLayer->addLineFeature(editedPath);

		mpGraphicsScene->removeItem(editingPath);
		mpGraphicsScene->addItem(editedPath);
		mlpEditedItem.append(editedPath);
	}

	// 保存编辑的多边形
	for (MyPolygonItem* editingPolygon : mlpEditingPolygon) {
		QPolygonF polygonF = editingPolygon->polygon();
		QVector<QPointF> points = polygonF.toList().toVector();
		// 替换为 QGraphicsPolygonItem 并添加到场景
		QGraphicsPolygonItem* editedPolygon = new QGraphicsPolygonItem(polygonF);
		editedPolygon->setBrush(editingPolygon->brush()); // 保留刷
		editedPolygon->setPen(QPen(Qt::black, 0.0004));
		editedPolygon->setPos(editingPolygon->pos()); // 保留位置
		// 添加多边形到当前图层
		mpOpenedLayer->addPolygonFeature(editedPolygon);

		mpGraphicsScene->removeItem(editingPolygon);
		mpGraphicsScene->addItem(editedPolygon);
		mlpEditedItem.append(editedPolygon);
	}

	// 清空编辑列表
	mlpEditingPath.clear();
	mlpEditingPoint.clear();
	mlpEditingPolygon.clear();

	// 更新图层中的图形项
	mpOpenedLayer->setGraphicsItem(mlpEditedItem);

	// 保存图层数据到 Shapefile
	saveLayerToShapefile(mpOpenedLayer);

	log4cpp::Category& log = log4cpp::Category::getRoot();
	log.addAppender(mpFileAppender);
	log.info("停止编辑");


	mpGraphicsScene->update(); // 更新场景

	ui.actionStartEditing->setEnabled(true); // 启用编辑按钮
	ui.actionStopEditing->setEnabled(false); // 禁用停止编辑按钮
    mbIsEditing = false;
}
//将信息存储到shapefile文件中
void MyGIS::saveLayerToShapefile(MyVectorLayer* layer) {
	// 打开原始的 Shapefile 文件
	GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpenEx(layer->getFilePath().toStdString().c_str(), GDAL_OF_VECTOR | GDAL_OF_UPDATE, nullptr, nullptr, nullptr));
	if (!poDataset) {
		return;
	}

	OGRLayer* poLayer = poDataset->GetLayer(0);
	if (!poLayer) {
		GDALClose(poDataset);
		return;
	}

	// 清除当前 Shapefile 中的所有要素
	poLayer->DeleteFeature(0);

	// 逐个添加新要素，重写了myvectorlayer的函数，根据不同的要素类型，先将图层清空，然后重新绘制信息。
	for (QGraphicsItem* item : layer->getGraphicsItem()) {
		OGRFeature* poFeature = OGRFeature::CreateFeature(poLayer->GetLayerDefn());

		if (QGraphicsEllipseItem* pointItem = dynamic_cast<QGraphicsEllipseItem*>(item)) {    //点要素
			QPointF pos = pointItem->pos();      //读取点的坐标
			OGRPoint point(pos.x(), pos.y());     //添加点的信息
			poFeature->SetGeometry(&point);
		}
		else if (QGraphicsLineItem* lineItem = dynamic_cast<QGraphicsLineItem*>(item)) {    //线要素
			//先读取线信息，再在图层中添加线的信息
            QLineF line = lineItem->line();      
			OGRLineString lineString;
			lineString.addPoint(line.p1().x(), line.p1().y());
			lineString.addPoint(line.p2().x(), line.p2().y());
			poFeature->SetGeometry(&lineString);
		}
		else if (QGraphicsPolygonItem* polygonItem = dynamic_cast<QGraphicsPolygonItem*>(item)) {  //面要素
            //先读取面的信息，再在图层中添加面的信息
			QPolygonF polygon = polygonItem->polygon();
			OGRPolygon ogrPolygon;
			OGRLinearRing ring;
			for (const QPointF& point : polygon) {
				ring.addPoint(point.x(), point.y());
			}
			ring.closeRings();
			ogrPolygon.addRing(&ring);
			poFeature->SetGeometry(&ogrPolygon);
		}

		// 将新要素添加到 Shapefile
		if (poLayer->CreateFeature(poFeature) != OGRERR_NONE) {
		}

		OGRFeature::DestroyFeature(poFeature);
	}

	GDALClose(poDataset);
}



/*<------------------------------------------------------------------------------------------------------------->*/

void MyGIS::importVector(const QString& strFilePath) {
    // 初始化GDAL
    GDALAllRegister();
    
    log4cpp::Category& log = log4cpp::Category::getRoot();
    log.addAppender(mpFileAppender);

    // 打开矢量数据集
    GDALDataset* pDataset = static_cast<GDALDataset*>(GDALOpenEx(strFilePath.toStdString().c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr));
    if (!pDataset) {
        log.warn("无法打开数据集");
        return;
    }

    OGRLayer* poLayer = pDataset->GetLayer(0);
    if (!poLayer) {
        log.warn("无法获取到图层");
        return;
    }

    // 获取字段名称
    QVector<QString> vstrFieldNames;
    int nFieldCount = poLayer->GetLayerDefn()->GetFieldCount();
    for (int i = 0; i < nFieldCount; i++)
    {
        OGRFieldDefn* poFieldDefn = poLayer->GetLayerDefn()->GetFieldDefn(i);
        vstrFieldNames.append(QString(poFieldDefn->GetNameRef()));
    }

    // 获取文件名
    QFileInfo fileInfo(strFilePath);
    QString strFileName = fileInfo.fileName();

    // 创建图形列表
    QList<QGraphicsItem*> vpGraphicsItems;
    
    // 用于获取矢量数据的类型
    GeometryType type = GeometryType::Unknown;
    
    // 创建矢量图层
    MyVectorLayer* pNewLayer = new MyVectorLayer(strFilePath, strFileName, type);
    log.info("创建图层成功");

    // 目标地理坐标系（WGS84）
    OGRSpatialReference oTargetSRS;
    oTargetSRS.SetWellKnownGeogCS("WGS84");

    // 获取原始坐标系
    OGRSpatialReference* poSrcSRS = poLayer->GetSpatialRef();
    OGRSpatialReference ownSrcSRS;
    if (poSrcSRS == nullptr) {
        poSrcSRS = &ownSrcSRS;
        poSrcSRS->SetWellKnownGeogCS("WGS84");
    }

    // 创建坐标转换器
    OGRCoordinateTransformation* poCT = OGRCreateCoordinateTransformation(poSrcSRS, &oTargetSRS);
    if (poCT == nullptr) {
        GDALClose(pDataset);
        return;
    }

    OGRFeature* poFeature = nullptr;
    poLayer->ResetReading();

    QRectF boundingRect;

    while ((poFeature = poLayer->GetNextFeature()) != nullptr) {
        MyVectorLayer::AttributeRecord record;
        // 获取每条记录的属性值
        for (int i = 0; i < nFieldCount; i++)
        {
            QVariant value(QString(poFeature->GetFieldAsString(i)));
            record.values.append(value);
        }

        // 向属性表中添加记录
        pNewLayer->addRecord(record);
        OGRGeometry* geometry = poFeature->GetGeometryRef();
        if (!geometry) {
            log.warn("无法获取到几何");
            OGRFeature::DestroyFeature(poFeature);
            continue;
        }

        // 转换几何图形到目标SRS
        if (poCT != nullptr) {
            geometry->transform(poCT);
        }
        //根据不同类型的元素类型来进行不同元素的操作
        OGRwkbGeometryType geomType = wkbFlatten(geometry->getGeometryType());
        switch (geomType) {
        case wkbPolygon: { 
            QList<QGraphicsItem*> vpPartOfItems = processPolygon(static_cast<OGRPolygon*>(geometry), mpGraphicsScene);
            vpGraphicsItems.append(vpPartOfItems);
            type = GeometryType::Polygon;
            break;
        }
        case wkbMultiPolygon: {
            QList<QGraphicsItem*> vpPartOfItems = processMultiPolygon(static_cast<OGRMultiPolygon*>(geometry), mpGraphicsScene);
            vpGraphicsItems.append(vpPartOfItems);
            type = GeometryType::Polygon;
            break;
        }
        case wkbLineString: {
            QList<QGraphicsItem*> vpPartOfItems = processLineString(static_cast<OGRLineString*>(geometry), mpGraphicsScene);
            vpGraphicsItems.append(vpPartOfItems);
            type = GeometryType::Line;
            break;
        }
        case wkbMultiLineString: {
            QList<QGraphicsItem*> vpPartOfItems = processMultiLineString(static_cast<OGRMultiLineString*>(geometry), mpGraphicsScene);
            vpGraphicsItems.append(vpPartOfItems);
            type = GeometryType::Line;
            break;
        }
        case wkbPoint: {
            QGraphicsItem* pPartOfItem = processPoint(static_cast<OGRPoint*>(geometry), mpGraphicsScene);
            vpGraphicsItems.push_back(pPartOfItem);
            type = GeometryType::Point;
            break;
        }
        case wkbMultiPoint: {
            QList<QGraphicsItem*> vpPartOfItems = processMultiPoint(static_cast<OGRMultiPoint*>(geometry), mpGraphicsScene);
            vpGraphicsItems.append(vpPartOfItems);
            type = GeometryType::Point;
            break;
        }
        default:
            break;
        }

        // 更新边界矩形
        for (const auto& item : vpGraphicsItems) {
            boundingRect = boundingRect.isNull() ? item->boundingRect() : boundingRect.united(item->boundingRect());
        }
        OGRFeature::DestroyFeature(poFeature);
    }
    log.info("几何创建成功");

    // 设置图层
    pNewLayer->setGraphicsItem(vpGraphicsItems);
    pNewLayer->setFieldNames(vstrFieldNames);
    pNewLayer->setType(type);
    mpLayerManager->addVectorLayer(pNewLayer);
    mpLayerManager->mlVectorLayers.append(strFilePath);
    mpTextStorage->storeVectorData(*pNewLayer);
    mpBinaryStorage->storeVectorData(*pNewLayer);

    // 调整视窗
    if (!boundingRect.isNull()) {
        ui.graphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
    }

    // 关闭Shapefile数据集
    GDALClose(pDataset);

    // 释放坐标转换器
    if (poCT != nullptr) {
        OCTDestroyCoordinateTransformation(poCT);
    }
    return;
}




/*<------------------------------------------------------------------------------------------------------------->*/
//打开栅格图层
void MyGIS::importRaster(const QString& strFilePath) {
    // 初始化GDAL
    GDALAllRegister();
    
    log4cpp::Category& log = log4cpp::Category::getRoot();
    log.addAppender(mpFileAppender);

    // 打开栅格文件
    GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFilePath.toStdString().c_str(), GA_ReadOnly));
    if (!poDataset) {
        log.warn("无法打开栅格数据集");
        return;
    }
    else {
        log.info("打开栅格数据集");
    }

    if (poDataset->GetRasterCount() < 3) {
        log.info("目标栅格的波段数小于3");
    }

    // 使用 QFileInfo 获取文件名
    QFileInfo fileInfo(strFilePath);
    QString strFileName = fileInfo.fileName();

    // 获取栅格大小
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();

    GDALRasterBand* poRedBand = poDataset->GetRasterBand(1);
    GDALRasterBand* poGreenBand = poDataset->GetRasterBand(2);
    GDALRasterBand* poBlueBand = poDataset->GetRasterBand(3);

    //检查数据是否有效
    if (!poRedBand || !poGreenBand || !poBlueBand) {
        log.warn("获取某一波段错误");
        log.info("尝试只获取一个波段");

        QVector<uint8_t> vRedData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));

        poRedBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vRedData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

        QImage* pImage = new QImage(nXSize, nYSize, QImage::Format_Grayscale8);
        // 使用 OpenMP 并行化像素处理
#pragma omp parallel for
        for (int y = 0; y < nYSize; ++y) {
            for (int x = 0; x < nXSize; ++x) {
                int value = vRedData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)];
                pImage->setPixel(x, y, qRgb(value, value, value));
            }
        }
        log.info("Image设置成功");

        // 获取仿射变换参数
        double adfGeoTransform[6];
        if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
            double originX = adfGeoTransform[0];  // 左上角X
            double originY = adfGeoTransform[3];  // 左上角Y
            double pixelWidth = adfGeoTransform[1];  // 每个像素的宽度
            double pixelHeight = adfGeoTransform[5];  // 每个像素的高度，通常为负值

            QGraphicsPixmapItem* pPixmapItem = mpGraphicsScene->addPixmap(QPixmap::fromImage(*pImage));

            // 设定图片的位置和缩放比例
            pPixmapItem->setPos(originX, originY);
            pPixmapItem->setScale(pixelWidth);
            pPixmapItem->setTransform(mTransform);
            //读取栅格图层的基本信息
            MyRasterLayer* pNewLayer = new MyRasterLayer(strFilePath, strFileName, nXSize, nYSize);
            log.info("图层创建成功");
            pNewLayer->setRasterData(pPixmapItem);
            pNewLayer->setOriginX(originX);
            pNewLayer->setOriginY(originY);
            pNewLayer->setPixelWidth(pixelWidth);
            pNewLayer->setPixelHeight(pixelHeight);
            mpLayerManager->addRasterLayer(pNewLayer);

            // 获取地理范围
            QRectF boundingRect(QPointF(originX, originY + nYSize * pixelHeight),
                QSizeF(nXSize * pixelWidth, nYSize * std::abs(pixelHeight)));
            // 视窗调整
            ui.graphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
        }
        else {
            log.warn("获取仿射变换参数失败");
        }

        GDALClose(poDataset);
        return;
    }
    else
    {
        log.info("获取波段");

        QVector<uint8_t> vRedData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));
        QVector<uint8_t> vGreenData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));
        QVector<uint8_t> vBlueData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));

        poRedBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vRedData.data(), nXSize, nYSize, GDT_Byte, 0, 0);
        poGreenBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vGreenData.data(), nXSize, nYSize, GDT_Byte, 0, 0);
        poBlueBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vBlueData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

        QImage* pImage = new QImage(nXSize, nYSize, QImage::Format_RGB32);
        // 使用 OpenMP 并行化像素处理
#pragma omp parallel for
        for (int y = 0; y < nYSize; ++y) {
            for (int x = 0; x < nXSize; ++x) {
                // 在使用索引时进行强制类型转换
                int r = vRedData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)];
                int g = vGreenData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)];
                int b = vBlueData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)];
                pImage->setPixel(x, y, qRgb(r, g, b));
            }
        }
        log.info("Image设置成功");

        // 获取仿射变换参数
        double adfGeoTransform[6];
        if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
            double originX = adfGeoTransform[0];  // 左上角X
            double originY = adfGeoTransform[3];  // 左上角Y
            double pixelWidth = adfGeoTransform[1];  // 每个像素的宽度
            double pixelHeight = adfGeoTransform[5];  // 每个像素的高度，通常为负值

            QGraphicsPixmapItem* pPixmapItem = mpGraphicsScene->addPixmap(QPixmap::fromImage(*pImage));

            // 设定图片的位置和缩放比例
            pPixmapItem->setPos(originX, originY);
            pPixmapItem->setScale(pixelWidth);
            pPixmapItem->setTransform(mTransform);

            MyRasterLayer* pNewLayer = new MyRasterLayer(strFilePath, strFileName, nXSize, nYSize);
            log.info("图层创建成功");
            pNewLayer->setRasterData(pPixmapItem);
            pNewLayer->setOriginX(originX);
            pNewLayer->setOriginY(originY);
            pNewLayer->setPixelWidth(pixelWidth);
            pNewLayer->setPixelHeight(pixelHeight);
            mpLayerManager->addRasterLayer(pNewLayer);
            mpLayerManager->mlRasterLayers.append(strFilePath);

            // 获取地理范围
            QRectF boundingRect(QPointF(originX, originY + nYSize * pixelHeight),
                QSizeF(nXSize * pixelWidth, nYSize * std::abs(pixelHeight)));
            // 视窗调整
            ui.graphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
        }
        else {
            log.warn("获取仿射变换参数失败");
        }

        GDALClose(poDataset);
        return;
    }
}




/*<------------------------------------------------------------------------------------------------------------->*/

void MyGIS::importLargeRaster(const QString& strFilePath) {
    // 初始化GDAL
    log4cpp::Category& log = log4cpp::Category::getRoot();
    log.addAppender(mpFileAppender);
    GDALAllRegister();

    // 打开栅格文件
    GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(strFilePath.toStdString().c_str(), GA_ReadOnly));
    if (!poDataset) {
        log.warn("Failed to open raster file.");
        return;
    }

    int nBands = poDataset->GetRasterCount();
    if (nBands == 0) {
        GDALClose(poDataset);
        log.warn("No raster bands found.");
        return;
    }

    // 使用 QFileInfo 获取文件名
    QFileInfo fileInfo(strFilePath);
    QString strFileName = fileInfo.fileName();

    // 获取栅格的宽和高
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();

    // 为每个波段创建数据存储空间
    QVector<uint8_t> vRedData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));
    QVector<uint8_t> vGreenData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));
    QVector<uint8_t> vBlueData(static_cast<size_t>(nXSize) * static_cast<size_t>(nYSize));

    // 读取每个波段的数据
    GDALRasterBand* poRedBand = poDataset->GetRasterBand(1);
    poRedBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vRedData.data(), nXSize, nYSize, GDT_Byte, 0, 0);

    if (nBands > 1) {
        GDALRasterBand* poGreenBand = poDataset->GetRasterBand(2);
        poGreenBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vGreenData.data(), nXSize, nYSize, GDT_Byte, 0, 0);
    }
    if (nBands > 2) {
        GDALRasterBand* poBlueBand = poDataset->GetRasterBand(3);
        poBlueBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, vBlueData.data(), nXSize, nYSize, GDT_Byte, 0, 0);
    }

    // 创建QImage对象
    QImage* pImage = new QImage(nXSize, nYSize, QImage::Format_RGB32);

    // 使用 OpenMP 并行化像素处理
    #pragma omp parallel for
    for (int y = 0; y < nYSize; ++y) {
        for (int x = 0; x < nXSize; ++x) {
            int r = vRedData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)];
            int g = (nBands > 1) ? vGreenData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)] : r;
            int b = (nBands > 2) ? vBlueData[static_cast<size_t>(y) * static_cast<size_t>(nXSize) + static_cast<size_t>(x)] : r;
            pImage->setPixel(x, y, qRgb(r, g, b));
        }
    }
    log.info("Image设置成功");

    // 获取仿射变换参数
    double adfGeoTransform[6];
    if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
        double originX = adfGeoTransform[0];  // 左上角X
        double originY = adfGeoTransform[3];  // 左上角Y
        double pixelWidth = adfGeoTransform[1];  // 每个像素的宽度
        double pixelHeight = adfGeoTransform[5];  // 每个像素的高度，通常为负值

        QGraphicsPixmapItem* pPixmapItem = mpGraphicsScene->addPixmap(QPixmap::fromImage(*pImage));

        // 设定图片的位置和缩放比例
        pPixmapItem->setPos(originX, originY);
        pPixmapItem->setScale(pixelWidth);
        pPixmapItem->setTransform(mTransform);

        // 创建MyRasterLayer对象并添加到图层管理器
        MyRasterLayer* pNewLayer = new MyRasterLayer(strFilePath, strFileName, nXSize, nYSize);
        log.info("图层创建成功");
        pNewLayer->setRasterData(pPixmapItem);
        pNewLayer->setOriginX(originX);
        pNewLayer->setOriginY(originY);
        pNewLayer->setPixelWidth(pixelWidth);
        pNewLayer->setPixelHeight(pixelHeight);
        mpLayerManager->addRasterLayer(pNewLayer);
        mpLayerManager->mlRasterLayers.append(strFilePath);

        // 获取地理范围或使用默认范围
        QRectF boundingRect(QPointF(originX, originY + nYSize * pixelHeight),
            QSizeF(nXSize * pixelWidth, nYSize * std::abs(pixelHeight)));
        // 视窗调整
        ui.graphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
    } else {
        log.warn("获取仿射变换参数失败");
    }

    GDALClose(poDataset);
    return;
}






/*<--------------------------------------------------------------------------------------------------------------->*/

// 处理单个多边形并将其转换为QGraphicsItem对象列表
// polygon: OGRPolygon对象，表示一个多边形
// scene: QGraphicsScene对象，将多边形绘制到该场景中
// 返回值: 包含多边形及其内部环的QGraphicsItem对象列表
QList<QGraphicsItem*> MyGIS::processPolygon(OGRPolygon* polygon, QGraphicsScene* scene) {
	QList<QGraphicsItem*> polygonItems; // 存储所有绘制的多边形项
	QPolygonF qPolygon; // 存储外部环的点以绘制外部多边形

	// 获取外部环（边界）并将其转换为QPolygonF格式
	OGRLinearRing* exteriorRing = polygon->getExteriorRing();
	if (exteriorRing) {
		int numPoints = exteriorRing->getNumPoints(); // 获取外部环点的数量
		for (int i = 0; i < numPoints; i++) {
			double x = exteriorRing->getX(i);
			double y = exteriorRing->getY(i);
			qPolygon << QPointF(x, y); // 添加点到QPolygonF对象中
		}
	}

	// 设置绘制属性，如边框颜色、宽度和填充颜色
	QPen pen(Qt::black);
	pen.setWidth(0.2);
	QBrush brush(Qt::lightGray);
	QGraphicsItem* polygonItem = scene->addPolygon(qPolygon, pen, brush); // 绘制多边形并添加到场景
	polygonItems.push_back(polygonItem); // 将多边形项添加到列表中

	// 处理所有内部环（如果有），并将其转换为QGraphicsItem对象
	for (int i = 0; i < polygon->getNumInteriorRings(); i++) {
		OGRLinearRing* interiorRing = polygon->getInteriorRing(i);
		QPolygonF qInteriorPolygon;
		int numPoints = interiorRing->getNumPoints(); // 获取内部环点的数量
		for (int j = 0; j < numPoints; j++) {
			double x = interiorRing->getX(j);
			double y = interiorRing->getY(j);
			qInteriorPolygon << QPointF(x, y); // 添加点到内部多边形中
		}
		QGraphicsItem* interiorPolygonItem = scene->addPolygon(qInteriorPolygon, pen, brush); // 绘制内部多边形
		polygonItems.append(interiorPolygonItem); // 将内部多边形项添加到列表中
	}

	return polygonItems; // 返回多边形项列表
}

// 处理多多边形（MultiPolygon）并将其转换为QGraphicsItem对象列表
// multiPolygon: OGRMultiPolygon对象，表示多个多边形的集合
// scene: QGraphicsScene对象，将多多边形绘制到该场景中
// 返回值: 包含所有多边形的QGraphicsItem对象列表
QList<QGraphicsItem*> MyGIS::processMultiPolygon(OGRMultiPolygon* multiPolygon, QGraphicsScene* scene) {
	QList<QGraphicsItem*> multiPolygonItems; // 存储所有绘制的多多边形项
	for (int i = 0; i < multiPolygon->getNumGeometries(); i++) {
		OGRPolygon* polygon = static_cast<OGRPolygon*>(multiPolygon->getGeometryRef(i)); // 获取每个单独的多边形
		QList<QGraphicsItem*> polygonItems = processPolygon(polygon, scene); // 处理并绘制单个多边形
		multiPolygonItems.append(polygonItems); // 将单个多边形项添加到多多边形列表中
	}
	return multiPolygonItems; // 返回多多边形项列表
}

// 处理单条线段（LineString）并将其转换为QGraphicsItem对象列表
// lineString: OGRLineString对象，表示一条线
// scene: QGraphicsScene对象，将线段绘制到该场景中
// 返回值: 包含线段的QGraphicsItem对象列表
QList<QGraphicsItem*> MyGIS::processLineString(OGRLineString* lineString, QGraphicsScene* scene) {
	QList<QGraphicsItem*> pathItems; // 存储所有绘制的线段项
	QPainterPath path; // 存储线段路径
	int numPoints = lineString->getNumPoints(); // 获取线段点的数量
	if (numPoints > 0) {
		path.moveTo(lineString->getX(0), lineString->getY(0)); // 移动到第一个点
		for (int i = 1; i < numPoints; i++) {
			path.lineTo(lineString->getX(i), lineString->getY(i)); // 连接所有点形成线段
		}
	}

	// 设置绘制属性，如线段颜色和宽度
	QPen pen(Qt::black);
	pen.setWidth(0.2);
	QGraphicsPathItem* pathItem = scene->addPath(path, pen); // 绘制线段并添加到场景
	pathItems.push_back(pathItem); // 将线段项添加到列表中

	return pathItems; // 返回线段项列表
}

// 处理多线段（MultiLineString）并将其转换为QGraphicsItem对象列表
// multiLineString: OGRMultiLineString对象，表示多个线段的集合
// scene: QGraphicsScene对象，将多线段绘制到该场景中
// 返回值: 包含所有线段的QGraphicsItem对象列表
QList<QGraphicsItem*> MyGIS::processMultiLineString(OGRMultiLineString* multiLineString, QGraphicsScene* scene) {
	QList<QGraphicsItem*> multiPathItems; // 存储所有绘制的多线段项
	for (int i = 0; i < multiLineString->getNumGeometries(); i++) {
		OGRLineString* lineString = static_cast<OGRLineString*>(multiLineString->getGeometryRef(i)); // 获取每个单独的线段
		QList<QGraphicsItem*> pathItems = processLineString(lineString, scene); // 处理并绘制单个线段
		multiPathItems.append(pathItems); // 将单个线段项添加到多线段列表中
	}
	return multiPathItems; // 返回多线段项列表
}

// 处理单个点并将其转换为QGraphicsItem对象
// point: OGRPoint对象，表示一个点
// scene: QGraphicsScene对象，将点绘制到该场景中
// 返回值: 绘制点的QGraphicsItem对象
QGraphicsItem* MyGIS::processPoint(OGRPoint* point, QGraphicsScene* scene) {
	// 设置绘制属性，如点的颜色、边框宽度和填充颜色
	QPen pen(Qt::black);
	pen.setWidth(0.2);
	QBrush brush(Qt::lightGray);
	double x = point->getX(); // 获取点的X坐标
	double y = point->getY(); // 获取点的Y坐标
	// 绘制一个小圆来表示点，并将其添加到场景中
	return scene->addEllipse(x - 0.001, y - 0.001, 0.002, 0.002, pen, brush);
}

// 处理多个点（MultiPoint）并将其转换为QGraphicsItem对象列表
// multiPoint: OGRMultiPoint对象，表示多个点的集合
// scene: QGraphicsScene对象，将多个点绘制到该场景中
// 返回值: 包含所有点的QGraphicsItem对象列表
QList<QGraphicsItem*> MyGIS::processMultiPoint(OGRMultiPoint* multiPoint, QGraphicsScene* scene) {
	QList<QGraphicsItem*> multiPointItems; // 存储所有绘制的多点项
	for (int i = 0; i < multiPoint->getNumGeometries(); i++) {
		OGRPoint* point = static_cast<OGRPoint*>(multiPoint->getGeometryRef(i)); // 获取每个单独的点
		QGraphicsItem* multiPointItem = processPoint(point, scene); // 处理并绘制单个点
		multiPointItems.push_back(multiPointItem); // 将单个点项添加到多点列表中
	}
	return multiPointItems; // 返回多点项列表
}





/*<--------------------------------------------------------------------------------------------------------------->*/

// 打开一个GIS项目文件，并加载矢量和栅格图层，同时恢复视图设置
void MyGIS::openProject() {
	log4cpp::Category& log = log4cpp::Category::getRoot(); // 获取日志记录器的根类别
	log.addAppender(mpFileAppender); // 添加文件附加器，将日志输出到文件

	// 打开文件对话框，让用户选择项目文件
	QString strFilePath = QFileDialog::getOpenFileName(this, "Open Project", "", "Project Files (*.gproj)");
	if (strFilePath.isEmpty()) { // 如果用户未选择文件，则退出
		return;
	}

	QFile file(strFilePath); // 创建一个QFile对象来处理所选文件
	if (!file.open(QIODevice::ReadOnly)) { // 以只读模式打开文件
		QMessageBox::critical(this, "Error", "Failed to open project file."); // 弹出错误消息框
		log.warn("无法打开工程文件"); // 记录警告日志
		return;
	}

	QByteArray data = file.readAll(); // 读取文件中的所有数据
	QJsonDocument doc(QJsonDocument::fromJson(data)); // 将数据解析为QJsonDocument对象
	QJsonObject project = doc.object(); // 获取项目的JSON对象

	// 加载矢量图层
	QJsonArray vecLayersArray = project["vecLayers"].toArray(); // 获取包含矢量图层信息的数组
	for (int i = 0; i < vecLayersArray.size(); ++i) {
		QJsonObject layerObj = vecLayersArray[i].toObject(); // 获取数组中的每个对象
		QString fileName = layerObj["vecFileName"].toString(); // 提取矢量图层文件名
		importVector(fileName); // 调用函数导入矢量图层
	}
	log.info("矢量图层打开成功"); // 记录信息日志

	// 加载栅格图层
	QJsonArray rasLayersArray = project["rasLayers"].toArray(); // 获取包含栅格图层信息的数组
	for (int i = 0; i < rasLayersArray.size(); ++i) {
		QJsonObject layerObj = rasLayersArray[i].toObject(); // 获取数组中的每个对象
		QString fileName = layerObj["rasFileName"].toString(); // 提取栅格图层文件名
		importRaster(fileName); // 调用函数导入栅格图层
	}
	log.info("栅格图层打开成功"); // 记录信息日志

	// 恢复视图设置
	QJsonObject viewSettings = project["viewSettings"].toObject(); // 获取视图设置对象
	double scale = viewSettings["scale"].toDouble(1.0); // 获取保存的缩放比例，默认为1.0
	ui.graphicsView->resetTransform(); // 重置视图的变换矩阵
	ui.graphicsView->setTransform(mTransform); // 设置视图的变换矩阵
	ui.graphicsView->scale(scale, scale); // 根据保存的比例缩放视图
	log.info("视图恢复成功"); // 记录信息日志

	file.close(); // 关闭项目文件
}

// 保存当前的GIS项目，包括矢量和栅格图层信息以及视图设置
void MyGIS::saveProject() {
	log4cpp::Category& log = log4cpp::Category::getRoot(); // 获取日志记录器的根类别
	log.addAppender(mpFileAppender); // 添加文件附加器，将日志输出到文件

	// 打开文件对话框，让用户选择保存项目文件的位置
	QString filePath = QFileDialog::getSaveFileName(this, "Save Project", "", "Project Files (*.gproj)");
	if (filePath.isEmpty()) { // 如果用户未选择文件，则退出
		return;
	}

	QFile file(filePath); // 创建一个QFile对象来处理所选文件
	if (!file.open(QIODevice::WriteOnly)) { // 以只写模式打开文件
		QMessageBox::critical(this, "Error", "Failed to save project file."); // 弹出错误消息框
		log.warn("无法打开工程文件"); // 记录警告日志
		return;
	}

	QJsonObject project; // 创建一个JSON对象来存储项目信息

	// 保存矢量图层信息
	QJsonArray vecLayersArray; // 创建一个数组来存储所有矢量图层的信息
	for (int i = 0; i < mpLayerManager->mlVectorLayers.size(); ++i) {
		QJsonObject vecLayerObj;
		vecLayerObj["vecFileName"] = mpLayerManager->mlVectorLayers[i]; // 保存矢量图层文件路径
		//vecLayerObj["vecZValue"] = mpLayerManager->mlpVectorLayer[i]->getZValue(); // 这里注释掉了Z值保存
		vecLayersArray.append(vecLayerObj); // 将图层对象添加到数组中
	}
	project["vecLayers"] = vecLayersArray; // 将矢量图层数组添加到项目JSON对象中
	log.info("矢量图层保存成功"); // 记录信息日志

	// 保存栅格图层信息
	QJsonArray rasLayersArray; // 创建一个数组来存储所有栅格图层的信息
	for (int i = 0; i < mpLayerManager->mlRasterLayers.size(); ++i) {
		QJsonObject rasLayerObj;
		rasLayerObj["rasFileName"] = mpLayerManager->mlRasterLayers[i]; // 保存栅格图层文件路径
		//rasLayerObj["rasZValue"] = mpLayerManager->mlpRasterLayer[i]->getZValue(); // 这里注释掉了Z值保存
		rasLayersArray.append(rasLayerObj); // 将图层对象添加到数组中
	}
	project["rasLayers"] = rasLayersArray; // 将栅格图层数组添加到项目JSON对象中
	log.info("栅格图层保存成功"); // 记录信息日志

	// 保存视图设置
	QJsonObject viewSettings;
	viewSettings["scale"] = ui.graphicsView->transform().m11(); // 获取当前视图的缩放级别并保存
	project["viewSettings"] = viewSettings; // 将视图设置添加到项目JSON对象中
	log.info("视图设置保存成功"); // 记录信息日志

	QJsonDocument doc(project); // 将JSON对象转换为QJsonDocument
	file.write(doc.toJson()); // 将文档写入文件
	file.close(); // 关闭文件
}

