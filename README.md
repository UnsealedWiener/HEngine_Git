# HEngine_Git
 
 The HEngine is 3D graphic Engine based on DirectX12.

 It has dependency on DirectXTK(https://github.com/microsoft/DirectXTK) and FBX SDK.
 DirectXTK is already included in this repo but you have to download FBX SDK yourself to build this Engine.(details explained below)
 
 I also provided FBX Importer which changes fbx file into self format(hmodel, hanim). HEngine can directly load fbx files but it is slow
 so i recommend using my self format when you use this engine.
 
[how to build HEngine and HFbxImporter]

1. Download "fbx sdk 2020.1 VS2017" from this URL 
(https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-1)

2. Install fbx sdk

3. Move the folder named "FBX SDK" from installed address to HEngine\Dependency
(default installation address is C:\Program Files\Autodesk\FBX\FBX SDK\2020.1)

4. Open HEngine.sln and build.

5. The result will be created in "Output" folder.

[how to use HEngine in your game]

The HEngine is built as static library. In order to use it, you have to follow instruction below. 

1. Add the header files created in Output/HEngine/staticlibrary to your game project.

2. If you want to use DirectXTK mouse, keyboard and gamepad, add headers in "DirectXTKHeaders" to your game project as well.

2. Add HEngine.lib as addtional dependency.

3. Every usage of the HEngine should be written in EngineInterface.h.

[how to use HFbxImporter]

HFbxImporter is built as console application. 

Curruntly i have tested fbx files only from 3D Max. 
So fbx files from other graphic tool(ex. Blender, Maya) can goes wrong with HFbxImporter.

1. Run .exe file created in Output/HFbxImporter/exe/Release/HFbxImporter.exe in cmd.

2. Press enter and you will see detailed instruction.






