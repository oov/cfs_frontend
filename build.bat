call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsMSBuildCmd.bat"
cd %~dp0
MSBuild cfs_frontend.sln /t:clean;rebuild /p:Configuration=Release;Platform="x86"
