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

// Function wide string to get a random wallpaper from the specified folder path
    /*The argmument requested as [const std::wstring& folderpath] meas that a 
    wide string is expected and turns it into a pointer or reference */
static std::vector<std::wstring> GetRandomWallpapers(const std::wstring& folderPath, const UINT& count) {
	// Get all wallpaper file paths from the specified folder
    std::vector<std::wstring> allWallpapers = GetFilesPathsFromFolder(folderPath);

	std::string pathLog = "wallpaper_paths_log.txt"; // Log file to store wallpaper paths

	std::vector<std::wstring> historyLog = GetHistoyLogFromFile(pathLog); // Retrieve history log from file
	UINT maxLinesInLog = allWallpapers.size(); // Maximum number of lines to keep in the history log (equal to total wallpapers)
    if (historyLog.size() > maxLinesInLog) {
		std::wofstream outfile(pathLog, std::ios::trunc); // Open the log file in truncate mode to clear it
        if (outfile.is_open()) {
            outfile.close(); // Close the file after truncating
        } else {
            std::cerr << "Unable to open log file for truncating: " << pathLog << std::endl; // Error message if log file cannot be opened
        }
		historyLog.clear(); // Clear the in-memory history log
	}

    if (allWallpapers.size() <= count) {
		return allWallpapers; // If available wallpapers are less than or equal to count, return all
	}

	std::vector<std::wstring> selectedWallpapers; // Vector to hold selected random wallpapers

	bool success = false; // Flag to indicate successful selection of wallpaper

	// Select random wallpapers based on the count of monitors
    while (!success) {
        for (UINT i = 0; i < count; i++) {
            // Randomly select a wallpaper from the list
            std::random_device rd; // non-deterministic random number generator
            std::mt19937 gen(rd()); // Mersenne Twister random number generator initialized with random device
            // Use size_t for the distribution bounds to avoid narrowing conversion warnings (C4267)
            std::size_t maxIndex = allWallpapers.size() - 1;
            // Uniform distribution to select an index within the range of available Wallpapers
            std::uniform_int_distribution<std::size_t> dist(0, maxIndex);
			// Iterate through the history log in reverse order
			for (auto it = historyLog.rbegin(); it != historyLog.rend(); ++it) {
                // Check if the randomly selected wallpaper is in the history log
                if (*it == allWallpapers[dist(gen)]) {
                    break; // If found in history, break to select a new wallpaper
                }
                // If reached the end of history log without finding a match, selection is successful
                if (it + 1 == historyLog.rend()) {
                    success = true;
                }
            }
            selectedWallpapers.push_back(allWallpapers[dist(gen)]); // Add the randomly selected wallpaper to the list
        }
    }
	// If no wallpapers were selected, throw an error
    if (selectedWallpapers.empty()) {
        throw std::runtime_error("No wallpapers selected in folder.");
	}

	std::wofstream outFile(pathLog, std::ios::app); // Open the log file for writing
	if (outFile.is_open()) {
        for (const auto& wallpaperPath : selectedWallpapers) {
            outFile << wallpaperPath << std::endl; // Write each selected wallpaper path to the log file
        }
		outFile.close(); // Close the log file
        } else {
        std::cerr << "Unable to open log file: " << pathLog << std::endl; // Error message if log file cannot be opened
	}
    return selectedWallpapers;
}

int main() {
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

		std::vector<std::wstring> wallpapers = GetRandomWallpapers(folderPath, count); // Get random wallpapers for each monitor

		// Iterate through each monitor
        for (UINT i = 0; i < count; i++) {
            LPWSTR monitorId = nullptr; // Ensure monitorId is initialized
			HRESULT hrMonitor = pWallpaper->GetMonitorDevicePathAt(i, &monitorId); // Get the monitor ID
            
			// Check if the monitor ID retrieval was successful
            if (SUCCEEDED(hrMonitor) && monitorId != nullptr) {
                // Assign a random wallpaper from the folder
                try {
					std::wstring wallpaperPath = wallpapers[i]; // Get the wallpaper path for the current monitor
					pWallpaper->SetWallpaper(monitorId, wallpaperPath.c_str()); // Set the wallpaper for the monitor
					std::wcout << L"Monitor " << i << L": " << wallpaperPath << std::endl; // Output the assigned wallpaper path
                }
				// Handle any errors that occur during wallpaper assignment
                catch (const std::exception& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                }
				// Free the allocated memory for monitorId
                CoTaskMemFree(monitorId);
            }
        }
		// Release the IDesktopWallpaper instance
        pWallpaper->Release();
    }
    else {
		std::cerr << "Failed to initialize IDesktopWallpaper." << std::endl; // Error message if instance creation fails
    }

	CoUninitialize(); // Uninitialize COM library
    return 0;
}


