# Browser backend

`browser_backend.c/.h` 是不依赖具体屏幕和路由的文件浏览后端。

- 固定大小窗口，不为整个目录保存文件名。
- 前后窗口在目录首尾自动循环。
- `allow_directories` 控制是否显示目录。
- `extensions` 使用分号或逗号分隔，匹配时忽略大小写和开头的点。
- `filter_cb` 用于播放器格式检测等动态规则。
- 控制器上下文（窗口文件名、路径和菜单缓冲）在进入浏览器时通过
  `lv_malloc()` 动态申请，浏览器屏删除时通过 `lv_free()` 释放；固件中
  `lv_malloc()` 已映射到 FreeRTOS 动态堆。
- 普通文件双击打开文件菜单，长按仍执行全局返回动作。
- 目录读取失败、动态内存不足和根目录打开失败都会记录错误日志。

`browser_controller.c/.h` 负责连接 XML、统一焦点层和本后端。模块已通过
`user_config.cmake` 接入，原先位于 `H7_MediaPlayer_UI.c` 的浏览器实现已删除。
