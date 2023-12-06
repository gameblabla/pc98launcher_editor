#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#include <stdio.h>

extern int fetch_and_download_screenshots(char* game_name, char* folder_name, char* destination_folder, int plid);
extern void downscale_png_to_bmp(const char* input, const char* output);
extern char* Get_Developer_Game();

extern char* Get_Year_Game();
extern char* Get_Genre_Game();

#define MAX_FOLDERS 32000
#define MAX_FOLDER_NAME_LENGTH 260
#define MAX_LAUNCHDAT_SIZE 1024

// Global variables
TCHAR folderNames[MAX_FOLDERS][MAX_FOLDER_NAME_LENGTH];
int folderCount = 0;
HWND hListBox, hEdit;
HWND hFileListBox;
HWND hwnd;

long platform_id_selected = 95;
// 106 Is Sharp X68000

TCHAR currentFolderPath[MAX_PATH];
TCHAR mainfolder[MAX_PATH];

// Function declarations
void ListDirectories(HWND hwnd, TCHAR *folderPath);
void AddFolderToListBox(HWND hwnd, TCHAR *folderName);
void LoadLaunchDat(HWND hwnd, TCHAR *folderName);
void SaveLaunchDat(HWND hwnd, TCHAR *folderName);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define ID_NAME_EDIT      101
#define ID_GENRE_EDIT     102
#define ID_YEAR_EDIT      103
#define ID_DEVELOPER_EDIT 104
#define ID_SERIES_EDIT    105
#define ID_IMAGES_EDIT    106
#define ID_START_EDIT     107
#define ID_ALT_START_EDIT 108
#define ID_MIDI_MPU_EDIT  109
#define ID_MIDI_SERIAL_EDIT 110

#define ID_DOWNLOAD_BUTTON 200

#define IDM_OPEN_FOLDER  301
#define IDM_EXIT         302
#define IDM_ABOUT        303
#define IDM_SAVE 304 // New define for Save menu item

typedef struct {
    int id;         // Control ID
    int x, y;       // Position
    int width, height; // Size
    char label[50];           // Label Text
    char real_label[50];           // From launch.dat
} EditControlInfo;

// Define the properties for each control
const EditControlInfo editControls[] = {
    {ID_NAME_EDIT, 600, 10, 200, 20, "Name", "name"},
    {ID_GENRE_EDIT, 600, 40, 200, 20, "Genre", "genre"},
    {ID_YEAR_EDIT, 600, 70, 200, 20, "Year", "year"},
    {ID_DEVELOPER_EDIT, 600, 100, 200, 20, "Developer", "developer"},
    {ID_SERIES_EDIT, 600, 130, 200, 20, "Series", "series"},
    {ID_IMAGES_EDIT, 600, 160, 200, 20, "Images", "images"},
    {ID_START_EDIT, 600, 190, 200, 20, "Start", "start"},
    {ID_ALT_START_EDIT, 600, 220, 200, 20, "Alt Start", "altstart"},
    {ID_MIDI_MPU_EDIT, 600, 250, 200, 20, "MIDI MPU", "midi_mpu"},
    {ID_MIDI_SERIAL_EDIT, 600, 280, 200, 20, "MIDI Serial", "midi_serial"}
};

const int editControlCount = sizeof(editControls) / sizeof(editControls[0]);

void CreateMenus(HWND hwnd);

HWND hDownloadWindow;

// Function to create and show the download window
void ShowDownloadWindow(HWND hwndParent) {
    hDownloadWindow = CreateWindowEx(
        0, 
        "Static", // Use "Static" for a simple message window. Or use a custom class if defined.
        "Downloading and converting images, please wait...",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 100, // Adjust size as needed
        hwndParent, NULL, GetModuleHandle(NULL), NULL
    );

    ShowWindow(hDownloadWindow, SW_SHOW);
    UpdateWindow(hDownloadWindow);
}

