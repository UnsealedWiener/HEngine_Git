#include <iostream>
#include<string>
#include<vector>
#include<filesystem>
#include"HFBXImporter.h"
#include"HSelfFormatReader.h"

void ShowInstruction()
{
	std::cout << "<how to use>" << "\n";
	std::cout << "model data : --model [fbxfiledirection]" << "\n";
	std::cout << "animation data : --animation [fbxfiledirection]" << "\n";
	std::cout << "search by folder : --folder --model or --animation [fbxfilesfolder]" << "\n";
	std::cout << "****file direction should be english, due to limit of fbx sdk.****" << "\n";
}

int main(_In_ int argc, _In_z_count_(argc) char* argv[])
{
	std::vector<std::string> allArgs;

	if (argc > 1)
	{
		allArgs.assign(argv + 1, argv + argc);
	}

	//int x = 0;

	//for (const auto& entry : std::filesystem::directory_iterator("E:\\FinalProjectSVN\\5_Project\\HEngine\\HFbxImporter"))
	//{

	//	const auto& path = entry.path().filename().string();
	//	std::string
	//		//std::string path_ = path;
	//		x++;





	//}

	//HFbxImporter* pHFbxImporter = new HFbxImporter;


	//pHFbxImporter->ImportModel("E:\\FinalProjectSVN\\5_Project\\HEngine\\HFbxImporter\\Cliff1.fbx");
	//pHFbxImporter->WriteHModelFile();




	if (allArgs.size() != 2 && allArgs.size() != 3)
	{
		ShowInstruction();
		return -1;
	}
	else if (allArgs.size() == 2)
	{
		/*std::vector<size_t> backSlashPosition;

		for (std::size_t pos = 0; pos != std::string::npos;)
		{
			if (pos != 0)
			{
				pos++;
			}

			pos = allArgs[1].find("\\", pos);
			if (pos != std::string::npos)
			{
				backSlashPosition.push_back(pos);
			}
		}

		if (backSlashPosition.size() != 0)
		{
			for (int i = 0; i < backSlashPosition.size(); i++)
			{
				allArgs[1].replace(backSlashPosition[i], 1, "/");
			}
		}*/

		HFbxImporter* pHFbxImporter = new HFbxImporter;

		if (allArgs[0] == "--model")
		{
			if (pHFbxImporter->ImportModel(allArgs[1].c_str()))
				pHFbxImporter->WriteHModelFile();
		}
		else if (allArgs[0] == "--animation")
		{
			if (pHFbxImporter->ImportAnimation(allArgs[1].c_str()))
				pHFbxImporter->WriteHAnimFile();

		}
		else
		{
			ShowInstruction();
			return -1;
		}

		delete pHFbxImporter;
	}
	else if (allArgs.size() == 3)
	{
		if (!(allArgs[0] == "--folder" && (allArgs[1] == "--model" || allArgs[1] == "--animation")))
		{
			ShowInstruction();
			return -1;
		}

		for (const auto& entry : std::filesystem::directory_iterator(allArgs[2]))
		{
			const auto& filePath = entry.path().string();

			if (filePath.substr(filePath.find_last_of(".") + 1) == "fbx" ||
				filePath.substr(filePath.find_last_of(".") + 1) == "FBX")
			{
				HFbxImporter* pHFbxImporter = new HFbxImporter;

				if (allArgs[1] == "--model")
				{
					if (pHFbxImporter->ImportModel(filePath.c_str()))
						pHFbxImporter->WriteHModelFile();
				}
				else if (allArgs[1] == "--animation")
				{
					if (pHFbxImporter->ImportAnimation(filePath.c_str()))
						pHFbxImporter->WriteHAnimFile();
				}

				delete pHFbxImporter;
			}
		}
	}
	return 0;
}

