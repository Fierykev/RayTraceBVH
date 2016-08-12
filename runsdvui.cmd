cd /d "C:\Users\cyber\Documents\Visual Studio 2015\Projects\RayTraceBVH\RayTraceBVH" &msbuild "RayTraceBVH.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform=x64
exit %errorlevel% 