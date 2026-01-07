// WallpRandomTest01.cpp : This file contains the 'main' function. Program execution begins and ends there.
//


#include <windows.h>
#include <shobjidl.h> // For IDesktopWallpaper
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

std::vector<std::wstring> GetFilesPathsFromFolder(const std::wstring& folderPath) {
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
std::vector<std::wstring> GetRandomWallpapers(const std::wstring& folderPath, const UINT& count) {
	// Vector to hold wallpaper file paths
    std::vector<std::wstring> allWallpapers;

	// Retrieve wallpaper file paths from the specified folder
	allWallpapers = GetFilesPathsFromFolder(folderPath);

    std::vector<std::wstring> selectedWallpapers;
    for (UINT i = 0; i < count; i++) {
        // Randomly select a wallpaper from the list
        std::random_device rd; // non-deterministic random number generator
        std::mt19937 gen(rd()); // Mersenne Twister random number generator initialized with random device
        // Use size_t for the distribution bounds to avoid narrowing conversion warnings (C4267)
        std::size_t maxIndex = allWallpapers.size() - 1;
        // Uniform distribution to select an index within the range of available Wallpapers
        std::uniform_int_distribution<std::size_t> dist(0, maxIndex);

		selectedWallpapers.push_back(allWallpapers[dist(gen)]); // Add the randomly selected wallpaper to the list
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
					std::wstring wallpaperPath = wallpapers[i]; // Get a random wallpaper
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


