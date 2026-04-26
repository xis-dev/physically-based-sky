#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

class PathResolver {
public:
    static std::string getPath(const std::string& relativePath)
    {
        if (std::filesystem::exists(std::filesystem::path(relativePath)))
        {
            return relativePath;
        }

        std::vector<std::string> basePaths{
            ".",
            "..",
            "../..",
            "../../..",
            "../../../..",
            "../../../../..",
            "../../../../../..",
            "../../../../../../../..",
            "../../../../../../../../..",
            "../../../../../../../../../..",
            "../../../../../../../../../../..",


        };

        for (auto& base : basePaths)
        {
            if (auto filePath = base / std::filesystem::path(relativePath); std::filesystem::exists(filePath))
            {
                return filePath.string();
            }
        }


        std::cout << "FILEMANAGER:: Failed to locate file at: " << relativePath << ". File may be nested too deep. \n";
        std::cout << "FILEMANAGER:: Current path: " << std::filesystem::current_path() << '\n';

        return relativePath;
    }


    static std::string getAbsolutePath(const std::string& relativePath)
    {
        std::string absolute = getPath(relativePath);
        auto absolutePath = std::filesystem::absolute(absolute);

        if (std::filesystem::exists(absolutePath))
        {
            return absolutePath.string();
        }

        std::cout << "FILEMANAGER:: Cannot locate absolute path for: " << relativePath << '\n';
        return relativePath;
    }


    static bool fileExists(const std::string& path)
    {
        const std::ifstream file(path);

        return file.good();
    }

};