// Function to close the download window
void CloseDownloadWindow() {
    if (hDownloadWindow != NULL) {
        DestroyWindow(hDownloadWindow);
        hDownloadWindow = NULL;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Register the window class.
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = CreateSolidBrush(RGB(192, 192, 192)); // Grey background

    RegisterClass(&wc);

    // Create the window.
    hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        "PC98Launcher Editor",              // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 480,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void OpenFolder(HWND hwnd)
{
	folderCount = 0;
	memset(folderNames, 0, sizeof(folderNames));
	
	// Clear the list box before adding new items
	SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
	
	// Open folder selection dialog on startup
	BROWSEINFO bi = { 0 };
	bi.lpszTitle = "Select a folder";
	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

	if (pidl != 0) 
	{
		// Get the name of the folder
		SHGetPathFromIDList(pidl, mainfolder);
		
#if 0
        // Prompt for platform selection
        int msgboxID = MessageBox(hwnd, 
                                  _T("Select the platform for this folder:\n\nYes for PC-98\nNo for Sharp X68000"), 
                                  _T("Select Platform"), 
                                  MB_ICONQUESTION | MB_YESNO);

        if (msgboxID == IDYES) {
            platform_id_selected = 95; // PC-98
        } else {
            platform_id_selected = 106; // Sharp X68000
        }
#endif

		// List directories in the array
		ListDirectories(hwnd, mainfolder);

		// Populate list box with folder names
		for (int i = 0; i < folderCount; i++) {
			AddFolderToListBox(hwnd, folderNames[i]);
		}

		// Free memory used
		CoTaskMemFree(pidl);
	}	
}

BOOL IsValidGameName(const TCHAR* gameName) {
    if (gameName == NULL || gameName[0] == _T('\0')) {
        // The string is empty
        return FALSE;
    }

    for (int i = 0; gameName[i] != _T('\0'); i++) {
        if (gameName[i] != _T(' ')) {
            // Found a non-space character, the name is valid
            return TRUE;
        }
    }

    // The string contains only spaces
    return FALSE;
}

void ListFilesInDirectory(HWND hwnd, TCHAR *folderPath) {
    WIN32_FIND_DATA findFileData;
    TCHAR searchPath[MAX_PATH];
    HANDLE hFind;

    // List *.BAT files
    _stprintf(searchPath, _T("%s\\*.BAT"), folderPath);
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            SendMessage(hFileListBox, LB_ADDSTRING, 0, (LPARAM)findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }

    // List *.EXE files
    _stprintf(searchPath, _T("%s\\*.EXE"), folderPath);
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            SendMessage(hFileListBox, LB_ADDSTRING, 0, (LPARAM)findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
    
    // List *.COM files
    _stprintf(searchPath, _T("%s\\*.COM"), folderPath);
    hFind = FindFirstFile(searchPath, &findFileData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            SendMessage(hFileListBox, LB_ADDSTRING, 0, (LPARAM)findFileData.cFileName);
        } while (FindNextFile(hFind, &findFileData) != 0);
        FindClose(hFind);
    }

    // Repeat for *.EXE and *.COM files
    // ... [Add similar blocks for *.EXE and *.COM]
}


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create menus
            CreateMenus(hwnd);

            // Create list box with a white background
            hListBox = CreateWindowW(L"LISTBOX", NULL,
                                     WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
                                     10, 10, 280, 360,
                                     hwnd, (HMENU) 1, NULL, NULL);
            SendMessage(hListBox, WM_CTLCOLORLISTBOX, (WPARAM)CreateSolidBrush(RGB(255, 255, 255)), 0);


			// Create edit controls in a loop
			for (int i = 0; i < editControlCount; ++i) {
				// Adjusted positions for edit controls
				int controlX = 384; // Adjust as needed to not overlap the list box
				int controlY = 10 + i * 30; // Adjusted spacing
				int labelWidth = 80;
				int controlWidth = 200;

				// Create label for the edit control
				CreateWindowA("STATIC", editControls[i].label, WS_CHILD | WS_VISIBLE | SS_LEFT,
							  controlX - labelWidth, controlY, labelWidth, 20,
							  hwnd, NULL, NULL, NULL);

				// Create the edit control
				CreateWindowA("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
							  controlX, controlY,
							  controlWidth, 20,
							  hwnd, (HMENU)(intptr_t)editControls[i].id, NULL, NULL);
			}
			
			// Create Download Screenshots button
			CreateWindowW(L"BUTTON", L"Download Screenshots",
						  WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
						  300, 320, 200, 40, // Adjusted position
						  hwnd, (HMENU) ID_DOWNLOAD_BUTTON, NULL, NULL);

			// Create a new list box for file listings
			hFileListBox = CreateWindowW(L"LISTBOX", NULL,
										 WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY,
										 600, 10, 180, 360, // Adjust position and size
										 hwnd, (HMENU) 2, NULL, NULL);


			OpenFolder(hwnd);
            break;
        }
        case WM_COMMAND: {
           switch (LOWORD(wParam)) {
				case 1:
				if (HIWORD(wParam) == LBN_SELCHANGE) {
					// When a folder is selected in the list box
					if (currentFolderPath) SaveLaunchDat(hwnd, currentFolderPath);
					
					int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
					SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)currentFolderPath);
					LoadLaunchDat(hwnd, currentFolderPath);
					
					TCHAR selectedFolderPath[MAX_PATH];
					_stprintf(selectedFolderPath, _T("%s\\%s"), mainfolder, currentFolderPath);
					
					SendMessage(hFileListBox, LB_RESETCONTENT, 0, 0);
					ListFilesInDirectory(hwnd, selectedFolderPath);
				}
				break;
				
				case 2:
				if (HIWORD(wParam) == LBN_SELCHANGE) {
					int selIndex = SendMessage(hFileListBox, LB_GETCURSEL, 0, 0);
					TCHAR fileName[MAX_PATH];
					SendMessage(hFileListBox, LB_GETTEXT, selIndex, (LPARAM)fileName);

					// Set the selected file name in the "Start" edit control
					SetWindowText(GetDlgItem(hwnd, ID_START_EDIT), fileName);
				}
				break;
                case IDM_OPEN_FOLDER: {
                    // Handle "Open Folder" menu item
                    // ... [Open folder logic]
                    OpenFolder(hwnd);
                    break;
                }
                case ID_DOWNLOAD_BUTTON:
					TCHAR gameName[256];
					GetWindowText(GetDlgItem(hwnd, ID_NAME_EDIT), gameName, sizeof(gameName)/sizeof(gameName[0]));
					
					if (IsValidGameName(gameName) == FALSE)
					{
						MessageBox(hwnd, _T("Name field empty"), _T("Warning"), MB_OK);
						break;
					}
					
					ShowDownloadWindow(hwnd);

					int selIndex = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
					TCHAR folderName[MAX_FOLDER_NAME_LENGTH];
					SendMessage(hListBox, LB_GETTEXT, selIndex, (LPARAM)folderName);

					TCHAR pngFileName[MAX_PATH];
					
					TCHAR selectedfolder[MAX_PATH];
					_stprintf(selectedfolder, _T("%s\\%s\\"), mainfolder, currentFolderPath);
					
					int error = fetch_and_download_screenshots(gameName, folderName, selectedfolder, platform_id_selected);
					if (error != 0)
					{
						// Could not find game
						switch(error)
						{
							default:
								MessageBox(hwnd, _T("Cannot connect to Mobygames"), _T("Warning"), MB_OK); 
							break;
							case 2:
								MessageBox(hwnd, _T("Could not find game through name !"), _T("Warning"), MB_OK);
							break;
						}
						CloseDownloadWindow();
						break;
					}

					TCHAR newImages[MAX_PATH] = _T("");
					for (int i = 0; i < 4; i++) {
						TCHAR bmpFileName[MAX_PATH];
						_stprintf(bmpFileName, _T("%s\\%s_%d.BMP"), selectedfolder, folderName, i + 1);
						_stprintf(pngFileName, _T("%s\\%s_%d.PNG"), selectedfolder, folderName, i + 1);
						downscale_png_to_bmp(pngFileName, bmpFileName);

						if (i > 0) {
							_tcscat(newImages, _T(","));
						}
						TCHAR imageName[MAX_PATH];
						_stprintf(imageName, _T("%s_%d.BMP"), folderName, i + 1);
						_tcscat(newImages, imageName);
					}
					
					
					SetWindowText(GetDlgItem(hwnd, ID_DEVELOPER_EDIT), (LPCSTR) Get_Developer_Game());
					SetWindowText(GetDlgItem(hwnd, ID_YEAR_EDIT), (LPCSTR) Get_Year_Game());
					SetWindowText(GetDlgItem(hwnd, ID_GENRE_EDIT), (LPCSTR) Get_Genre_Game());
					
					// Update the images field in LaunchDatProperties
					SetWindowText(GetDlgItem(hwnd, ID_IMAGES_EDIT), newImages);
					
					CloseDownloadWindow();
                break;
                case IDM_EXIT: {
                    // Handle "Exit" menu item
                    PostQuitMessage(0);
                    break;
                }
                case IDM_ABOUT: {
                    MessageBox(hwnd, _T("PC98Launcher Editor by gameblabla, 2023"), _T("About"), MB_OK);
                    break;
                }
				case IDM_SAVE: {
					// Call SaveLaunchDat
					SaveLaunchDat(hwnd, currentFolderPath);
					MessageBox(hwnd, _T("Settings saved successfully"), _T("Save"), MB_OK);
					break;
				}
                // ... [Rest of the WM_COMMAND code]
            }
            
            break;
        }
        case WM_DESTROY: {
            SaveLaunchDat(hwnd, currentFolderPath); // Save the current LAUNCH.DAT file before closing
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void CreateMenus(HWND hwnd) {
    HMENU hMenubar = CreateMenu();
    HMENU hFileMenu = CreateMenu();
    HMENU hHelpMenu = CreateMenu();

    AppendMenu(hFileMenu, MF_STRING, IDM_OPEN_FOLDER, _T("Open Folder"));
    AppendMenu(hFileMenu, MF_STRING, IDM_SAVE, _T("Save current selection")); // Add Save menu item
    AppendMenu(hFileMenu, MF_STRING, IDM_EXIT, _T("Exit"));
    AppendMenu(hHelpMenu, MF_STRING, IDM_ABOUT, _T("About"));

    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFileMenu, _T("File"));
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hHelpMenu, _T("Help"));

    SetMenu(hwnd, hMenubar);
}



