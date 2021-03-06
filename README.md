# HEngine_Git
 
[KOR]

[최소 요구 사항]

빌드 요구사항 : 

Window 10 버전 : Window 10(20H1)

Windows SDK 버전 : 10.0.19041.0

레이트레이싱 효과 하드웨어 요구사항 : AMD Radeon RX 6000 series or Nvidia GeForce 10-, 20-, or 30-series

[간단한 설명]

**결과물만 확인하고 싶으시면 HEngine/Executable/ExampleGame.exe 만 실행시키면 됩니다.**

HEngine은 DirectX12에 기반한 3D 그래픽 엔진입니다.

이 엔진은 DirectXTK( https://github.com/microsoft/DirectXTK )와 FBX SDK에 종속성을 가집니다.
DirectXTK는 이미 저장소에 포함되어 있습니다. 하지만 FBX SDK는 스스로 받으셔야 합니다.(아래 자세히 설명)

fbx파일을 자체포맷(hmodel, hanim)으로 바꾸는 FBXImporter라는 콘솔 어플리케이션도 저장소에서 제공해줍니다.
HEngine으로 바로 fbx 파일을 로드 할 순 있지만 속도가 느리니 자체 포맷으로 변환 한 후 로드하는 것을 추천합니다.

[ExampleGame 빌드하는 법]

1. 이 주소( https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-1 )에서 "fbx sdk 2020.1 VS2017"를 다운받습니다.

2. 설치합니다.

3. 설치된 경로에서 "FBX SDK" 폴더를 찾아  HEngine\Dependency 경로로 넣어줍니다.( 기본 설치 경로는 C:\Program Files\Autodesk\FBX\FBX SDK\2020.1 입니다.)

4. HEngine.sln 을 열고 "ExampleGame" 프로젝트를 시작 프로젝트로 선택해서 빌드합니다.(프로젝트에 이미 종속성 세팅이 되어있습니다.)

(빌드된 HEngine은 정적 라이브러리로 생성되어 "Output"폴더에 저장됩니다.(ExampleGame이 이 폴더를 참조하도록 세팅했습니다.))

**만약 빌드하는데 문제가 있으면 HEngine/Executable/ExampleGame.exe로 결과물만 확인 할 수 있습니다.**

[HEngine을 자신의 게임에서 사용하는 법]

HEngine은 정적 라이브러리로 빌드됩니다. 아래 설명대로 사용합니다.
간단한 사용예시와 리소스는 ExampleGame이라는 프로젝트에 있으니 참고하시면 됩니다.

1. HEngine 프로젝트를 빌드하면 Output 폴더에 헤더파일과 정적 라이브러리가 생성됩니다.

2.  Output/HEngine/staticlibrary 경로에 있는 헤더 파일들을 게임 프로젝트에 추가합니다.

3. 만약 DirectXTK의 마우스, 키보드, 게임패드를 사용하기 원하신다면 "Dependency\DirectXTK\DirectXTK12-sept2020\Inc"폴더에 있는 헤더 역시 프로젝트에 추가해줍니다.

4. 추가 종속성에 HEngine.lib를 추가해줍니다.

5. 모든 자세한 사용 방법은 EngineInterface.h에 적혀있습니다.

[HFbxImporter 사용하는 법]

HFbxImporter는 콘솔 어플리케이션입니다.

현재 3D Max에서 생성된 fbx 파일만 테스트 해봤습니다. 다른 그래픽 툴(예 : 블렌더, 마야 등)에서 생성된 fbx파일은 못 읽을 수도 있습니다.

1. 생성된 exe파일을 cmd에서 실행시킵니다.

2. 자세한 설명이 뜰텐데 참고해서 사용하시면 됩니다.

-------------------------------------------------------------------------------------------------------------------------------------------------

[ENG]

[Requirements]

Requirements for Build  : 

Window 10 version : Window 10(20H1)

Windows SDK version : 10.0.19041.0

Hardware requirement for raytracing effect  : AMD Radeon RX 6000 series or Nvidia GeForce 10-, 20-, or 30-series

[Instruction]

**If you want to skip everyting and just see the output, run HEngine/Executable/ExampleGame.exe.**

 The HEngine is 3D graphic Engine based on DirectX12.

 It has dependency on DirectXTK( https://github.com/microsoft/DirectXTK ) and FBX SDK.
 DirectXTK is already included in this repo but you have to download FBX SDK yourself to build this Engine.(details explained below)
 
 I also provided FBX Importer which changes fbx file into self format(hmodel, hanim). HEngine can directly load fbx files but it is slow
 so i recommend using my self format when you use this engine.
 
[how to build Example Game]

1. Download "fbx sdk 2020.1 VS2017" from this URL 
( https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-1 )

2. Install fbx sdk

3. Move the folder named "FBX SDK" from installed address to HEngine\Dependency
(default installation address is C:\Program Files\Autodesk\FBX\FBX SDK\2020.1)

4. Open HEngine.sln and select the "ExampleGame" as start project and build.(Dependency setting is already done.)

(The Built "HEngine" is created as static library in "Output" folder.("ExampleGame" is referencing this address.))

**If you have problem with building the project, you can just check result with HEngine/Executable/Example.exe**

[how to use HEngine in your game]

The HEngine is built as static library. In order to use it, you have to follow instruction below. 
Simple example of a game and using HEngine and resources are in ExampleGame project.

1. Build "HEngine" project. This will create header files and static library in "Output" folder.

2. Add the header files created in Output/HEngine/staticlibrary to your game project.

3. If you want to use DirectXTK mouse, keyboard and gamepad, add headers in "Dependency\DirectXTK\DirectXTK12-sept2020\Inc" to your game project as well.

4. Add HEngine.lib as addtional dependency.

5. Every usage of the HEngine should be written in EngineInterface.h.

[how to use HFbxImporter]

HFbxImporter is built as console application. 

Curruntly i have tested fbx files only from 3D Max. 
So fbx files from other graphic tool(ex. Blender, Maya) can goes wrong with HFbxImporter.

1. Run .exe file created in Output/HFbxImporter/exe/Release/HFbxImporter.exe in cmd.

2. you will see detailed instruction.






