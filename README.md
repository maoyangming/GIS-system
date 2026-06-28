# GIS-system

基于 Qt Widgets、GDAL/OGR 和 QGraphicsView 的桌面地理信息系统实验项目。项目支持矢量与栅格数据加载、图层管理、地图浏览、属性查看、编辑保存，以及常见空间分析和栅格处理功能。

## 功能

- 加载 Shapefile、GeoJSON、CSV 等矢量数据，并转换到 WGS84 显示。
- 加载 TIFF、IMG、PNG、JPG 等栅格数据，支持普通栅格和大栅格读取。
- 在图层树中管理图层显隐、顺序、定位和删除。
- 支持地图平移、缩放、旋转、选择和属性表查看。
- 支持点、线、面要素编辑，并将编辑结果写回 Shapefile。
- 支持矢量分析：缓冲区、凸包、要素数量/面积/长度统计。
- 支持栅格分析：多波段真彩色合成、直方图与均衡化、掩膜提取、邻域统计。
- 支持将当前矢量图层导出为文本文件或 SQLite 数据库。
- 使用 log4cpp 输出运行日志。

## 目录结构

```text
GIS-system-main/
├── main.cpp                         # Qt 程序入口
├── MyGIS.*                          # 主窗口、菜单动作、图层导入与分析入口
├── MyLayer.h                        # 矢量/栅格图层数据结构
├── MyLayerManager.h                 # 图层树与图层显示管理
├── MyGraphicsView.h                 # 地图视图交互：缩放、平移、编辑模式
├── MyGraphicsItem.h                 # 可编辑点、线、面图形项
├── ConvexHull.*                     # 凸包分析
├── Buffer.*                         # 缓冲区分析
├── Statistic.*                      # 矢量统计
├── TFColorDisplay.*                 # 多波段合成显示
├── Histogram.*                      # 栅格直方图与均衡化
├── Mask.*                           # 矢量/栅格掩膜处理
├── NeighborhoodStatistics.*         # 栅格邻域统计
├── Storage.h / textStorage.*        # 文本导出
└── dataStorage.* / vectorStorage.h  # SQLite 导出接口
```

## 环境要求

- Windows 开发环境
- Qt Widgets
- GDAL/OGR
- PROJ
- OpenMP
- SQLite3
- log4cpp

源码中包含 `Windows.h`，并引用了 `ui_*.h` 生成头文件，通常需要配合 Qt Designer 的 `.ui` 文件、Visual Studio Qt 插件或 Qt 构建流程使用。

## 构建与运行

仓库中没有看到 `.pro`、`CMakeLists.txt` 或 Visual Studio 工程文件，因此需要在本地工程中加入这些 `.cpp`、`.h` 文件，并配置 Qt、GDAL、PROJ、SQLite3、log4cpp、OpenMP 的头文件与库路径。

运行后可通过菜单加载矢量或栅格图层，再使用图层栏和分析工具进行处理。部分分析结果会生成新的 Shapefile 或 GeoTIFF，并自动加载回主窗口。

## 注意事项

- 当前源码中部分中文注释存在编码乱码，但不影响主要逻辑判断。
- 编辑保存逻辑会尝试写回原 Shapefile，使用前建议备份数据。
- 部分功能依赖 `ui_*.h`，如果缺少对应 `.ui` 文件，需要从原 Qt 工程恢复。
- 日志默认写入 `Log.log`。
