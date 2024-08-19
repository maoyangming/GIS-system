#pragma once

#include <QTreeWidget>
#include <QGraphicsView>
#include <QStandardItemModel>
#include <QGraphicsScene>
#include <QCheckBox>
#include <QIcon>

#include "gdal_priv.h"
#include "ogrsf_frmts.h"
#include "MyGraphicsView.h"
#include "MyTreeView.h"

// MyLayerManager 类用于管理矢量和栅格图层
class MyLayerManager {
public:
	// 构造函数，初始化 TreeView、GraphicsView 和 QAction 的指针，并将其连接
	MyLayerManager(MyTreeView* pTreeView, MyGraphicsView* pGraphicsView, QAction* pEdit, QAction* pAttributes)
		: mpTreeView(pTreeView), mpGraphicsView(pGraphicsView), mpEdit(pEdit), mpAttributes(pAttributes) {
		pTreeView->connectLayerManager(this);  // 连接 LayerManager 到 TreeView
		pTreeView->connectScene(pGraphicsView->scene());  // 连接场景到 TreeView
	}

	// 析构函数
	~MyLayerManager() {}

	QStringList mlVectorLayers;  // 矢量图层路径列表
	QStringList mlRasterLayers;  // 栅格图层路径列表
	//QList<MyVectorLayer*> mlpVectorLayer;  // 矢量图层对象列表（注释掉）
	//QList<MyRasterLayer*> mlpRasterLayer;  // 栅格图层对象列表（注释掉）

	// 添加矢量图层到 TreeView 中
	void addVectorLayer(MyVectorLayer* pVectorLayer) {
		//mlpVectorLayer.append(pVectorLayer);  // 将矢量图层添加到对象列表（注释掉）
		// 创建新的 MyItem 项目，并与矢量图层及其名称关联
		MyItem* pNewItem = new MyItem(pVectorLayer, pVectorLayer->getName());
		pNewItem->setCheckable(true);  // 设置项目可选中
		pNewItem->setCheckState(pVectorLayer->isVisible() ? Qt::Checked : Qt::Unchecked);  // 根据图层可见性设置选中状态

		// 根据矢量图层的类型设置相应的图标
		if (pNewItem->getVectorLayer()->getType() == GeometryType::Polygon) {
			QIcon icon(":/MyGIS/icons/polygon.png");
			pNewItem->setIcon(icon);
		}
		else if (pNewItem->getVectorLayer()->getType() == GeometryType::Line) {
			QIcon icon(":/MyGIS/icons/line.png");
			pNewItem->setIcon(icon);
		}
		else if (pNewItem->getVectorLayer()->getType() == GeometryType::Point) {
			QIcon icon(":/MyGIS/icons/point.png");
			pNewItem->setIcon(icon);
		}

		// 将新创建的项目添加到 TreeView 的模型中
		qobject_cast<QStandardItemModel*>(mpTreeView->model())->appendRow(pNewItem);
	}

	// 添加栅格图层到 TreeView 中
	void addRasterLayer(MyRasterLayer* pRasterLayer) {
		//mlpRasterLayer.append(pRasterLayer);  // 将栅格图层添加到对象列表（注释掉）
		// 创建新的 MyItem 项目，并与栅格图层及其名称关联
		MyItem* pNewItem = new MyItem(pRasterLayer, pRasterLayer->getName());
		pNewItem->setCheckable(true);  // 设置项目可选中
		pNewItem->setCheckState(pRasterLayer->isVisible() ? Qt::Checked : Qt::Unchecked);  // 根据图层可见性设置选中状态

		// 如果栅格图层存在，则设置图标
		if (pNewItem->getRasterLayer()) {
			QIcon icon(":/MyGIS/icons/raster.png");
			pNewItem->setIcon(icon);
		}

		// 将新创建的项目添加到 TreeView 的模型中
		qobject_cast<QStandardItemModel*>(mpTreeView->model())->appendRow(pNewItem);
	}

	// 删除矢量图层
	void deleteVectorLayer(MyVectorLayer* pVectorLayer) {
		//mlpVectorLayer.removeOne(pVectorLayer);  // 从对象列表中移除矢量图层（注释掉）
		mlVectorLayers.removeOne(pVectorLayer->getFilePath());  // 从路径列表中移除矢量图层

		// 从场景中删除与图层关联的所有图形项
		for (auto graphicsItem : pVectorLayer->getGraphicsItem()) {
			if (graphicsItem) {
				mpGraphicsView->scene()->removeItem(graphicsItem);
			}
		}

		// 禁用编辑和属性 QAction
		mpEdit->setEnabled(false);
		mpAttributes->setEnabled(false);

		// 删除图层对象，并将指针置为 nullptr
		delete pVectorLayer;
		pVectorLayer = nullptr;
	}

