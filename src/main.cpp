//by ydog01. -- 2026/2/6
#include "Demaker.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <string>
#include <sstream>

namespace fs = std::filesystem;

void waitForCompleteFile(const fs::path& filePath)
{
    while (!fs::exists(filePath))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void processManiaMaps(bool enableAlign, int rangeMs)
{
    Demaker demaker;
    demaker.setAlignParams(enableAlign, rangeMs);
    
    std::vector<fs::path> filesToProcess;
    
    for (const auto& entry : fs::recursive_directory_iterator("tmp"))
    {
        if (entry.path().extension() == ".osu")
            filesToProcess.push_back(entry.path());
    }
    
    for (const auto& filePath : filesToProcess)
    {
        try
        {
            Osum chart;
            chart.read(filePath.string());
            
            std::string& metadata = chart.getSection("Metadata");
            if (!metadata.empty())
            {
                std::istringstream iss(metadata);
                std::ostringstream oss;
                std::string line;
                bool hasArtistUnicode(false);
                bool hasArtist(false);
                bool hasCreator(false);
                bool hasBeatmapID(false);
                bool hasBeatmapSetID(false);
                bool hasTags(false);
                
                while (std::getline(iss, line))
                {
                    if (line.find("Artist:") == 0)
                    {
                        hasArtist = true;
                        if (line.find("[changed by demaker]") == std::string::npos)
                            line += " [changed by demaker]";
                    }
                    else if (line.find("ArtistUnicode:") == 0)
                    {
                        hasArtistUnicode = true;
                        if (line.find("[changed by demaker]") == std::string::npos)
                            line += " [changed by demaker]";
                    }
                    else if (line.find("Creator:") == 0)
                    {
                        hasCreator = true;
                        if (line.find("[changed by demaker]") == std::string::npos)
                            line += " [changed by demaker]";
                    }
                    else if (line.find("BeatmapID:") == 0)
                    {
                        hasBeatmapID = true;
                        line = "BeatmapID:0";
                    }
                    else if (line.find("BeatmapSetID:") == 0)
                    {
                        hasBeatmapSetID = true;
                        line = "BeatmapSetID:-1";
                    }
                    else if (line.find("Tags:") == 0)
                    {
                        hasTags = true;
                        line = "Tags:";
                    }
                    
                    oss << line;
                    if (!iss.eof())
                        oss << "\n";
                }
                
                std::string contentStr = oss.str();
                
                if (!hasArtist)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "Artist:\n";
                }
                
                if (!hasArtistUnicode)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "ArtistUnicode:\n";
                }
                
                if (!hasCreator)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "Creator: Demaker\n";
                }
                
                if (!hasBeatmapID)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "BeatmapID:0\n";
                }
                
                if (!hasBeatmapSetID)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "BeatmapSetID:-1\n";
                }
                
                if (!hasTags)
                {
                    if (!contentStr.empty() && contentStr.back() != '\n')
                        contentStr += "\n";
                    contentStr += "Tags:\n";
                }
                
                metadata = contentStr;
            }
            
            demaker.process(chart);
            
            std::string prefix("[changed by demaker] ");
            fs::path newFileName(filePath.parent_path() / (prefix + filePath.filename().string()));
            
            chart.write(newFileName.string());
            
            fs::remove(filePath);
            
            std::cout << "Processed: " << filePath.filename().string() 
                      << " -> " << newFileName.filename().string() << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error processing " << filePath.filename().string()
                     << ": " << e.what() << std::endl;
        }
    }
}

int main()
{
    try
    {
        fs::remove("complete");
        
        std::string inputPath;
        std::cout << "Enter osz file path: ";
        std::getline(std::cin, inputPath);
        
        if (!fs::exists(inputPath))
            throw std::runtime_error("Input file does not exist: " + inputPath);
        
        bool enableAlign = false;
        int rangeMs = 0;
        
        std::cout << "Enable note alignment? (y/n): ";
        std::string alignInput;
        std::getline(std::cin, alignInput);
        
        if (alignInput == "y" || alignInput == "Y")
        {
            enableAlign = true;
            std::cout << "Enter alignment range (ms): ";
            std::string rangeInput;
            std::getline(std::cin, rangeInput);
            
            try
            {
                rangeMs = std::stoi(rangeInput);
                if (rangeMs < 0)
                {
                    std::cout << "Range cannot be negative. Using absolute value." << std::endl;
                    rangeMs = std::abs(rangeMs);
                }
            }
            catch (...)
            {
                std::cout << "Invalid range. Using default value: 5ms" << std::endl;
                rangeMs = 5;
            }
        }
        
        std::ofstream paramsFile("params");
        if (!paramsFile)
            throw std::runtime_error("Cannot create params file");
        paramsFile << inputPath;
        paramsFile.close();
        
        if (!fs::exists("decoder.exe"))
            throw std::runtime_error("decoder.exe not found");
        
        int decoderResult = std::system("decoder.exe");
        if (decoderResult != 0)
            throw std::runtime_error("decoder.exe failed with code: " + std::to_string(decoderResult));
        
        waitForCompleteFile("complete");
        fs::remove("complete");
        
        if (!fs::exists("tmp") || fs::is_empty("tmp"))
            throw std::runtime_error("tmp folder is empty or does not exist");
        
        processManiaMaps(enableAlign, rangeMs);
        
        fs::path inputFilePath(inputPath);
        fs::path outputPath = inputFilePath.parent_path() / ("[demaker] " + inputFilePath.filename().string());
        
        std::ofstream paramsFileOut("params");
        if (!paramsFileOut)
            throw std::runtime_error("Cannot modify params file");
        paramsFileOut << outputPath.string();
        paramsFileOut.close();
        
        if (!fs::exists("encoder.exe"))
            throw std::runtime_error("encoder.exe not found");
        
        int encoderResult = std::system("encoder.exe");
        if (encoderResult != 0)
            throw std::runtime_error("encoder.exe failed with code: " + std::to_string(encoderResult));
        
        std::cout << "Processing completed successfully!" << std::endl;
        std::cout << "Output file: " << outputPath.string() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        
        try
        {
            if (fs::exists("params")) fs::remove("params");
            if (fs::exists("complete")) fs::remove("complete");
        }
        catch (...)
        {
        }
    }

    system("pause");
    
    return 0;
}