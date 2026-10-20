#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
#include "source/MainForm.h"
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int) {
    try {
        Application->Initialize();
        Application->MainFormOnTaskBar = true;
        Application->CreateForm(__classid(TMainForm), &MainForm);
        MainForm->Show();
        MainForm->start();
        Application->Run();
    } catch (Exception& exception) {
        Application->ShowException(&exception);
    }
    return 0;
}
