# HEngine_Git
 
[how to build]

1. Download "fbx sdk 2020.1 VS2017" from this URL 
(https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-1)

2. Install fbx sdk

3. Move folder "FBX SDK" from installed address to HEngine\Dependency\
(default installation address is C:\Program Files\Autodesk\FBX\FBX SDK\2020.1)

4. Open HEngine.sln and build.

5. HEngine.lib is created in lib folder with header files.

[how to use this library in your game]

1. Add the header files to your project forlder.

2. Add HEngine.lib as addtional dependency.

3. Every usage of the HEngine should be written in EngineInterface.h.




