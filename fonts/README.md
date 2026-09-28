# Fonts

项目烘焙两套 14 号 CJK 字体，以及一套仅含数字和冒号的主页面时钟字体：

- `cjk_sc_14`：Source Han Sans CN，项目默认字体。
- `cjk_jp_14`：Noto Sans JP，检测到日文假名时使用。
- `home_time_44`：仅用于 Editor 中预览大号时间；固件运行时会优先替换为 TinyTTF。

两份运行时字体描述符都把 LVGL 内建的 `lv_font_montserrat_14` 设置为
`fallback`，用于 ASCII、数字和 `LV_SYMBOL_*` 等缺失字形。不再单独烘焙
Montserrat 或 FontAwesome 图标字体。

## TinyTTF

固件启用了 TinyTTF 文件加载。运行时字体统一使用：

- SD 卡文件：`/fonts/SourceHanSansCN-Regular.ttf`
- LVGL 路径：`C:/fonts/SourceHanSansCN-Regular.ttf`
- PC 模拟器：`A:fonts/SourceHanSansCN-Regular.ttf`

当前仅主页面的时间、日期和 Wi-Fi 状态文字使用 TinyTTF。播放器歌词、文件列表等
高频刷新区域继续使用烘焙字体。字体文件缺失或加载失败时，控件会保留 XML 中设置
的烘焙字体作为回退，不影响界面启动。

Wi-Fi 图标使用嵌入式 PNG，不依赖思源黑体或 FontAwesome 私有区字形，以保证
LVGL Editor 预览与固件显示一致。
