// WallpRandomTest01.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <windows.h>
#include <shobjidl.h> // For IDesktopWallpaper
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <random>
#include <fstream>

namespace fs = std::filesystem;

static int writeLogFile(const std::string& filename, const std::vector<std::wstring>& logEntries) {
    std::wofstream outFile(filename, std::ios::app);
    if (outFile.is_open()) {
        for (const auto& entry : logEntries) {
            outFile << entry << std::endl;
        }
        outFile.close();
        return 0; // Success
    } else {
        std::cerr << "Unable to open log file for writing: " << filename << std::endl;
        return -1; // Failure
    }
}

static int clearLogFile(const std::string& filename) {
    std::ofstream outFile(filename, std::ios::trunc);
    if (outFile.is_open()) {
        outFile.close();
        return 0; // Success
		std::cout << "Log file cleared: " << filename << std::endl;
    } else {
        std::cerr << "Unable to open log file for clearing: " << filename << std::endl;
        return -1; // Failure
    }
}

static std::vector<std::wstring> RandomizeWallpapers(const std::vector<std::wstring>& availableWallpapers,  const UINT& count) {
	std::vector<std::wstring> selectedWallpapers;
    for (UINT i = 0; i < count; i++) {
        std::random_device rd; // non-deterministic random number generator
        std::mt19937 gen(rd()); // Mersenne Twister random number generator initialized with random device
        std::size_t maxIndex = availableWallpapers.size() - 1;
        // Uniform distribution to select an index within the range of available Wallpapers
        std::uniform_int_distribution<std::size_t> dist(0, maxIndex);
        selectedWallpapers.push_back(availableWallpapers[dist(gen)]); // Add the randomly selected wallpaper to the list
    }
	return selectedWallpapers;
}

static std::vector<std::wstring> GetAvailableWallpapers(const std::vector<std::wstring>& allWallpapers, const std::vector<std::wstring>& historyLog) {
	std::vector<std::wstring> availableWallpapers;
    for (const auto& path : allWallpapers) {
        bool inHistory = false;
        for (const auto& h : historyLog) {
            if (path == h) {
                inHistory = true;
                break;
            }
        }
        if (!inHistory) {
            availableWallpapers.push_back(path);
        }
    }
	return availableWallpapers;
}

static std::vector<std::wstring>GetHistoyLogFromFile(const std::string& filename) {
    std::vector<std::wstring> historyLog;
    std::wifstream inFile(filename);
    if (inFile.is_open()) {
        std::wstring line;
        while (std::getline(inFile, line)) {
            historyLog.push_back(line);
        }
        inFile.close();
    } else {
        std::cerr << "Unable to open history log file: " << filename << std::endl;
    }
    return historyLog;
}

static std::vector<std::wstring> GetFilesPathsFromFolder(const std::wstring& folderPath) {
    std::vector<std::wstring> wallpapers;

    // Iterate through the directory and collect image files paths
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        // Check if the file is an image based on its extension
        if (entry.is_regular_file()) {
            // Get the file extension in wide string format
            auto ext = entry.path().extension().wstring();
            // Convert to lowercase for comparison, checking common image formats
            if (ext == L".jpg" || ext == L".jpeg" || ext == L".png" || ext == L".bmp") {
                wallpapers.push_back(entry.path().wstring()); // Add the wallpaper path to the list as a wide string
            }
        }
    }
    // If no wallpapers found, throw an error
    if (wallpapers.empty()) {
        throw std::runtime_error("No wallpapers found in folder.");
    }

    return wallpapers;
}

