#include "dataStorage.h"
#include <stdexcept>

// 构造函数，打开指定的SQLite数据库文件
// 参数：
// - dbFilePath: 数据库文件路径
dataStorage::dataStorage(const std::string& dbFilePath) {
	if (sqlite3_open(dbFilePath.c_str(), &mDb) != SQLITE_OK) {
		throw std::runtime_error("Failed to open database");
	}
}

// 存储矢量数据到SQLite数据库中
// 参数：
// - layer: 包含矢量图层数据的MyVectorLayer对象
void dataStorage::storeVectorData(const MyVectorLayer& layer) {
	// 创建图层表，用于存储图层的元数据（如图层名称和几何类型）
	std::string createTableSQL = "CREATE TABLE IF NOT EXISTS Layers ("
		"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
		"LayerName TEXT, "
		"GeometryType INTEGER);";
	sqlite3_exec(mDb, createTableSQL.c_str(), nullptr, nullptr, nullptr);

	// 插入图层信息，包括图层名称和几何类型
	std::string insertLayerSQL = "INSERT INTO Layers (LayerName, GeometryType) VALUES (?, ?);";
	sqlite3_stmt* stmt;
	sqlite3_prepare_v2(mDb, insertLayerSQL.c_str(), -1, &stmt, nullptr);
	sqlite3_bind_text(stmt, 1, layer.getName().toStdString().c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, static_cast<int>(layer.getType()));
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	// 获取刚插入的图层ID，用于关联后续的字段和记录
	int layerId = static_cast<int>(sqlite3_last_insert_rowid(mDb));

	// 创建字段表，用于存储图层中的字段信息
	std::string createFieldsTableSQL = "CREATE TABLE IF NOT EXISTS Fields ("
		"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
		"LayerID INTEGER, "
		"FieldName TEXT, "
		"FOREIGN KEY (LayerID) REFERENCES Layers(ID));";
	sqlite3_exec(mDb, createFieldsTableSQL.c_str(), nullptr, nullptr, nullptr);

	// 插入字段信息，将图层中的每个字段名称保存到数据库中
	const auto& fieldNames = layer.getFieldNames();
	std::string insertFieldSQL = "INSERT INTO Fields (LayerID, FieldName) VALUES (?, ?);";
	sqlite3_prepare_v2(mDb, insertFieldSQL.c_str(), -1, &stmt, nullptr);
	for (const auto& fieldName : fieldNames) {
		sqlite3_bind_int(stmt, 1, layerId);
		sqlite3_bind_text(stmt, 2, fieldName.toStdString().c_str(), -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_reset(stmt);
	}
	sqlite3_finalize(stmt);

	// 创建记录表，用于存储图层的实际数据记录以及对应的几何信息
	std::string createRecordsTableSQL = "CREATE TABLE IF NOT EXISTS Records ("
		"ID INTEGER PRIMARY KEY AUTOINCREMENT, "
		"LayerID INTEGER, "
		"RecordData TEXT, "
		"GeometryData TEXT, "
		"FOREIGN KEY (LayerID) REFERENCES Layers(ID));";
	sqlite3_exec(mDb, createRecordsTableSQL.c_str(), nullptr, nullptr, nullptr);

	// 插入记录信息和几何信息
	// 每条记录包括属性数据和几何数据，几何数据根据图形类型进行适当的序列化
	const auto& records = layer.getRecords();
	const auto& graphicsItems = layer.getGraphicsItem();
	std::string insertRecordSQL = "INSERT INTO Records (LayerID, RecordData, GeometryData) VALUES (?, ?, ?);";
	sqlite3_prepare_v2(mDb, insertRecordSQL.c_str(), -1, &stmt, nullptr);
	for (int i = 0; i < records.size(); ++i) {
		const auto& record = records[i];

		// 拼接记录数据，将每个字段的值转换为字符串并连接起来
		std::string recordData;
		for (const auto& value : record.values) {
			recordData += value.toString().toStdString() + " ";
		}

		// 拼接几何数据，根据几何类型（点、线、多边形）生成对应的几何描述字符串
		std::string geometryData;
		QGraphicsItem* graphicsItem = graphicsItems[i];
		if (QGraphicsEllipseItem* pointItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
			// 处理点数据
			QPointF point = pointItem->rect().center();
			geometryData = "Point: (" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ")";
		}
		else if (QGraphicsPathItem* lineItem = dynamic_cast<QGraphicsPathItem*>(graphicsItem)) {
			// 处理线数据
			QPainterPath path = lineItem->path();
			geometryData = "Line: ";
			for (int j = 0; j < path.elementCount(); ++j) {
				QPointF point(path.elementAt(j).x, path.elementAt(j).y);
				geometryData += "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ") ";
			}
		}
		else if (QGraphicsPolygonItem* polygonItem = dynamic_cast<QGraphicsPolygonItem*>(graphicsItem)) {
			// 处理多边形数据
			QPolygonF polygon = polygonItem->polygon();
			geometryData = "Polygon: ";
			for (const QPointF& point : polygon) {
				geometryData += "(" + std::to_string(point.x()) + ", " + std::to_string(point.y()) + ") ";
			}
		}

		// 执行SQL插入，将记录数据和几何数据保存到数据库中
		sqlite3_bind_int(stmt, 1, layerId);
		sqlite3_bind_text(stmt, 2, recordData.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(stmt, 3, geometryData.c_str(), -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_reset(stmt);
	}
	sqlite3_finalize(stmt);
}

// 析构函数，关闭SQLite数据库连接
dataStorage::~dataStorage() {
	sqlite3_close(mDb);
}
