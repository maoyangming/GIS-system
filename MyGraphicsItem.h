
#pragma once
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPolygonItem>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsSceneEvent>
#include <QColorDialog>
#include <QPainter>
#include <QVector>
#include <QCursor>
#include <QPointF>

/*============可编辑的面======================================================================================*/
class MyPolygonItem : public QGraphicsPolygonItem {
public:
    MyPolygonItem(const QPolygonF& polygon, QGraphicsItem* parent = nullptr)
        : QGraphicsPolygonItem(polygon, parent) {
        // 设置图形项的属性和行为
        setPen(QPen(Qt::black, 0.002));
        setBrush(QBrush(Qt::lightGray));
        setFlag(QGraphicsItem::ItemIsMovable, true);  // 允许移动
        setFlag(QGraphicsItem::ItemIsSelectable, true);  // 允许选择
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);  // 允许发送几何变化信号
        setAcceptHoverEvents(true);  // 接受鼠标悬停事件

        createHandles(); // 创建悬挂点
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override {
        for (QGraphicsEllipseItem* handle : mvpHandles) {
            if (handle->contains(event->pos())) {
                setCursor(Qt::SizeAllCursor);  // 如果鼠标在控制点上，则设置为移动光标
                return;
            }
        }
        setCursor(Qt::ArrowCursor);  // 否则设置为箭头光标
        QGraphicsPolygonItem::hoverMoveEvent(event);  // 调用基类的悬停事件处理函数
    }

	void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
		// 如果按下的是右键，则弹出颜色选择对话框，让用户选择多边形的填充颜色
		if (event->button() == Qt::RightButton) {
			QColor color = QColorDialog::getColor(brush().color(), nullptr, "Select Color");
			// 如果用户选择了有效的颜色，则将多边形的填充颜色更新为用户选择的颜色
			if (color.isValid()) {
				setBrush(QBrush(color));
			}
			return;
		}
		// 检查鼠标是否点击了手柄（handle）
		for (QGraphicsEllipseItem* handle : mvpHandles) {
			if (handle->contains(event->pos())) {
				mpMovingHandle = handle; // 如果点击了手柄，则将该手柄标记为正在移动
				return;
			}
		}
		// 如果没有点击手柄，则调用基类的鼠标按下事件处理
		QGraphicsPolygonItem::mousePressEvent(event);
	}

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
		// 如果正在移动手柄
		if (mpMovingHandle) {
			// 更新手柄的位置，并重新绘制手柄的矩形框
			QPointF newPos = event->pos();
			mpMovingHandle->setRect(newPos.x() - SHandleRadius, newPos.y() - SHandleRadius, SHandleRadius * 2, SHandleRadius * 2);
			// 更新多边形的形状，以反映手柄位置的变化
			updatePolygonFromHandles();
		}
		else {
			// 如果没有移动手柄，则调用基类的鼠标移动事件处理
			QGraphicsPolygonItem::mouseMoveEvent(event);
		}
	}

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
		// 当鼠标释放时，清除正在移动的手柄的指针
		if (mpMovingHandle) {
			mpMovingHandle = nullptr;
		}
		// 调用基类的鼠标释放事件处理
		QGraphicsPolygonItem::mouseReleaseEvent(event);
	}

	QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
		// 当多边形的位置或变换发生变化时，更新手柄的位置
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemTransformChange) {
			updateHandles(); // 更新手柄位置
		}
		// 调用基类的项变化处理函数
		return QGraphicsPolygonItem::itemChange(change, value);
	}

