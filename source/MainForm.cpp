#include <vcl.h>
#pragma hdrstop
#include "MainForm.h"
#include "GLS.SpaceText.hpp"
#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Winapi.OpenGL.hpp>
#include <algorithm>
#include <cmath>
#include <memory>
#pragma package(smart_init)
#pragma link "GLS.Scene"
#pragma link "GLS.Objects"
#pragma link "GLS.SceneViewer"
#pragma link "GLS.Cadencer"
#pragma link "GLS.SimpleNavigation"
#pragma link "GLS.SpaceText"
TMainForm* MainForm;

namespace {
TLabel* label(TComponent* owner,TWinControl* parent,int x,int y,const UnicodeString& text) {
    auto p=new TLabel(owner); p->Parent=parent; p->Left=x; p->Top=y; p->Caption=text; return p;
}
TButton* button(TComponent* owner,TWinControl* parent,int y,const UnicodeString& text,TNotifyEvent action) {
    auto p=new TButton(owner); p->Parent=parent; p->SetBounds(12,y,232,30);
    p->Caption=text; p->OnClick=action; return p;
}
void item(TComponent* owner,TMenuItem* parent,const UnicodeString& text,TNotifyEvent action) {
    auto p=new TMenuItem(owner); p->Caption=text; p->OnClick=action; parent->Add(p);
}
TGLLines* lines(TGLBaseSceneObject* parent,float r,float g,float b) {
    auto p=static_cast<TGLLines*>(parent->AddNewChild(__classid(TGLLines)));
    p->NodesAspect=lnaInvisible; p->SplineMode=lsmSegments;
    p->LineColor->SetColor(r,g,b,1); return p;
}
void segment(TGLLines* p,float x,float y,float z,float a,float b,float c) {
    p->AddNode(x,y,z); p->AddNode(a,b,c);
}
void text3d(TGLBaseSceneObject* parent,const UnicodeString& text,float x,float y,float z,
            float r,float g,float b) {
    auto p=static_cast<TGLSpaceText*>(parent->AddNewChild(__classid(TGLSpaceText)));
    p->Text=text; p->Font->Name=L"Arial"; p->Extrusion=0;
    p->TextHeight=44; p->Position->SetPoint(x,y,z);
    p->Material->FrontProperties->Diffuse->SetColor(r,g,b,1);
}
}

__fastcall TMainForm::TMainForm(TComponent* owner)
    : TForm(owner,0), controls_(nullptr), viewer_(nullptr), scene_(nullptr), camera_(nullptr),
      domain_(nullptr), cloud_(nullptr), frame_(nullptr), lookAt_(nullptr), points_(nullptr),
      cadencer_(nullptr), navigator_(nullptr), elapsed_(0), savedElapsed_(0), benchmarkRenderSeconds_(0),
      lastLifetimeSecond_(-1), paused_(false), benchmarking_(false), ready_(false),
      rendererRecorded_(false), suite_(false), captured_(false), monitorStarted_(false), suiteStep_(0),
      totalFrames_(0), intervalFrames_(0), benchmarkFrames_(0) {
    Caption=L"Лабораторная №1. Случайные точки"; Position=poScreenCenter;
    ClientWidth=1180; ClientHeight=760; Font->Name=L"Segoe UI"; Font->Size=9;
    Constraints->MinWidth=1000; Constraints->MinHeight=740;
    createControls(); createMenu(); createScene();
    lastTick_=intervalStart_=Clock::now(); OnClose=closing;
    suite_=FindCmdLineSwitch(L"benchmark-all",true);
    if (suite_) {
        suiteDirectory_=ExtractFilePath(Application->ExeName)+L"results\\run-"+FormatDateTime(L"yyyymmdd-hhnnss",Now());
        ForceDirectories(suiteDirectory_);
    }
}
void TMainForm::start() {
    if (ready_) return;
    ready_=true;
    generate(nullptr);
}
__fastcall TMainForm::~TMainForm() {
    ready_=false;
    if (cadencer_) cadencer_->Enabled=false;
    // The navigator owns event hooks into the viewer and form.
    delete navigator_; navigator_=nullptr;
}

