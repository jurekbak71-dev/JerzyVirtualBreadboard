#ifdef _WIN32
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <filesystem>
#include <string>
#include <stdexcept>
#include "audio.hpp"
#include "spice_render.hpp"
#include "circuit.hpp"
#include <fstream>
namespace {
constexpr int Select=1001,Process=1002,Presets=1003,SpiceMode=1004,DiodeMode=1005,ExportCircuit=1006,OpenBoard=1007;
HWND pathEdit{},status{},presets{},runButton{},spiceCheckbox{},diodeCheckbox{};
std::wstring wide(const std::string& s) {
 int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);
 if(n<1)return L"Nieznany blad";
 std::wstring w(static_cast<size_t>(n),L'\0');
 MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,w.data(),n);
 w.pop_back();return w;
}
HWND add(HWND parent,const wchar_t* cls,const wchar_t* label,DWORD style,int x,int y,int w,int h,int id=0){
 HWND c=CreateWindowExW(0,cls,label,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
 SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return c;
}
void select(HWND parent) {
 wchar_t selected[32768]{};
 OPENFILENAMEW cfg{};cfg.lStructSize=sizeof(cfg);cfg.hwndOwner=parent;cfg.lpstrFile=selected;cfg.nMaxFile=32768;cfg.lpstrFilter=L"Pliki WAV\0*.wav\0Wszystkie pliki\0*.*\0\0";cfg.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
 if(GetOpenFileNameW(&cfg)){SetWindowTextW(pathEdit,selected);SetWindowTextW(status,L"Gotowe do przetwarzania");}
}
bool hasBundledSolver(){
 wchar_t raw[32768]{};
 if(!GetModuleFileNameW(nullptr,raw,32768))return false;
 auto base=std::filesystem::path(raw).parent_path();
 for(const auto& candidate:{base/L"Spice64"/L"bin",base/L"ngspice"/L"Spice64"/L"bin",base/L"ngspice",base}){
  if(std::filesystem::exists(candidate/L"ngspice_con.exe")){
   wchar_t oldPath[32768]{};GetEnvironmentVariableW(L"PATH",oldPath,32768);
   auto updated=candidate.wstring()+L";"+oldPath;
   SetEnvironmentVariableW(L"PATH",updated.c_str());
   auto libDir=candidate.parent_path()/L"share"/L"ngspice";
   if(std::filesystem::exists(libDir/L"scripts"/L"spinit")){
    SetEnvironmentVariableW(L"SPICE_LIB_DIR",libDir.c_str());
    SetEnvironmentVariableW(L"SPICE_EXEC_DIR",candidate.c_str());
   }
   return true;
  }
 }
 return false;
}
void openBreadboard(HWND parent){
 wchar_t exe[32768]{};
 if(!GetModuleFileNameW(nullptr,exe,32768)){MessageBoxW(parent,L"Nie mozna znalezc katalogu aplikacji.",L"Blad",MB_OK);return;}
 const auto page=std::filesystem::path(exe).parent_path()/L"web"/L"editor.html";
 if(!std::filesystem::exists(page)){MessageBoxW(parent,L"Brak web/editor.html. Pobierz pelny ZIP z GitHub Actions.",L"Brak edytora",MB_OK|MB_ICONERROR);return;}
 auto result=reinterpret_cast<INT_PTR>(ShellExecuteW(parent,L"open",page.c_str(),nullptr,nullptr,SW_SHOWNORMAL));
 if(result<=32)MessageBoxW(parent,L"Nie udalo sie otworzyc edytora w przegladarce.",L"Blad",MB_OK|MB_ICONERROR);
}
void exportCircuit(HWND parent){
 wchar_t target[MAX_PATH]=L"VBAL_circuit.cir";
 OPENFILENAMEW cfg{};cfg.lStructSize=sizeof(cfg);cfg.hwndOwner=parent;cfg.lpstrFile=target;cfg.nMaxFile=MAX_PATH;
 cfg.lpstrFilter=L"Netlist SPICE (*.cir)\0*.cir\0\0";cfg.lpstrDefExt=L"cir";
 cfg.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
 if(!GetSaveFileNameW(&cfg))return;
 try{
  const double caps[]={1e-8,1e-7,1e-6};
  auto idx=static_cast<int>(SendMessageW(presets,CB_GETCURSEL,0,0));if(idx<0||idx>2)idx=0;
  auto clip=SendMessageW(diodeCheckbox,BM_GETCHECK,0,0)==BST_CHECKED;
  const auto graph=vbal::makeRcCircuit(10000.,caps[idx],clip);
  auto file=std::filesystem::path(target);
  auto csv=file.parent_path()/(file.stem().wstring()+L"_BOM.csv");
  auto wiring=file.parent_path()/(file.stem().wstring()+L"_connections.txt");
  for(const auto& entry:{std::pair{file,vbal::exportCircuitSpice(graph)},
                         std::pair{csv,vbal::exportCircuitBomCsv(graph)},
                         std::pair{wiring,vbal::exportCircuitWiring(graph)}}){
   std::ofstream out(entry.first,std::ios::binary);
   if(!out)throw std::runtime_error("Cannot write circuit export");
   out<<entry.second;
   if(!out)throw std::runtime_error("Cannot save circuit export");
  }
  SetWindowTextW(status,L"Wyeksportowano schemat, BOM i polaczenia.");
  MessageBoxW(parent,L"Zapisano: .cir, _BOM.csv oraz _connections.txt",L"Eksport obwodu",MB_OK|MB_ICONINFORMATION);
 }catch(const std::exception& e){MessageBoxW(parent,wide(e.what()).c_str(),L"Blad eksportu",MB_OK|MB_ICONERROR);}
}
void process(HWND parent){
 int n=GetWindowTextLengthW(pathEdit);
 if(n==0){MessageBoxW(parent,L"Wybierz plik WAV.",L"Virtual Breadboard",MB_OK);return;}
 std::wstring name(static_cast<size_t>(n)+1,L'\0');GetWindowTextW(pathEdit,name.data(),n+1);name.resize(static_cast<size_t>(n));
 EnableWindow(runButton,FALSE);SetWindowTextW(status,L"Przetwarzanie...");
 try{
  auto file=std::filesystem::path(name);
  const bool useSpice=SendMessageW(spiceCheckbox,BM_GETCHECK,0,0)==BST_CHECKED;
  const bool useDiode=SendMessageW(diodeCheckbox,BM_GETCHECK,0,0)==BST_CHECKED;
  if(useDiode&&!useSpice){MessageBoxW(parent,L"Wlacz symulacje SPICE, aby uzyc diod.",L"Virtual Breadboard",MB_OK);EnableWindow(runButton,TRUE);return;}
  auto output=file.parent_path()/(file.stem().wstring()+(useDiode?L"_SPICE_DIODE.wav":useSpice?L"_SPICE_RC.wav":L"_RC.wav"));
  const double caps[]={1e-8,1e-7,1e-6};
  auto idx=static_cast<int>(SendMessageW(presets,CB_GETCURSEL,0,0));if(idx<0||idx>2)idx=0;
  auto audio=vbal::readPcmWav(file);
  if(useSpice){
   if(!hasBundledSolver()) throw std::runtime_error("Nie znaleziono ngspice_con.exe. Pobierz pelna paczke z solverem.");
   if(audio.samples.size()/audio.channels>500000) throw std::runtime_error("SPICE: maksymalnie 500000 ramek audio na kanal na raz. Dla dluzszych nagran przygotowywany jest streaming.");
   audio=vbal::renderCircuitWithNgspice(audio,10000.,caps[idx],useDiode?vbal::CircuitMode::DiodeClipper:vbal::CircuitMode::RcLowpass);
  }else vbal::rcLowpass(audio,10000.,caps[idx]);
  vbal::writePcm16Wav(output,audio);
  SetWindowTextW(status,useDiode?L"Zapisano *_SPICE_DIODE.wav (diody).":useSpice?L"Zapisano *_SPICE_RC.wav (ngspice).":L"Zapisano *_RC.wav (model RC).");
  MessageBoxW(parent,(L"Zapisano:\n"+output.wstring()).c_str(),L"Zakonczono",MB_OK|MB_ICONINFORMATION);
 }catch(const std::exception& e){
  SetWindowTextW(status,L"Blad. Wybierz poprawny plik PCM WAV (16/24/32 bit).");
  MessageBoxW(parent,wide(e.what()).c_str(),L"Blad",MB_OK|MB_ICONERROR);
 }
 EnableWindow(runButton,TRUE);
}
LRESULT CALLBACK wndProc(HWND hwnd,UINT msg,WPARAM w,LPARAM l){
 switch(msg){
  case WM_CREATE:{
   add(hwnd,L"STATIC",L"Virtual Breadboard Audio Lab — test audio RC",0,20,16,580,26);
   add(hwnd,L"STATIC",L"Plik WAV:",0,20,55,570,24);
   pathEdit=add(hwnd,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,20,84,445,30);
   add(hwnd,L"BUTTON",L"Wybierz...",BS_PUSHBUTTON,478,82,120,34,Select);
   add(hwnd,L"STATIC",L"Filtr dolnoprzepustowy RC:",0,20,135,480,26);
   presets=add(hwnd,L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,20,165,385,200,Presets);
   SendMessageW(presets,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"1,59 kHz — R 10k, C 10nF"));
   SendMessageW(presets,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"159 Hz — R 10k, C 100nF"));
   SendMessageW(presets,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(L"15,9 Hz — R 10k, C 1uF"));
   SendMessageW(presets,CB_SETCURSEL,0,0);
   spiceCheckbox=add(hwnd,L"BUTTON",L"Symuluj fizyczny obwod w ngspice",BS_AUTOCHECKBOX,20,206,570,28,SpiceMode);
   diodeCheckbox=add(hwnd,L"BUTTON",L"Dwie przeciwsobne diody (nieliniowy clipping)",BS_AUTOCHECKBOX,20,232,570,28,DiodeMode);
   runButton=add(hwnd,L"BUTTON",L"Przetworz WAV",BS_DEFPUSHBUTTON,20,272,190,44,Process);
   add(hwnd,L"BUTTON",L"Eksportuj obwod + BOM",BS_PUSHBUTTON,228,272,205,44,ExportCircuit);
   add(hwnd,L"BUTTON",L"Otworz breadboard",BS_PUSHBUTTON,445,272,162,44,OpenBoard);
   status=add(hwnd,L"STATIC",L"Wybierz plik WAV, aby uruchomic test.",0,20,337,580,26);
   add(hwnd,L"STATIC",L"Tryb SPICE wymaga ngspice_con.exe w dolaczonej paczce.",0,20,381,580,24);
   return 0;
  }
  case WM_COMMAND:
   if(HIWORD(w)==BN_CLICKED){if(LOWORD(w)==Select)select(hwnd);else if(LOWORD(w)==Process)process(hwnd);else if(LOWORD(w)==ExportCircuit)exportCircuit(hwnd);else if(LOWORD(w)==OpenBoard)openBreadboard(hwnd);else if(LOWORD(w)==DiodeMode && SendMessageW(diodeCheckbox,BM_GETCHECK,0,0)==BST_CHECKED)SendMessageW(spiceCheckbox,BM_SETCHECK,BST_CHECKED,0);}
   return 0;
  case WM_DESTROY:PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE app,HINSTANCE,LPWSTR,int show){
 WNDCLASSW cls{};cls.lpfnWndProc=wndProc;cls.hInstance=app;cls.lpszClassName=L"VBALWindow";cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
 if(!RegisterClassW(&cls))return 1;
 HWND wnd=CreateWindowW(cls.lpszClassName,L"Virtual Breadboard Audio Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,635,475,nullptr,nullptr,app,nullptr);
 if(!wnd)return 1;ShowWindow(wnd,show);UpdateWindow(wnd);
 MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return static_cast<int>(msg.wParam);
}
#endif
