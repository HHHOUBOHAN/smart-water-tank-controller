# Flash优化说明

## 已实施

1. Debug由 `-O0` 改为 `-Og`：保留断点调试能力，同时减小代码。
2. Release使用 `-Os` 并启用LTO：面向最终烧录体积优化。
3. 保留 `nano.specs`、函数/数据分段和 `--gc-sections`，继续移除未引用代码。
4. 用 `COMMON/text_parser` 替换AT、MQTT和JSON接收解析中的全部 `sscanf()`。
5. 发布标签缓存由4字节改为6字节，消除编译器截断警告。
6. 保留安全保护、网络重连、ACK、Flash参数区和OLED现有显示，不用删除功能换空间。

## 为什么先移除sscanf

`sscanf()`是通用格式解析器，会引入较大的C库代码。项目实际只需要解析十进制整数和逗号分隔字段，因此轻量解析器更适合STM32F103，并且能够显式检查32位溢出。

## 验证方法

在VS Code中依次构建：

```text
Debug
Release
```

记录两次构建分析中的FLASH和RAM。最终下降量必须以ARM-GCC生成的MAP/ELF为准。本工程还提供 `tests/test_protocol_parsers.c`，覆盖整数边界、AT、MQTT、命令和配置解析。

## 后续可选优化

如果Release仍超过85%，下一步按收益/风险排序：

1. 将发送端简单 `snprintf()` 改为定长字符串构建器；
2. 检查MAP文件中newlib格式化函数占用；
3. OLED只保留必要字体，评估取消11x18大字体；
4. 将诊断字符串按发布等级裁剪；
5. 最后才考虑调整Flash参数区，且必须先重做存储布局与掉电测试。

