win32_smartdlg

win32_smartdlg is a library for creating dynamic, pixel-less layouts in Win32 applications. It provides an abstraction layer over the traditional Win32 window API, allowing for more flexible and responsive UI designs.

Features

	•	Dynamic Layouts: Automatically resizes and repositions controls.
	•	Easy Integration: Simple API for integrating into existing Win32 projects.
	•	No Pixel Positioning: Define layouts without worrying about pixel coordinates.

Getting Started

Prerequisites

	•	Windows: This library is designed for Windows applications.
	•	C++ Compiler: Ensure you have a C++ compiler that supports the Win32 API.

Installation

	1.	Clone the Repository:

git clone https://github.com/thpatch/win32_smartdlg


	2.	Include in Project: Add the source files to your C++ project.

Usage

	1.	Include Header:

#include "SmartDlg.h"


	2.	Initialize Layout Manager:

void InitDialog(HWND hDlg) {
    SmartDlg dlg(hDlg);
    dlg.addControl(IDC_BUTTON1, SMARTDLG_ANCHOR_LEFT | SMARTDLG_ANCHOR_TOP);
    dlg.addControl(IDC_EDIT1, SMARTDLG_ANCHOR_LEFT | SMARTDLG_ANCHOR_RIGHT | SMARTDLG_ANCHOR_TOP);
    // Add more controls as needed
}


	3.	Compile and Run: Build and run your application. The layout manager will handle dynamic resizing.

Example

Here’s a basic example of setting up a dialog with dynamic layouts:

#include "SmartDlg.h"

void InitDialog(HWND hDlg) {
    SmartDlg dlg(hDlg);
    dlg.addControl(IDC_BUTTON1, SMARTDLG_ANCHOR_LEFT | SMARTDLG_ANCHOR_TOP);
    dlg.addControl(IDC_EDIT1, SMARTDLG_ANCHOR_LEFT | SMARTDLG_ANCHOR_RIGHT | SMARTDLG_ANCHOR_TOP);
    // More control initializations
}

Contributing

Contributions are welcome! Please fork the repository and submit pull requests.

Disclaimer

This README was produced using ChatGPT and may contain errors, both in its content and its formatting. Please verify the information and refer to the source code for the most accurate details.

License

This project is licensed under the MIT License.
