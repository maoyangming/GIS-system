
#pragma once
#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QScrollBar>

class MyGraphicsView : public QGraphicsView {
	Q_OBJECT
public:
	// 构造函数，初始化自定义的 QGraphicsView
	MyGraphicsView(QWidget* parent)
		: QGraphicsView(parent), _isPanning(true), _panStartX(0), _panStartY(0) {
		setRenderHint(QPainter::Antialiasing); // 开启抗锯齿渲染
		setDragMode(QGraphicsView::NoDrag); // 禁用拖动模式
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用水平滚动条
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 禁用垂直滚动条
		setMouseTracking(true); // 开启鼠标追踪
		setTransformationAnchor(QGraphicsView::AnchorUnderMouse); // 设置变换锚点为鼠标位置
		setResizeAnchor(QGraphicsView::AnchorUnderMouse); // 设置缩放锚点为鼠标位置
		setViewportUpdateMode(QGraphicsView::SmartViewportUpdate); // 设置视口更新模式为智能更新
	}

	// 设置编辑模式
	void setEditing(bool isEditing) {
		_isEditing = isEditing; // 更新编辑模式标志
	}

protected:
	// 重载鼠标滚轮事件，实现视图缩放功能
	void wheelEvent(QWheelEvent* event) override {
		const double zoomInFactor = 1.25; // 放大因子
		const double zoomOutFactor = 1.0 / zoomInFactor; // 缩小因子

		QPointF oldPos = mapToScene(event->position().toPoint()); // 获取缩放前的鼠标位置在场景中的坐标

		// 根据滚轮滚动方向决定缩放比例
		double zoomFactor = (event->angleDelta().y() > 0) ? zoomInFactor : zoomOutFactor;
		scale(zoomFactor, zoomFactor); // 执行缩放操作

		QPointF newPos = mapToScene(event->position().toPoint()); // 获取缩放后的鼠标位置在场景中的坐标

		QPointF delta = newPos - oldPos; // 计算鼠标位置的偏移量
		translate(delta.x(), delta.y()); // 平移视图，使缩放后鼠标位置保持在原地
	}

	// 重载鼠标按下事件，处理平移和编辑模式下的操作
	void mousePressEvent(QMouseEvent* event) override {
		if (_isEditing) {
			QGraphicsView::mousePressEvent(event); // 在编辑模式下，执行默认的鼠标按下处理
			return;
		}
		if (event->button() == Qt::LeftButton) {
			_isPanning = true; // 开始平移操作
			setCursor(Qt::ClosedHandCursor); // 将光标设置为闭合手型
			_panStartX = event->x(); // 记录平移的起始位置X坐标
			_panStartY = event->y(); // 记录平移的起始位置Y坐标
		}
		QGraphicsView::mousePressEvent(event); // 执行默认的鼠标按下处理
	}

	// 重载鼠标移动事件，处理平移和编辑模式下的操作
	void mouseMoveEvent(QMouseEvent* event) override {
		if (_isEditing) {
			QGraphicsView::mouseMoveEvent(event); // 在编辑模式下，执行默认的鼠标移动处理
			return;
		}
		if (_isPanning) {
			int deltaX = event->x() - _panStartX; // 计算鼠标移动的X轴偏移量
			int deltaY = event->y() - _panStartY; // 计算鼠标移动的Y轴偏移量
			_panStartX = event->x(); // 更新起始位置X坐标
			_panStartY = event->y(); // 更新起始位置Y坐标
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() - deltaX); // 平移水平滚动条
			verticalScrollBar()->setValue(verticalScrollBar()->value() - deltaY); // 平移垂直滚动条
		}
		QGraphicsView::mouseMoveEvent(event); // 执行默认的鼠标移动处理
	}

	// 重载鼠标释放事件，结束平移操作或执行编辑模式下的操作
	void mouseReleaseEvent(QMouseEvent* event) override {
		if (_isEditing) {
			QGraphicsView::mouseReleaseEvent(event); // 在编辑模式下，执行默认的鼠标释放处理
			return;
		}
		if (event->button() == Qt::LeftButton) {
			_isPanning = false; // 结束平移操作
			setCursor(Qt::ArrowCursor); // 将光标设置回箭头形状
		}
		QGraphicsView::mouseReleaseEvent(event); // 执行默认的鼠标释放处理
	}

private:
	bool _isPanning; // 平移模式标志，标记当前是否正在执行平移操作
	bool _isEditing; // 编辑模式标志，标记当前是否处于编辑模式
	int _panStartX; // 平移起始位置的X坐标
	int _panStartY; // 平移起始位置的Y坐标
};