void ListDirectories(HWND hwnd, TCHAR *folderPath) {
    WIN32_FIND_DATA findFileData;
    TCHAR searchPath[MAX_PATH];
    _stprintf(searchPath, _T("%s\\*"), folderPath);
    HANDLE hFind = FindFirstFile(searchPath, &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        MessageBox(hwnd, "Failed to list directories", "Error", MB_OK);
        return;
    } 

    do {
        if ((findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            _tcscmp(findFileData.cFileName, _T(".")) != 0 &&
            _tcscmp(findFileData.cFileName, _T("..")) != 0) {
			// Add folder name to array
            if (folderCount < MAX_FOLDERS) {
                _tcscpy_s(folderNames[folderCount], MAX_FOLDER_NAME_LENGTH, findFileData.cFileName);
                folderCount++;
            }
        }
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void AddFolderToListBox(HWND hwnd, TCHAR *folderName) {
    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)folderName);
}

void LoadLaunchDat(HWND hwnd, TCHAR *folderName) {
    TCHAR filePath[MAX_PATH];
    _stprintf(filePath, _T("%s\\%s\\LAUNCH.DAT"), mainfolder, folderName);
    

    // Clear all edit control fields before loading new data
    for (int i = 0; i < editControlCount; ++i) {
        SetWindowText(GetDlgItem(hwnd, editControls[i].id), _T(""));
    }

    FILE *file = _tfopen(filePath, _T("rb"));
    if (!file) {
        MessageBox(hwnd, _T("Failed to open LAUNCH.DAT"), _T("Error"), MB_OK);
        return;
    }

    TCHAR line[256], *property, *value;
    while (_fgetts(line, sizeof(line), file)) {
        property = _tcstok(line, _T("="));
        value = _tcstok(NULL, _T("\r\n")); // Handle potential carriage return

        if (property != NULL && value != NULL) {
            for (int i = 0; i < editControlCount; ++i) {
                if (_tcscmp(editControls[i].real_label, property) == 0) {
                    SetWindowText(GetDlgItem(hwnd, editControls[i].id), value);
                    break;
                }
            }
        }
    }

    fclose(file);
}

void SaveLaunchDat(HWND hwnd, TCHAR *folderName) {
    TCHAR filePath[MAX_PATH];
    _stprintf(filePath, _T("%s\\%s\\LAUNCH.DAT"), mainfolder, folderName);

	// To make sure it outputs it as LF instead of CRLF to avoid a bug in PC98Launcher only handling CRLF line endings
    FILE *file = _tfopen(filePath, _T("wb"));
    if (!file) {
        MessageBox(hwnd, _T("Failed to save LAUNCH.DAT"), _T("Error"), MB_OK);
        return;
    }

    TCHAR value[256];
    for (int i = 0; i < editControlCount; ++i) {
        GetWindowText(GetDlgItem(hwnd, editControls[i].id), value, sizeof(value));

        // Skip saving altstart if it is empty
        if (editControls[i].id == ID_ALT_START_EDIT && _tcslen(value) == 0) {
            continue;
        }

        _ftprintf(file, _T("%s=%s\n"), editControls[i].real_label, value);
    }

    fclose(file);
}