void TMainForm::createControls() {
    status_=new TStatusBar(this); status_->Parent=this; status_->Align=alBottom;
    status_->Panels->Add()->Width=700; status_->Panels->Add()->Width=300;
    controls_=new TPanel(this); controls_->Parent=this; controls_->Width=260; controls_->Align=alRight;
    controls_->BevelOuter=bvNone;
    label(this,controls_,12,12,L"Число объектов");
    count_=new TSpinEdit(this); count_->Parent=controls_; count_->SetBounds(142,9,102,26);
    count_->MinValue=1; count_->MaxValue=100000; count_->Value=100000;
    label(this,controls_,12,44,L"Начальное число");
    seed_=new TSpinEdit(this); seed_->Parent=controls_; seed_->SetBounds(142,41,102,26);
    seed_->MinValue=0; seed_->MaxValue=2147483647; seed_->Value=23;
    containerChoice_=new TRadioGroup(this); containerChoice_->Parent=controls_;
    containerChoice_->SetBounds(12,77,232,117); containerChoice_->Caption=L"Контейнер";
    for (auto s:{L"Куб",L"Сфера",L"Цилиндр",L"Конус"}) containerChoice_->Items->Add(s);
    containerChoice_->ItemIndex=0;
    containerChoice_->OnClick=generate;
    displayChoice_=new TRadioGroup(this); displayChoice_->Parent=controls_;
    displayChoice_->SetBounds(12,202,232,77); displayChoice_->Caption=L"Объекты";
    displayChoice_->Items->Add(L"Точки: 255 цветов"); displayChoice_->Items->Add(L"Сферы: 7 цветов, r = 0…1");
    displayChoice_->ItemIndex=0;
    displayChoice_->OnClick=generate;
    cameraChoice_=new TRadioGroup(this); cameraChoice_->Parent=controls_;
    cameraChoice_->SetBounds(12,287,232,77); cameraChoice_->Caption=L"Камера";
    cameraChoice_->Items->Add(L"Снаружи"); cameraChoice_->Items->Add(L"В центре облака");
    cameraChoice_->ItemIndex=0; cameraChoice_->OnClick=cameraChanged;
    generateButton_=button(this,controls_,375,L"Сгенерировать",generate);
    pauseButton_=button(this,controls_,411,L"Пауза свечения",pause);
    benchmarkButton_=button(this,controls_,447,L"Измерить FPS (15 секунд)",benchmark);
    button(this,controls_,483,L"Сохранить изображение…",savePicture);
    label(this,controls_,12,529,L"Координаты объекта №");
    selection_=new TSpinEdit(this); selection_->Parent=controls_; selection_->SetBounds(158,525,86,26);
    selection_->MinValue=1; selection_->MaxValue=100000; selection_->Value=1;
    selection_->OnChange=selectionChanged;
    coordinates_=label(this,controls_,12,560,L""); coordinates_->AutoSize=false;
    coordinates_->SetBounds(12,560,232,92); coordinates_->WordWrap=true;
    state_=label(this,controls_,12,659,L""); state_->AutoSize=false;
    state_->SetBounds(12,659,232,54); state_->WordWrap=true;
    viewer_=new TGLSceneViewer(this); viewer_->Name=L"SceneViewer"; viewer_->Parent=this;
    viewer_->Align=alClient; viewer_->Buffer->BackgroundColor=static_cast<TColor>(0x00170F0B);
    viewer_->Buffer->Lighting=false; viewer_->VSync=vsmNoSync;
    viewer_->AfterRender=afterRender; viewer_->PostRender=postRender;
}
void TMainForm::createMenu() {
    auto menu=new TMainMenu(this); Menu=menu;
    auto file=new TMenuItem(this); file->Caption=L"&File"; menu->Items->Add(file);
    item(this,file,L"Сохранить координаты CSV…",exportCsv);
    item(this,file,L"Сохранить изображение PNG…",savePicture); item(this,file,L"Выход",exitApplication);
    auto edit=new TMenuItem(this); edit->Caption=L"&Edit"; menu->Items->Add(edit);
    item(this,edit,L"Сгенерировать заново",generate); item(this,edit,L"Очистить",clear);
    item(this,edit,L"Пауза / продолжить",pause);
    auto view=new TMenuItem(this); view->Caption=L"&View"; menu->Items->Add(view);
    item(this,view,L"Вернуть камеру",cameraChanged); item(this,view,L"Измерить FPS",benchmark);
    auto help=new TMenuItem(this); help->Caption=L"&Help"; menu->Items->Add(help);
    item(this,help,L"Управление",showHelp); item(this,help,L"О программе",showAbout);
}
void TMainForm::createScene() {
    scene_=new TGLScene(this); scene_->ObjectsSorting=osNone;
    domain_=static_cast<TGLDummyCube*>(scene_->Objects->AddNewChild(__classid(TGLDummyCube)));
    domain_->Name=L"ModellingBox"; domain_->Position->SetPoint(512,512,512);
    domain_->CubeSize=1024; domain_->VisibleAtRunTime=true;
    domain_->EdgeColor->SetColor(0.35f,0.42f,0.50f,1);
    cloud_=static_cast<TGLDummyCube*>(domain_->AddNewChild(__classid(TGLDummyCube)));
    frame_=static_cast<TGLDummyCube*>(scene_->Objects->AddNewChild(__classid(TGLDummyCube)));
    lookAt_=static_cast<TGLDummyCube*>(scene_->Objects->AddNewChild(__classid(TGLDummyCube)));
    camera_=static_cast<TGLCamera*>(scene_->Objects->AddNewChild(__classid(TGLCamera)));
    camera_->TargetObject=domain_; camera_->DepthOfView=12000; camera_->NearPlaneBias=0.02f;
    camera_->FocalLength=100; viewer_->Camera=camera_;
    navigator_=new TGLSimpleNavigation(this); navigator_->Name=L"GLSimpleNavigator";
    navigator_->Form=this; navigator_->GLSceneViewer=viewer_;
    navigator_->FormCaption=L"Лабораторная №1 — %FPS";
    navigator_->MoveAroundTargetSpeed=0.3f; navigator_->ZoomSpeed=1.15f;
    cadencer_=new TGLCadencer(this); cadencer_->Enabled=false;
    cadencer_->Scene=scene_; cadencer_->OnProgress=progress;
    cameraChanged(nullptr);
}
void TMainForm::rebuildFrame() {
    frame_->DeleteChildren();
    auto x=lines(frame_,1,0.35f,0.35f); segment(x,0,0,0,1090,0,0);
    auto y=lines(frame_,0.4f,1,0.4f); segment(y,0,0,0,0,1090,0);
    auto z=lines(frame_,0.4f,0.65f,1); segment(z,0,0,0,0,0,1090);
    text3d(frame_,L"X",1100,0,0,1,0.35f,0.35f);
    text3d(frame_,L"Y",0,1100,0,0.4f,1,0.4f);
    text3d(frame_,L"Z",0,0,1100,0.4f,0.65f,1);
    auto ticks=lines(frame_,0.55f,0.6f,0.7f);
    for (int v=0; v<=1024; v+=128) {
        segment(ticks,v,-10,0,v,10,0); segment(ticks,-10,v,0,10,v,0);
        segment(ticks,0,-10,v,0,10,v);
        if (v%256 == 0) {
            text3d(frame_,IntToStr(v),v,-35,0,0.8f,0.8f,0.8f);
            if (v) { text3d(frame_,IntToStr(v),-60,v,0,0.8f,0.8f,0.8f);
                     text3d(frame_,IntToStr(v),0,-35,v,0.8f,0.8f,0.8f); }
        }
    }
    if (currentContainer_ == lab1::Container::Cube) return;
    auto wire=lines(frame_,0.4f,0.48f,0.58f);
    const double pi=3.14159265358979323846;
    for (int i=0; i<96; ++i) {
        const double a=2*pi*i/96,b=2*pi*(i+1)/96;
        const float x1=512+511*std::cos(a),z1=512+511*std::sin(a);
        const float x2=512+511*std::cos(b),z2=512+511*std::sin(b);
        if (currentContainer_ == lab1::Container::Sphere) {
            segment(wire,x1,512,z1,x2,512,z2);
            segment(wire,x1,z1,512,x2,z2,512); segment(wire,512,x1,z1,512,x2,z2);
        } else {
            segment(wire,x1,1,z1,x2,1,z2);
            if (currentContainer_ == lab1::Container::Cylinder) segment(wire,x1,1023,z1,x2,1023,z2);
            if (i%12 == 0) segment(wire,x1,1,z1,
                currentContainer_ == lab1::Container::Cone ? 512:x1,1023,
                currentContainer_ == lab1::Container::Cone ? 512:z1);
        }
    }
}
void __fastcall TMainForm::generate(TObject* sender) {
    if (!ready_ || benchmarking_ || (suite_ && sender)) return;
    Vcl::Forms::Screen->Cursor=crHourGlass; cadencer_->Enabled=false;
    try {
        currentContainer_=static_cast<lab1::Container>(containerChoice_->ItemIndex);
        currentDisplay_=static_cast<lab1::Display>(displayChoice_->ItemIndex);
        lab1::Generator generator(static_cast<std::uint32_t>(seed_->Value));
        auto next=generator.generate(count_->Value,currentContainer_,currentDisplay_);
        scene_->BeginUpdate();
        try {
            points_=nullptr; spheres_.clear(); cloud_->DeleteChildren(); particles_.swap(next);
            if (currentDisplay_ == lab1::Display::Points) {
                points_=static_cast<TGLPoints*>(cloud_->AddNewChild(__classid(TGLPoints)));
                points_->Size=2; points_->Style=psSquare; points_->Static=false;
                points_->Positions->Capacity=static_cast<int>(particles_.size());
                points_->Colors->Capacity=static_cast<int>(particles_.size());
            } else {
                spheres_.reserve(particles_.size());
                for (const auto& p:particles_) {
                    auto sphere=static_cast<TGLSphere*>(cloud_->AddNewChild(__classid(TGLSphere)));
                    sphere->Radius=p.radius; sphere->Slices=8; sphere->Stacks=6;
                    sphere->Position->SetPoint(p.position.x-512,p.position.y-512,p.position.z-512);
                    sphere->Material->FrontProperties->Diffuse->SetColor(p.color.r,p.color.g,p.color.b,p.color.a);
                    spheres_.push_back(sphere);
                }
            }
            elapsed_=0; lastLifetimeSecond_=-1; paused_=false; pauseButton_->Caption=L"Пауза свечения";
            rebuildFrame(); rebuildVisible(0); selection_->MaxValue=static_cast<int>(particles_.size());
            selection_->Value=1; updateCoordinates(); cameraChanged(nullptr);
        } __finally { scene_->EndUpdate(); }
    } __finally {
        lastTick_=Clock::now(); cadencer_->Enabled=true; Vcl::Forms::Screen->Cursor=crDefault;
    }
}
void TMainForm::rebuildVisible(double elapsed) {
    std::size_t alive=0;
    if (points_) { points_->Positions->Clear(); points_->Colors->Clear(); }
    for (std::size_t i=0; i<particles_.size(); ++i) {
        const auto& p=particles_[i]; const bool visible=p.alive(elapsed);
        if (!spheres_.empty()) spheres_[i]->Visible=visible;
        if (!visible) continue;
        ++alive;
        if (points_) { points_->Positions->Add(p.position.x-512,p.position.y-512,p.position.z-512);
            points_->Colors->Add(p.color.r,p.color.g,p.color.b,p.color.a); }
    }
    if (points_) { points_->Visible=alive>0; points_->StructureChanged(); }
    state_->Caption=Format(L"Светятся: %d / %d\nВремя: %.1f с",ARRAYOFCONST((int(alive),int(particles_.size()),static_cast<long double>(elapsed))));
    updateCoordinates();
}
void __fastcall TMainForm::clear(TObject*) {
    if (benchmarking_) return;
    cloud_->DeleteChildren(); points_=nullptr; spheres_.clear(); particles_.clear();
    elapsed_=0; coordinates_->Caption=L"Облако очищено"; state_->Caption=L"Светятся: 0";
}
void __fastcall TMainForm::pause(TObject*) {
    if (benchmarking_) return;
    paused_=!paused_; pauseButton_->Caption=paused_ ? L"Продолжить свечение":L"Пауза свечения";
}
void __fastcall TMainForm::cameraChanged(TObject*) {
    if (!camera_) return;
    if (cameraChoice_->ItemIndex == 0) {
        camera_->TargetObject=domain_; camera_->Position->SetPoint(2050,1750,2250);
    } else {
        // Target must differ from the camera position: avoid a zero view direction.
        lookAt_->Position->SetPoint(512,512,0); camera_->TargetObject=lookAt_;
        camera_->Position->SetPoint(512,512,512);
    }
    camera_->Up->SetVector(0,1,0); viewer_->Invalidate();
}
void TMainForm::updateCoordinates() {
    if (particles_.empty()) return;
    const int index=std::max(0,std::min(int(particles_.size())-1,selection_->Value-1));
    const auto& p=particles_[index];
    coordinates_->Caption=Format(L"x = %.3f\ny = %.3f\nz = %.3f\nЦвет: %d; жизнь: %d с; r: %.3f",
        ARRAYOFCONST((static_cast<long double>(p.position.x),static_cast<long double>(p.position.y),static_cast<long double>(p.position.z),int(p.colorIndex),int(p.lifetimeSeconds),static_cast<long double>(p.radius))));
}
void __fastcall TMainForm::selectionChanged(TObject*) { if (ready_) updateCoordinates(); }
void __fastcall TMainForm::progress(TObject*,const double,const double) {
    if (!ready_) return;
    const auto now=Clock::now();
    const double dt=std::chrono::duration<double>(now-lastTick_).count(); lastTick_=now;
    if (!paused_ && !benchmarking_) elapsed_+=dt;
    if (!benchmarking_ && int(elapsed_) != lastLifetimeSecond_) {
        lastLifetimeSecond_=int(elapsed_); rebuildVisible(elapsed_);
    }
    const double interval=std::chrono::duration<double>(now-intervalStart_).count();
    if (interval>=1) {
        status_->Panels->Items[0]->Text=Format(L"Экран: %d × %d; фактическая частота: %.1f кадр/с",
            ARRAYOFCONST((viewer_->Width,viewer_->Height,static_cast<long double>(double(intervalFrames_)/interval))));
        intervalFrames_=0; intervalStart_=now;
    }
    if (suite_ && !benchmarking_ && totalFrames_>0) nextSuiteCase();
    if (benchmarking_) {
        const double t=std::chrono::duration<double>(now-benchmarkStart_).count();
        if (!monitorStarted_ && t>=5) { viewer_->ResetPerformanceMonitor(); monitorStarted_=true; }
        if (!captured_ && t>=3 && t<5) {
            captured_=true;
            captureScene(resultsDirectory()+L"\\"+UnicodeString(lab1::containerName(currentContainer_))+
                (currentDisplay_ == lab1::Display::Points ? L"-Points-":L"-Spheres-")+
                (cameraChoice_->ItemIndex == 0 ? L"Outside.png":L"Centre.png"));
        }
        if (t>=15) finishBenchmark();
    }
    viewer_->Invalidate();
}
void __fastcall TMainForm::afterRender(TObject*) {
    ++totalFrames_; ++intervalFrames_;
    if (benchmarking_) {
        const double t=std::chrono::duration<double>(Clock::now()-benchmarkStart_).count();
        if (t>=5 && t<15) { ++benchmarkFrames_; benchmarkRenderSeconds_+=viewer_->LastFrameTime(); }
    }
}
void __fastcall TMainForm::postRender(TObject*) {
    if (rendererRecorded_) return;
    const auto renderer=glGetString(GL_RENDERER),vendor=glGetString(GL_VENDOR),version=glGetString(GL_VERSION);
    if (renderer && vendor && version) {
        renderer_=AnsiString(reinterpret_cast<const char*>(renderer)); vendor_=AnsiString(reinterpret_cast<const char*>(vendor));
        glVersion_=AnsiString(reinterpret_cast<const char*>(version)); rendererRecorded_=true;
    }
}
void TMainForm::setControlsEnabled(bool enabled) {
    count_->Enabled=enabled; seed_->Enabled=enabled; containerChoice_->Enabled=enabled;
    displayChoice_->Enabled=enabled; cameraChoice_->Enabled=enabled;
    generateButton_->Enabled=enabled; pauseButton_->Enabled=enabled; benchmarkButton_->Enabled=enabled;
}
UnicodeString TMainForm::resultsDirectory() {
    if (!suiteDirectory_.IsEmpty()) return suiteDirectory_;
    const UnicodeString dir=ExtractFilePath(Application->ExeName)+L"results";
    ForceDirectories(dir); return dir;
}
void __fastcall TMainForm::benchmark(TObject*) {
    if (benchmarking_ || particles_.empty()) return;
    savedElapsed_=elapsed_; rebuildVisible(0); benchmarkFrames_=0; benchmarkRenderSeconds_=0; captured_=false;
    monitorStarted_=false;
    // Use the same buffer counter as the navigator, without its once-a-second reset.
    navigator_->Options=TGLSimpleNavigationOptions() << snoMouseWheelHandled;
    benchmarkStart_=Clock::now(); benchmarking_=true; setControlsEnabled(false);
    state_->Caption=L"Замер FPS: 5 с прогрева + 10 с измерения.\nСвечение приостановлено.";
}
void TMainForm::finishBenchmark() {
    benchmarking_=false; setControlsEnabled(true);
    const double wallFps=benchmarkFrames_/10.0;
    const double navigatorFps=viewer_->FramesPerSecond();
    const double renderFps=benchmarkRenderSeconds_>0 ? benchmarkFrames_/benchmarkRenderSeconds_:0;
    navigator_->Options=TGLSimpleNavigationOptions() << snoMouseWheelHandled << snoShowFPS;
    const UnicodeString path=resultsDirectory()+L"\\fps.csv";
    std::unique_ptr<TStringList> csv(new TStringList);
    if (FileExists(path)) csv->LoadFromFile(path,TEncoding::UTF8);
    else csv->Add(L"date,container,display,camera,count,seed,width,height,warmup_s,measurement_s,frames,wall_fps,navigator_fps,render_fps,renderer,opengl");
    const auto invariant=TFormatSettings::Create(L"en-US");
    csv->Add(FormatDateTime(L"yyyy-mm-dd hh:nn:ss",Now())+L","+UnicodeString(lab1::containerName(currentContainer_))+L","+
        (currentDisplay_ == lab1::Display::Points ? L"Points":L"Spheres")+L","+
        (cameraChoice_->ItemIndex == 0 ? L"Outside":L"Centre")+L","+IntToStr(int(particles_.size()))+L","+
        IntToStr(seed_->Value)+L","+IntToStr(viewer_->Width)+L","+IntToStr(viewer_->Height)+L",5,10,"+
        IntToStr(static_cast<__int64>(benchmarkFrames_))+L","+FloatToStr(wallFps,invariant)+L","+FloatToStr(navigatorFps,invariant)+L","+
        FloatToStr(renderFps,invariant)+L",\""+renderer_+L"\",\""+glVersion_+L"\"");
    csv->SaveToFile(path,TEncoding::UTF8);
    elapsed_=savedElapsed_; rebuildVisible(elapsed_);
    status_->Panels->Items[1]->Text=Format(L"FPS навигатора: %.1f; экран: %.1f",ARRAYOFCONST((static_cast<long double>(navigatorFps),static_cast<long double>(wallFps))));
    if (suite_) ++suiteStep_;
    else ShowMessage(L"Результат измерения сохранён:\n"+path);
}
void TMainForm::nextSuiteCase() {
    if (suiteStep_>=16) {
        std::unique_ptr<TStringList> info(new TStringList);
        info->Add(L"All 16 cases completed: 4 containers x 2 display modes x 2 camera positions.");
        info->Add(L"Objects: 100000; seed: 23; warm-up: 5 s; measurement: 10 s; VSync: off.");
        info->Add(L"Renderer: "+renderer_); info->Add(L"Vendor: "+vendor_); info->Add(L"OpenGL: "+glVersion_);
        info->SaveToFile(suiteDirectory_+L"\\completed.txt",TEncoding::UTF8);
        suite_=false; Application->Terminate(); return;
    }
    const int container=suiteStep_/4, display=(suiteStep_%4)/2;
    const bool regenerate=particles_.empty() || int(currentContainer_)!=container || int(currentDisplay_)!=display;
    containerChoice_->ItemIndex=container; displayChoice_->ItemIndex=display;
    count_->Value=100000; seed_->Value=23;
    if (regenerate) generate(nullptr);
    cameraChoice_->ItemIndex=suiteStep_%2; cameraChanged(nullptr);
    paused_=true; benchmark(nullptr);
}
void TMainForm::captureScene(const UnicodeString& path) {
    std::unique_ptr<TBitmap> bitmap(viewer_->CreateSnapShotBitmap());
    std::unique_ptr<TPngImage> png(new TPngImage); png->Assign(bitmap.get()); png->SaveToFile(path);
}
void __fastcall TMainForm::exportCsv(TObject*) {
    std::unique_ptr<TSaveDialog> dialog(new TSaveDialog(this)); dialog->Filter=L"CSV (*.csv)|*.csv";
    dialog->DefaultExt=L"csv"; dialog->FileName=L"points.csv";
    if (!dialog->Execute()) return;
    std::unique_ptr<TStreamWriter> writer(new TStreamWriter(dialog->FileName,false,TEncoding::UTF8,4096));
    writer->WriteLine(L"index,x,y,z,r,g,b,a,color_index,radius,lifetime_seconds");
    const auto invariant=TFormatSettings::Create(L"en-US"); int index=0;
    for (const auto& p:particles_) writer->WriteLine(IntToStr(++index)+L","+
        FloatToStr(p.position.x,invariant)+L","+FloatToStr(p.position.y,invariant)+L","+FloatToStr(p.position.z,invariant)+L","+
        FloatToStr(p.color.r,invariant)+L","+FloatToStr(p.color.g,invariant)+L","+FloatToStr(p.color.b,invariant)+L","+
        FloatToStr(p.color.a,invariant)+L","+IntToStr(int(p.colorIndex))+L","+FloatToStr(p.radius,invariant)+L","+IntToStr(int(p.lifetimeSeconds)));
}
void __fastcall TMainForm::savePicture(TObject*) {
    std::unique_ptr<TSaveDialog> dialog(new TSaveDialog(this)); dialog->Filter=L"PNG (*.png)|*.png";
    dialog->DefaultExt=L"png"; dialog->FileName=UnicodeString(lab1::containerName(currentContainer_))+L".png";
    if (!dialog->Execute()) return;
    viewer_->Repaint(); captureScene(dialog->FileName);
}
void __fastcall TMainForm::showHelp(TObject*) {
    ShowMessage(L"Выберите контейнер, вид объектов и число, затем нажмите «Сгенерировать».\n"
        L"Мышь с нажатой кнопкой вращает камеру, колесо меняет расстояние.\n"
        L"Переключатель камеры возвращает наружный или центральный вид.\n"
        L"Каждый объект гаснет через собственные 1–100 секунд. Пауза останавливает время.\n"
        L"Координаты любого объекта доступны по его номеру; все данные сохраняются в CSV.\n"
        L"При измерении FPS число объектов сохраняется: время свечения временно остановлено.");
}
void __fastcall TMainForm::showAbout(TObject*) {
    ShowMessage(L"Лабораторная №1. Генерация точек в объёме контейнера.\n"
        L"C++Builder / GLScene. Основа: AstrobloQ.\nhttps://github.com/glscene/AstrobloQ\n"
        L"Исходники лабораторной:\nhttps://github.com/danil-890/lab1-ravnomernoe-raspredelenie-tochek\n\n"
        L"OpenGL: "+renderer_+L"\n"+glVersion_);
}
void __fastcall TMainForm::exitApplication(TObject*) { Close(); }
void __fastcall TMainForm::closing(TObject*,TCloseAction&) { ready_=false; cadencer_->Enabled=false; }
