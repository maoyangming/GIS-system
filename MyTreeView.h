#pragma once

#include <QTreeView>
#include <QMenu>
#include <QStandardItemModel>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QColorDialog>
#include <QPainter>
#include <QBrush>

#include "MyItem.h"

// 前向声明 MyLayerManager 类，避免循环依赖
class MyLayerManager;

// MyTreeView 类继承自 QTreeView，用于管理和操作图层，支持上下文菜单功能
class MyTreeView : public QTreeView {
	Q_OBJECT

public:
	// 构造函数
	MyTreeView(QWidget* parent = nullptr);

	// 析构函数
	~MyTreeView();

	// 连接图形场景，用于在场景中操作图层
	// 参数：
	// - pScene: 指向 QGraphicsScene 的指针
	void connectScene(QGraphicsScene* pScene) {
		mpScene = pScene;
	}

	// 连接图层管理器，用于管理图层数据
	// 参数：
	// - pManager: 指向 MyLayerManager 的指针
	void connectLayerManager(MyLayerManager* pManager) {
		mpManager = pManager;
	}

private slots:
	// 打开上下文菜单的槽函数
	// 参数：
	// - position: 鼠标右键点击的位置
	void openContextMenu(const QPoint& position);

	// 当树视图中的项目发生变化时调用的槽函数
	// 参数：
	// - item: 被修改的 QStandardItem 项目
	void onItemChanged(QStandardItem* item);

	// 执行操作1的槽函数
	void performAction1();

	// 执行操作2的槽函数
	void performAction2();

	// 执行操作3的槽函数
	void performAction3();

	// 执行操作4的槽函数
	void performAction4();

	// 执行操作5的槽函数
	void performAction5();

private:
	MyItem* mpChoosedItem; // 当前选中的图层项
	QStandardItemModel* mpModel; // 树视图的模型
	QGraphicsScene* mpScene; // 指向图形场景的指针，用于图层渲染
	MyLayerManager* mpManager; // 指向图层管理器的指针，用于管理图层数据
};
