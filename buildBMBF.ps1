# Builds a .qmod file for loading with BMBF
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt
$Name = "sliceVisualizer_v0.1.0"

$buildScript = "$NDKPath/build/ndk-build"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $buildScript += ".cmd"
}

& $buildScript NDK_PROJECT_PATH=$PSScriptRoot APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk NDK_APPLICATION_MK=$PSScriptRoot/Application.mk
Compress-Archive -Path "./libs/arm64-v8a/libslicevisualizer.so","./mod.json","./libs/arm64-v8a/libbeatsaber-hook_2_2_4.so" -DestinationPath "./$Name.zip" -Update
$FileName = "./$Name.qmod"
if(Test-Path $FileName) {
  Remove-Item $FileName
}
Rename-Item -Path "./$Name.zip" "$Name.qmod"