#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

#include "MyTreeView.h"
#include "MyLayerManager.h"

// MyTreeView 构造函数，初始化 QTreeView 和相关组件
MyTreeView::MyTreeView(QWidget* parent)
	: QTreeView(parent), mpChoosedItem(nullptr) {
	// 创建一个 QStandardItemModel 并将其设置为 QTreeView 的模型
	mpModel = new QStandardItemModel(this);
	setModel(mpModel);

	// 设置上下文菜单策略，允许通过右键单击触发上下文菜单
	setContextMenuPolicy(Qt::CustomContextMenu);

	// 连接信号和槽，处理上下文菜单请求和项目更改事件
	connect(this, &QTreeView::customContextMenuRequested, this, &MyTreeView::openContextMenu);
	connect(mpModel, &QStandardItemModel::itemChanged, this, &MyTreeView::onItemChanged);
}

// MyTreeView 析构函数，删除模型
MyTreeView::~MyTreeView() {
	delete mpModel;
}

// 打开上下文菜单的槽函数
void MyTreeView::openContextMenu(const QPoint& position) {
	// 获取用户点击位置的索引
	QModelIndex index = indexAt(position);
	if (index.isValid()) {
		// 根据索引获取用户选择的项目
		mpChoosedItem = static_cast<MyItem*>(mpModel->itemFromIndex(index));

		if (mpChoosedItem) {
			// 创建上下文菜单并添加菜单项
			QMenu menu;
			QAction* action1 = menu.addAction(QIcon(":/MyGIS/icons/up.png"), "向上");
			QAction* action2 = menu.addAction(QIcon(":/MyGIS/icons/down.png"), "向下");
			QAction* action3 = menu.addAction(QIcon(":/MyGIS/icons/delete.png"), "删除");
			menu.addSeparator(); // 添加分隔符
			QAction* action4 = menu.addAction(QIcon(":/MyGIS/icons/zoom.png"), "缩放至图层");
			QAction* action5 = menu.addAction(QIcon(":/MyGIS/icons/color.png"), "设置图层颜色");

			// 如果选择的项目关联了一个矢量图层且该图层正在编辑，则禁用相关操作
			if (mpChoosedItem->getVectorLayer()) {
				if (mpChoosedItem->getVectorLayer()->isEditing()) {
					action1->setEnabled(false);
					action2->setEnabled(false);
					action3->setEnabled(false);
					action4->setEnabled(false);
					action5->setEnabled(false);
				}
			}

			// 连接菜单项的信号到相应的槽函数
			connect(action1, &QAction::triggered, this, &MyTreeView::performAction1);
			connect(action2, &QAction::triggered, this, &MyTreeView::performAction2);
			connect(action3, &QAction::triggered, this, &MyTreeView::performAction3);
			connect(action4, &QAction::triggered, this, &MyTreeView::performAction4);
			connect(action5, &QAction::triggered, this, &MyTreeView::performAction5);

			// 显示上下文菜单
			menu.exec(viewport()->mapToGlobal(position));
		}
	}
}

// 当项目状态改变时触发的槽函数
void MyTreeView::onItemChanged(QStandardItem* item) {
	MyItem* pItem = static_cast<MyItem*>(item);

	// 如果项目关联了一个矢量图层
	if (pItem->getVectorLayer()) {
		// 如果项目未选中，则从场景中移除图层的图形项
		if (pItem->checkState() == Qt::CheckState::Unchecked) {
			for (auto graphicsItem : pItem->getVectorLayer()->getGraphicsItem()) {
				if (graphicsItem) {
					mpScene->removeItem(graphicsItem);
				}
			}
			pItem->getVectorLayer()->setVisible(false);
		}
		// 如果项目选中，则将图层的图形项添加到场景中
		else if (pItem->checkState() == Qt::CheckState::Checked) {
			for (auto graphicsItem : pItem->getVectorLayer()->getGraphicsItem()) {
				if (graphicsItem) {
					mpScene->addItem(graphicsItem);
				}
			}
			pItem->getVectorLayer()->setVisible(true);
		}
	}
	// 如果项目关联了一个栅格图层
	else if (pItem->getRasterLayer()) {
		// 如果项目未选中，则从场景中移除栅格数据
		if (pItem->checkState() == Qt::CheckState::Unchecked) {
			mpScene->removeItem(pItem->getRasterLayer()->getRasterData());
			pItem->getRasterLayer()->setVisible(false);
		}
		// 如果项目选中，则将栅格数据添加到场景中
		else if (pItem->checkState() == Qt::CheckState::Checked) {
			mpScene->addItem(pItem->getRasterLayer()->getRasterData());
			pItem->getRasterLayer()->setVisible(true);
		}
	}
}