private:
	void createHandles() {
		// 根据多边形的顶点创建手柄
		const QPolygonF& poly = polygon();
		for (const QPointF& point : poly) {
			// 为每个顶点创建一个手柄（红色的小圆）
			QGraphicsEllipseItem* handle = new QGraphicsEllipseItem(
				QRectF(point.x() - SHandleRadius, point.y() - SHandleRadius, SHandleRadius * 2, SHandleRadius * 2), this);
			handle->setFlag(QGraphicsItem::ItemIsMovable, true); // 使手柄可以移动
			handle->setBrush(Qt::red); // 设置手柄的填充颜色为红色
			handle->setPen(QPen(Qt::black, 0.001)); // 设置手柄的边框颜色和宽度
			mvpHandles.append(handle); // 将手柄添加到手柄列表中
		}
		updateHandles(); // 更新手柄位置以确保与多边形的顶点对齐
	}

	void updateHandles() {
		// 更新手柄的位置，使其与多边形的顶点位置同步
		const QPolygonF& poly = polygon();
		for (int i = 0; i < mvpHandles.size(); ++i) {
			mvpHandles[i]->setPos(poly[i]); // 设置手柄的位置为对应的多边形顶点的位置
		}
	}


    void updatePolygonFromHandles() {
        QPolygonF newPolygon;
        for (QGraphicsEllipseItem* handle : mvpHandles) {
            newPolygon << handle->rect().center();  // 将控制点的中心坐标添加到新的多边形顶点中
        }
        setPolygon(newPolygon);  // 更新多边形的顶点坐标
    }

    static constexpr qreal SHandleRadius = 0.001;
    QVector<QGraphicsEllipseItem*> mvpHandles;
    QGraphicsEllipseItem* mpMovingHandle = nullptr;
};

/*============可编辑的线======================================================================================*/
class MyPathItem : public QGraphicsPathItem {
public:
    MyPathItem(const QPainterPath& path, QGraphicsItem* parent = nullptr)
        : QGraphicsPathItem(path, parent) {
        // 设置图形项的属性和行为
        setPen(QPen(Qt::black, 0.002));
        setBrush(QBrush(Qt::NoBrush));  // 不填充
        setFlag(QGraphicsItem::ItemIsMovable, true);  // 允许移动
        setFlag(QGraphicsItem::ItemIsSelectable, true);  // 允许选择
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);  // 允许发送几何变化信号
        setAcceptHoverEvents(true);  // 接受鼠标悬停事件

        createHandles();  // 创建控制点
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override {
        for (QGraphicsEllipseItem* handle : mvpHandles) {
            if (handle->contains(event->pos())) {
                setCursor(Qt::SizeAllCursor);  // 如果鼠标在控制点上，则设置为移动光标
                return;
            }
        }
        setCursor(Qt::ArrowCursor);  // 否则设置为箭头光标
        QGraphicsPathItem::hoverMoveEvent(event);  // 调用基类的悬停事件处理函数
    }

	void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
		// 如果按下的是右键，则弹出颜色选择对话框，让用户选择路径的颜色
		if (event->button() == Qt::RightButton) {
			QColor color = QColorDialog::getColor(pen().color(), nullptr, "Select Color");
			// 如果用户选择了有效的颜色，则将路径的颜色更新为用户选择的颜色
			if (color.isValid()) {
				setPen(QPen(color, pen().widthF()));
			}
			return;
		}

		// 检查鼠标是否点击了手柄（handle）
		for (QGraphicsEllipseItem* handle : mvpHandles) {
			if (handle->contains(event->pos())) {
				mpMovingHandle = handle; // 如果点击了手柄，则将该手柄标记为正在移动
				return;
			}
		}

