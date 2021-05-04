#include <iostream>
#include<string>
#include<vector>
#include"HFBXImporter.h"
#include"HSelfFormatReader.h"

int main(_In_ int argc, _In_z_count_(argc) char* argv[])
{
    std::vector<std::string> allArgs;

    if (argc > 1)
    {
        allArgs.assign(argv + 1, argv + argc);
    }

    if (allArgs.size() != 2)
    {
        std::cout << "<how to use>" << "\n";
        std::cout << "model data : --model [fbxfiledirection]" << "\n";
        std::cout << "animation data : --animation [fbxfiledirection]" << "\n";
        std::cout << "****file direction should be english, due to limit of fbx sdk.****" << "\n";

        
        return -1;
    }
    else
    {
        std::vector<size_t> backSlashPosition;

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
        }

        HFbxImporter* pHFbxImporter = new HFbxImporter;

        if (allArgs[0] == "--model")
        {
           if(pHFbxImporter->ImportModel(allArgs[1].c_str()))
            pHFbxImporter->WriteHModelFile();
        }
        else if (allArgs[0] == "--animation")
        {
           if(pHFbxImporter->ImportAnimation(allArgs[1].c_str()))
            pHFbxImporter->WriteHAnimFile();

        }
        else
        {
            std::cout << "<how to use>" << "\n";
            std::cout << "model data     : --model [fbxfiledirection]" << "\n";
            std::cout << "animation data : --animation [fbxfiledirection]" << "\n";
            std::cout << "****file direction should be english, due to limit of fbx sdk.****" << "\n";

            return -1;
        }

        delete pHFbxImporter;
    }

    return 0;
}

