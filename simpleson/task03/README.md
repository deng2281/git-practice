# 任务三：固定工作负载与性能采集

本目录复用周作业二实际使用的 `main.cpp`、`profile_workload.h/.cpp`，一次程序执行包含六组实验的八项操作。输入准备、内容预校验、循环执行及业务对象析构均在测量范围内；循环内不打印，也不链接 AllocCounter。

完整的分析、八轮优化过程及历史数据见 [作业记录](../作业记录.md)。历史 `results`、中间构建目录、原生报告及软件安装包均未迁入本作业。

## 普通运行与 VTune

在 `simpleson` 目录执行：

```powershell
cmake -S . -B build/optimized -G "Visual Studio 17 2022" -A x64 -DSIMPLESON_VARIANT=optimized
cmake --build build/optimized --config Debug --target simpleson_profile_cpu --parallel 4
.\build\optimized\task03\Debug\simpleson_profile_cpu.exe 50
```

参数是整套操作的循环轮数，50 轮对应 400 项循环操作。程序会预校验结果并检查循环结果，打印的 `Elapsed wall seconds` 是墙钟时间，不能当作 VTune CPU Time。

VTune 使用带符号的优化构建，固定 50,000 轮：

```powershell
cmake --build build/optimized --config RelWithDebInfo --target simpleson_profile_cpu --parallel 4
$vtune = Join-Path ${env:ProgramFiles(x86)} 'Intel/oneAPI/vtune/latest/bin64/vtune.exe'
& $vtune -collect hotspots -knob sampling-mode=sw -knob enable-stack-collection=true -result-dir profile-results/optimized-vtune-01 -- .\build\optimized\task03\RelWithDebInfo\simpleson_profile_cpu.exe 50000
```

独立采集三次，使用不同结果目录，例如 `optimized-vtune-01/02/03`，取 CPU Time 中位数，截图包含 Summary 中的 CPU Time、Top Hotspots。VTune 需另行安装；结果目录由工具创建，不应覆盖已有采集。

## MTuner

MTuner 采集需要另外安装的 SDK。配置时把 `MTUNER_SDK_DIR` 替换成自己机器上实际的 `MTuner/SDK` 路径：

```powershell
cmake -S . -B build/optimized -G "Visual Studio 17 2022" -A x64 -DSIMPLESON_VARIANT=optimized -DPROFILE_WITH_MTUNER=ON -DMTUNER_SDK_DIR="D:/your-tools/MTuner/SDK"
cmake --build build/optimized --config Debug --target simpleson_profile_mtuner --parallel 4

# This program requires an existing capture directory whose full path is ASCII.
New-Item -ItemType Directory -Path 'D:/simpleson-captures/optimized-01'
.\build\optimized\task03\Debug\simpleson_profile_mtuner.exe 50 'D:/simpleson-captures/optimized-01'
```

`D:/your-tools/MTuner/SDK` 是需替换的示例路径。采集目录使用新的空目录，完整路径不能包含中文。用 MTuner 打开生成的 `.MTuner` 文件，清除时间选择和过滤后，截图保留完整时间线、堆内存峰值及分配次数。

## 前后对照原则

- 原始版使用 `-DSIMPLESON_VARIANT=original` 和 `build/original`；最终版使用 `optimized` 和 `build/optimized`。两者保持同一工具、配置、轮数和生命周期。
- MTuner 为 Debug、50 轮；VTune 为 RelWithDebInfo、50,000 轮。MTuner 统计存活堆，VTune 观察 CPU 热点，二者不能互相替代。
- 堆峰值不能除以轮数。累计申请字节也不是堆峰值；整套分配次数还包含准备和预校验，不能直接与任务二某一项的单次计数对应。
- 新运行用于复测原始版和最终版；记录中优化一至八的表格来自周作业二当时的采集。只提供三套源码对照，不虚构已经保存所有中间版本的独立工程。

新增采集结果保留本地。`.gitignore` 已排除 `build/`、`results/`、`measurements/`、`profile-results/` 等目录。
