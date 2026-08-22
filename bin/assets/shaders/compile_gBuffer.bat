@echo off
set SCRIPT_DIR=%~dp0

"C:\VulkanSDK\1.4.350.0\Bin\slangc.exe" "%SCRIPT_DIR%gbuffer.slang" ^
-target spirv ^
-profile spirv_1_4 ^
-emit-spirv-directly ^
-fvk-use-entrypoint-name ^
-entry vertMain ^
-entry fragMain ^
-o "%SCRIPT_DIR%gbuffer.spv"

pause