	// 删除栅格图层
	void deleteRasterLayer(MyRasterLayer* pRasterLayer) {
		//mlpRasterLayer.removeOne(pRasterLayer);  // 从对象列表中移除栅格图层（注释掉）
		mlRasterLayers.removeOne(pRasterLayer->getFilePath());  // 从路径列表中移除栅格图层

		// 获取栅格数据并从场景中移除
		QGraphicsPixmapItem* pPixmapItem = pRasterLayer->getRasterData();
		if (pPixmapItem) {
			mpGraphicsView->scene()->removeItem(pPixmapItem);
		}

		// 禁用编辑和属性 QAction
		mpEdit->setEnabled(false);
		mpAttributes->setEnabled(false);

		// 删除图层对象，并将指针置为 nullptr
		delete pRasterLayer;
		pRasterLayer = nullptr;
	}

	// 缩放至矢量图层
	void moveToVectorLayer(MyVectorLayer* pVectorLayer) {
		// 获取第一个图形项的边界矩形
		QRectF boundingRect = pVectorLayer->getGraphicsItem()[0]->boundingRect();
		// 合并所有图形项的边界矩形，计算出总的边界矩形
		for (const auto& item : pVectorLayer->getGraphicsItem()) {
			boundingRect = boundingRect.united(item->boundingRect());
		}
		// 调整视图以适应总的边界矩形，保持纵横比
		mpGraphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
	}

	// 缩放至栅格图层
	void moveToRasterLayer(MyRasterLayer* pRasterLayer) {
		double originX = pRasterLayer->getOriginX();  // 栅格图层左上角的 X 坐标
		double originY = pRasterLayer->getOriginY();  // 栅格图层左上角的 Y 坐标
		double pixelWidth = pRasterLayer->getPixelWidth();  // 每个像素的宽度
		double pixelHeight = pRasterLayer->getPixelHeight();  // 每个像素的高度

		// 计算栅格图层的地理范围
		QRectF boundingRect(QPointF(originX, originY + pRasterLayer->getColCount() * pixelHeight),
			QSizeF(pRasterLayer->getRowCount() * pixelWidth, pRasterLayer->getColCount() * std::abs(pixelHeight)));

		// 调整视图以适应栅格图层的地理范围，保持纵横比
		mpGraphicsView->fitInView(boundingRect, Qt::KeepAspectRatio);
	}

	// 将矢量图层上移
	void upVectorLayer(MyVectorLayer* pVectorLayer) {
		// 将图层的每个图形项的 z 值增加 1，使图层上移
		for (auto graphicsItem : pVectorLayer->getGraphicsItem()) {
			qreal zValue = graphicsItem->zValue();
			graphicsItem->setZValue(zValue + 1);
		}
		// 更新图层的 z 值
		pVectorLayer->setZValue(pVectorLayer->getGraphicsItem()[0]->zValue());
	}

	// 将栅格图层上移
	void upRasterLayer(MyRasterLayer* pRasterLayer) {
		QGraphicsPixmapItem* pPixmapItem = pRasterLayer->getRasterData();  // 获取栅格图层的数据项
		qreal zValue = pPixmapItem->zValue();  // 获取当前 z 值
		pPixmapItem->setZValue(zValue + 1);  // 将 z 值增加 1，使图层上移
		pRasterLayer->setZValue(zValue);  // 更新图层的 z 值
	}

	// 图层下移（矢量图层）
	void downVectorLayer(MyVectorLayer* pVectorLayer) {
		// 遍历与矢量图层关联的所有图形项，将它们的 z 值减少 1，使图层下移
		for (auto graphicsItem : pVectorLayer->getGraphicsItem()) {
			qreal zValue = graphicsItem->zValue();  // 获取当前 z 值
			graphicsItem->setZValue(zValue - 1);    // 将 z 值减少 1
		}
		// 更新矢量图层的 z 值
		pVectorLayer->setZValue(pVectorLayer->getGraphicsItem()[0]->zValue());
	}

	// 图层下移（栅格图层）
	void downRasterLayer(MyRasterLayer* pRasterLayer) {
		QGraphicsPixmapItem* pPixmapItem = pRasterLayer->getRasterData();  // 获取栅格图层的数据项
		qreal zValue = pPixmapItem->zValue();  // 获取当前 z 值
		pPixmapItem->setZValue(zValue - 1);    // 将 z 值减少 1，使图层下移
		pRasterLayer->setZValue(zValue);       // 更新栅格图层的 z 值
	}

private:
	MyTreeView* mpTreeView;            // 指向 MyTreeView 对象的指针，用于管理图层的树形视图
	MyGraphicsView* mpGraphicsView;    // 指向 MyGraphicsView 对象的指针，用于在场景中显示图层
	QAction* mpEdit;                   // 指向编辑 QAction 的指针，用于启用或禁用编辑功能
	QAction* mpAttributes;             // 指向属性 QAction 的指针，用于启用或禁用属性查看功能

};
