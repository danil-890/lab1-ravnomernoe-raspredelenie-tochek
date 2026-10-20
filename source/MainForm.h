#ifndef MAIN_FORM_H
#define MAIN_FORM_H
#include <System.Classes.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
#include <Vcl.Menus.hpp>
#include <Vcl.Samples.Spin.hpp>
#include "GLS.Scene.hpp"
#include "GLS.Objects.hpp"
#include "GLS.SceneViewer.hpp"
#include "GLS.Cadencer.hpp"
// Winspool's SetForm macro must not rename GLScene's Pascal method.
#ifdef SetForm
#undef SetForm
#endif
#include "GLS.SimpleNavigation.hpp"
#include "PointCloud.h"
#include <chrono>
#include <vector>

class TMainForm : public TForm {
public:
    __fastcall TMainForm(TComponent* owner);
    __fastcall ~TMainForm();
    void start();
private:
    typedef std::chrono::steady_clock Clock;
    TPanel* controls_;
    TGLSceneViewer* viewer_;
    TGLScene* scene_;
    TGLCamera* camera_;
    TGLDummyCube *domain_, *cloud_, *frame_, *lookAt_;
    TGLPoints* points_;
    TGLCadencer* cadencer_;
    TGLSimpleNavigation* navigator_;
    TRadioGroup *containerChoice_, *displayChoice_, *cameraChoice_;
    TSpinEdit *count_, *seed_, *selection_;
    TLabel *coordinates_, *state_;
    TButton *generateButton_, *pauseButton_, *benchmarkButton_;
    TStatusBar* status_;
    std::vector<lab1::Particle> particles_;
    std::vector<TGLSphere*> spheres_;
    lab1::Container currentContainer_;
    lab1::Display currentDisplay_;
    double elapsed_, savedElapsed_, benchmarkRenderSeconds_;
    int lastLifetimeSecond_;
    bool paused_, benchmarking_, ready_, rendererRecorded_;
    bool suite_, captured_, monitorStarted_;
    int suiteStep_;
    UnicodeString suiteDirectory_;
    unsigned long long totalFrames_, intervalFrames_, benchmarkFrames_;
    Clock::time_point lastTick_, intervalStart_, benchmarkStart_;
    UnicodeString renderer_, vendor_, glVersion_;
    void createControls();
    void createScene();
    void createMenu();
    void rebuildFrame();
    void rebuildVisible(double elapsed);
    void updateCoordinates();
    void setControlsEnabled(bool enabled);
    UnicodeString resultsDirectory();
    void finishBenchmark();
    void nextSuiteCase();
    void captureScene(const UnicodeString& path);
    void __fastcall generate(TObject* sender);
    void __fastcall clear(TObject* sender);
    void __fastcall pause(TObject* sender);
    void __fastcall cameraChanged(TObject* sender);
    void __fastcall selectionChanged(TObject* sender);
    void __fastcall progress(TObject* sender, const double deltaTime, const double newTime);
    void __fastcall afterRender(TObject* sender);
    void __fastcall postRender(TObject* sender);
    void __fastcall benchmark(TObject* sender);
    void __fastcall exportCsv(TObject* sender);
    void __fastcall savePicture(TObject* sender);
    void __fastcall showHelp(TObject* sender);
    void __fastcall showAbout(TObject* sender);
    void __fastcall exitApplication(TObject* sender);
    void __fastcall closing(TObject* sender, TCloseAction& action);
};
extern PACKAGE TMainForm* MainForm;
#endif