static std::vector<std::wstring> GetRandomWallpapers(const std::wstring& folderPath, const UINT& count, const std::string& pathLog) {
	// Get all wallpaper file paths from the specified folder
    std::vector<std::wstring> allWallpapers = GetFilesPathsFromFolder(folderPath);
    if (allWallpapers.size() <= count) {
        throw std::runtime_error("There are not enough wallpapers for all monitors in selected folder.");
    }

	std::vector<std::wstring> historyLog = GetHistoyLogFromFile(pathLog); // Retrieve history log from file
	std::size_t maxLinesInLog = allWallpapers.size(); // Maximum number of lines to keep in the history log (equal to total wallpapers)
    if (historyLog.size() > maxLinesInLog) {
		clearLogFile(pathLog); // Clear the log file if it exceeds maximum lines
		historyLog.clear(); // Clear the in-memory history log
	}

    // 1. Find all wallpapers that ARE NOT in history
    std::vector<std::wstring> availableWallpapers = GetAvailableWallpapers(allWallpapers,historyLog);

    // 2. Adjust count if we don't have enough remaining
    size_t finalCount = (std::min)(static_cast<size_t>(count), availableWallpapers.size());

    std::vector<std::wstring> selectedWallpapers; // Vector to hold selected random wallpapers

    // If the number of selected wallpapers doesn't match the monitor count, clear the log and reselect
    if (finalCount < count) {
        clearLogFile(pathLog); // Clear the log file if selected wallpapers count doesn't match monitor count
		selectedWallpapers = RandomizeWallpapers(allWallpapers, count); // Reselect wallpapers from all available ones
    } else {
        // 3. Randomly select wallpapers from the available ones
		selectedWallpapers = RandomizeWallpapers(availableWallpapers, count);
    }
    return selectedWallpapers;
}

int main() {

    std::string pathLog = "wallpaper_paths_log.txt"; // Log file to store wallpaper paths

	// Initialize COM library
    HRESULT hrc = CoInitialize(NULL);
    
	// Check if COM initialization was successful
    if (FAILED(hrc)) {
		// Output error message if COM initialization fails
        std::cerr << "COM initialization failed. HRESULT: " << std::hex << hrc << std::endl;
        return 1;
    }

	// Create an instance of IDesktopWallpaper
    IDesktopWallpaper* pWallpaper = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_DesktopWallpaper, NULL, CLSCTX_ALL, IID_IDesktopWallpaper, (void**)&pWallpaper);

	// Check if the instance creation was successful
    if (SUCCEEDED(hr)) {
		// Get the number of monitors
        UINT count = 0;
        pWallpaper->GetMonitorDevicePathCount(&count); 

        std::wstring folderPath = L"D:\\Imagenes\\Wallpapers\\IA_Wallpapers_pack_03"; // Specify the folder path containing wallpapers

		std::vector<std::wstring> wallpapers = GetRandomWallpapers(folderPath, count, pathLog); // Get random wallpapers for each monitor

		std::vector<std::wstring> usedWallpapers; // Vector to keep track of used wallpapers

		// Iterate through each monitor
        for (UINT i = 0; i < count; i++) {
            LPWSTR monitorId = nullptr; // Ensure monitorId is initialized
			HRESULT hrMonitor = pWallpaper->GetMonitorDevicePathAt(i, &monitorId); // Get the monitor ID
            
			// Check if the monitor ID retrieval was successful
            if (SUCCEEDED(hrMonitor) && monitorId != nullptr) {
				RECT monitorRect;
				if (SUCCEEDED(pWallpaper->GetMonitorRECT(monitorId, &monitorRect))) {
					int width = monitorRect.right - monitorRect.left;
					int height = monitorRect.bottom - monitorRect.top;

					if (width > 0 && height > 0) {
                        // Assign a random wallpaper from the folder
                            try {
                                std::wstring wallpaperPath = wallpapers[i]; // Get the wallpaper path for the current monitor
                                pWallpaper->SetWallpaper(monitorId, wallpaperPath.c_str()); // Set the wallpaper for the monitor
                                std::wcout << L"Monitor " << i << L": " << wallpaperPath << std::endl; // Output the assigned wallpaper path
								usedWallpapers.push_back(wallpaperPath); // Add the wallpaper path to the used wallpapers log
                            }
                            // Handle any errors that occur during wallpaper assignment
                            catch (const std::exception& e) {
                                std::cerr << "Error: " << e.what() << std::endl;
                            }

                    }

                }
				// Free the allocated memory for monitorId
                CoTaskMemFree(monitorId);
            }
        }
		// Release the IDesktopWallpaper instance
        pWallpaper->Release();
        writeLogFile(pathLog, usedWallpapers); // Write the used wallpapers to the log file
    }
    else {
		std::cerr << "Failed to initialize IDesktopWallpaper." << std::endl; // Error message if instance creation fails
    }

	CoUninitialize(); // Uninitialize COM library
    return 0;
}