// 执行“向上”操作的槽函数
void MyTreeView::performAction1() {
	if (mpChoosedItem) {
		// 如果项目关联了一个矢量图层，则将该图层向上移动
		if (mpChoosedItem->getVectorLayer()) {
			mpManager->upVectorLayer(mpChoosedItem->getVectorLayer());
		}
		// 如果项目关联了一个栅格图层，则将该图层向上移动
		else if (mpChoosedItem->getRasterLayer()) {
			mpManager->upRasterLayer(mpChoosedItem->getRasterLayer());
		}
	}
}

// 执行“向下”操作的槽函数
void MyTreeView::performAction2() {
	if (mpChoosedItem) {
		// 如果项目关联了一个矢量图层，则将该图层向下移动
		if (mpChoosedItem->getVectorLayer()) {
			mpManager->downVectorLayer(mpChoosedItem->getVectorLayer());
		}
		// 如果项目关联了一个栅格图层，则将该图层向下移动
		else if (mpChoosedItem->getRasterLayer()) {
			mpManager->downRasterLayer(mpChoosedItem->getRasterLayer());
		}
	}
}


void MyTreeView::performAction3() {
    if (mpChoosedItem) {
        // 检查并删除与 mpChoosedItem 关联的图层
        if (mpChoosedItem->getVectorLayer()) {
            mpManager->deleteVectorLayer(mpChoosedItem->getVectorLayer());
            mpChoosedItem->setVectorLayer(nullptr); // 断开关联
        }
        else if (mpChoosedItem->getRasterLayer()) {
            mpManager->deleteRasterLayer(mpChoosedItem->getRasterLayer());
            mpChoosedItem->setRasterLayer(nullptr); // 断开关联
        }

        // 删除列表中的信息
        mpModel->removeRow(mpChoosedItem->row());
        mpChoosedItem = nullptr; // 确保指针不再指向已删除的内存
    }
}


// 执行“缩放至图层”操作的槽函数
void MyTreeView::performAction4() {
	// 确保有一个被选择的项目
	if (mpChoosedItem) {
		// 如果选中的项目关联了一个矢量图层
		if (mpChoosedItem->getVectorLayer()) {
			// 调用图层管理器的函数，将视图缩放至该矢量图层
			mpManager->moveToVectorLayer(mpChoosedItem->getVectorLayer());
		}
		// 如果选中的项目关联了一个栅格图层
		else if (mpChoosedItem->getRasterLayer()) {
			// 调用图层管理器的函数，将视图缩放至该栅格图层
			mpManager->moveToRasterLayer(mpChoosedItem->getRasterLayer());
		}
	}
}

// 执行“设置图层颜色”操作的槽函数
void MyTreeView::performAction5() {
	// 确保有一个被选择的项目
	if (mpChoosedItem) {
		// 如果选中的项目关联了一个矢量图层
		if (mpChoosedItem->getVectorLayer()) {
			// 打开颜色选择对话框，默认颜色为白色，标题为“选择颜色”
			QColor color = QColorDialog::getColor(Qt::white, nullptr, "选择颜色");

			// 如果用户选择了有效的颜色
			if (color.isValid()) {
				// 用选择的颜色初始化 QBrush 和 QPen
				QBrush brush(color);
				QPen pen(color);

				// 遍历与矢量图层关联的所有图形项，并应用新颜色
				for (QGraphicsItem* graphicsItem : mpChoosedItem->getVectorLayer()->getGraphicsItem()) {
					// 如果图形项是 QGraphicsEllipseItem，设置其填充颜色
					if (QGraphicsEllipseItem* pItem = dynamic_cast<QGraphicsEllipseItem*>(graphicsItem)) {
						pItem->setBrush(brush);
					}
					// 如果图形项是 QGraphicsPathItem，设置其边线颜色
					else if (QGraphicsPathItem* pItem = dynamic_cast<QGraphicsPathItem*>(graphicsItem)) {
						pItem->setPen(pen);
					}
					// 如果图形项是 QGraphicsPolygonItem，设置其填充颜色
					else if (QGraphicsPolygonItem* pItem = dynamic_cast<QGraphicsPolygonItem*>(graphicsItem)) {
						pItem->setBrush(brush);
					}
				}
			}
		}
	}
}



