#include "textStorage.h"
#include <fstream>
#include <QPen>
#include <QBrush>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>

textStorage::textStorage(const std::string& filePath)
	: mFilePath(filePath) {}

void textStorage::storeVectorData(const MyVectorLayer& layer) {
	std::ofstream outFile(mFilePath);
	if (!outFile.is_open()) {
		throw std::runtime_error("Failed to open file for writing");
	}

	// 写入几何类型
	outFile << "Geometry Type: " << static_cast<int>(layer.getType()) << "\n";

	// 写入字段名称
	const auto& fieldNames = layer.getFieldNames();
	outFile << "Fields: ";
	for (const auto& fieldName : fieldNames) {
		outFile << fieldName.toStdString() << " ";
	}
	outFile << "\n";

	// 写入记录和几何信息
	const auto& records = layer.getRecords();
	const auto& graphicsItems = layer.getGraphicsItem();

	for (int i = 0; i < records.size(); ++i) {
		// 写入属性值
		for (const auto& value : records[i].values) {
			outFile << value.toString().toStdString() << " ";
		}

		// 写入几何信息
		QGraphicsItem* graphicsItem = graphicsItems[i];
		if (QGraphicsEllipseItem* pointItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
			// 存储点的坐标
			QPointF point = pointItem->rect().center();
			outFile << "Point: (" << point.x() << ", " << point.y() << ")";
		}
		else if (QGraphicsPathItem* lineItem = dynamic_cast<QGraphicsPathItem*>(graphicsItem)) {
			// 存储线的起点和终点坐标
			QPainterPath path = lineItem->path();
			outFile << "Line: ";
			for (int j = 0; j < path.elementCount(); ++j) {
				QPointF point(path.elementAt(j).x, path.elementAt(j).y);
				outFile << "(" << point.x() << ", " << point.y() << ") ";
			}
		}
		else if (QGraphicsPolygonItem* polygonItem = dynamic_cast<QGraphicsPolygonItem*>(graphicsItem)) {
			// 存储多边形的顶点坐标
			QPolygonF polygon = polygonItem->polygon();
			outFile << "Polygon: ";
			for (const QPointF& point : polygon) {
				outFile << "(" << point.x() << ", " << point.y() << ") ";
			}
		}

		outFile << "\n";
	}

	outFile.close();
}