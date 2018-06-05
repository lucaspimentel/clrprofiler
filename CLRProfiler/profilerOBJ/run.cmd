@echo off
SET COR_ENABLE_PROFILING=1
SET COR_PROFILER={846F5F1C-F9AE-4B07-969E-05C26BC060D8}
SET COR_PROFILER_PATH=C:\Users\lucas\source\repos\dd-trace-csharp\src\Datadog.Trace.ClrProfiler\x64\Debug\Datadog.Trace.ClrProfiler.dll
rem SET COR_PROFILER_PATH=C:\Users\lucas\source\repos\dd-trace-csharp\src\Datadog.Trace.ClrProfiler\Win32\Debug\Datadog.Trace.ClrProfiler.dll

"C:\Users\lucas\source\repos\clrprofiler\ILRewrite\SampleApp\bin\x64\Debug\SampleApp.exe"