		// 如果没有点击手柄，则调用基类的鼠标按下事件处理
		QGraphicsPathItem::mousePressEvent(event);
	}

	void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override {
		// 如果正在移动手柄
		if (mpMovingHandle) {
			// 更新手柄的位置，并重新绘制手柄的矩形框
			QPointF newPos = event->pos();
			mpMovingHandle->setRect(newPos.x() - SHandleRadius, newPos.y() - SHandleRadius, SHandleRadius * 2, SHandleRadius * 2);
			// 更新路径，以反映手柄位置的变化
			updatePathFromHandles();
		}
		else {
			// 如果没有移动手柄，则调用基类的鼠标移动事件处理
			QGraphicsPathItem::mouseMoveEvent(event);
		}
	}

	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
		// 当鼠标释放时，清除正在移动的手柄的指针
		if (mpMovingHandle) {
			mpMovingHandle = nullptr;
		}
		// 调用基类的鼠标释放事件处理
		QGraphicsPathItem::mouseReleaseEvent(event);
	}

	QVariant itemChange(GraphicsItemChange change, const QVariant& value) override {
		// 当路径的位置或变换发生变化时，更新手柄的位置
		if (change == QGraphicsItem::ItemPositionChange || change == QGraphicsItem::ItemTransformChange) {
			updateHandles(); // 更新手柄位置
		}
		// 调用基类的项变化处理函数
		return QGraphicsPathItem::itemChange(change, value);
	}

private:
	// 创建用于操作路径的手柄
	void createHandles() {
		const QPainterPath& path = this->path();
		// 遍历路径中的每个元素，为每个点创建一个手柄
		for (int i = 0; i < path.elementCount(); ++i) {
			const QPainterPath::Element& element = path.elementAt(i);
			QGraphicsEllipseItem* handle = new QGraphicsEllipseItem(
				QRectF(element.x - SHandleRadius, element.y - SHandleRadius, SHandleRadius * 2, SHandleRadius * 2), this);
			handle->setFlag(QGraphicsItem::ItemIsMovable, true); // 使手柄可以移动
			handle->setBrush(Qt::red); // 设置手柄的填充颜色为红色
			handle->setPen(QPen(Qt::black, 0.001)); // 设置手柄的边框颜色和宽度
			mvpHandles.append(handle); // 将手柄添加到手柄列表中
		}
		updateHandles(); // 更新手柄位置以确保与路径的点对齐
	}

	// 更新手柄的位置，使其与路径中的点同步
	void updateHandles() {
		const QPainterPath& path = this->path();
		for (int i = 0; i < mvpHandles.size(); ++i) {
			const QPainterPath::Element& element = path.elementAt(i);
			mvpHandles[i]->setPos(QPointF(element.x, element.y)); // 设置手柄的位置为路径中的对应点的位置
		}
	}

	// 根据手柄的位置更新路径
	void updatePathFromHandles() {
		QPainterPath newPath;
		// 使用手柄的位置重新构建路径
		newPath.moveTo(mvpHandles[0]->rect().center());
		for (int i = 1; i < mvpHandles.size(); ++i) {
			newPath.lineTo(mvpHandles[i]->rect().center());
		}
		setPath(newPath); // 更新路径
	}

	// 手柄的半径，决定了手柄的大小
	static constexpr qreal SHandleRadius = 0.001;

	// 存储所有手柄的列表，用于操作路径的顶点
	QVector<QGraphicsEllipseItem*> mvpHandles;

	// 当前正在移动的手柄的指针，如果没有手柄正在移动，则为nullptr
	QGraphicsEllipseItem* mpMovingHandle = nullptr;
};


/*============可编辑的点======================================================================================*/
class MyPointItem : public QGraphicsEllipseItem {
public:
    MyPointItem(const QPointF& point, QGraphicsItem* parent = nullptr)
        : QGraphicsEllipseItem(QRectF(point.x() - SHandleRadius, point.y() - SHandleRadius, SHandleRadius * 2, SHandleRadius * 2), parent) {
        setBrush(Qt::blue);
        setPen(QPen(Qt::black, 0.002));
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::RightButton) {
            QColor color = QColorDialog::getColor(brush().color(), nullptr, "Select Color");
            if (color.isValid()) {
                setBrush(QBrush(color));
            }
            return;
        }
        QGraphicsEllipseItem::mousePressEvent(event);
    }

private:
    static constexpr qreal SHandleRadius = 0.001;
};
