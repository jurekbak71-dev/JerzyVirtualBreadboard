#ifdef _WIN32
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <string>
#include <stdexcept>
#include "audio.hpp"
namespace {
constexpr int Select=1001,Process=1002,Presets=1003;
HWND pathEdit{},status{},presets{},runButton{};
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
void process(HWND parent){
 int n=GetWindowTextLengthW(pathEdit);
 if(n==0){MessageBoxW(parent,L"Wybierz plik WAV.",L"Virtual Breadboard",MB_OK);return;}
 std::wstring name(static_cast<size_t>(n)+1,L'\0');GetWindowTextW(pathEdit,name.data(),n+1);name.resize(static_cast<size_t>(n));
 EnableWindow(runButton,FALSE);SetWindowTextW(status,L"Przetwarzanie...");
 try{
  auto file=std::filesystem::path(name);
  auto output=file.parent_path()/(file.stem().wstring()+L"_RC.wav");
  const double caps[]={1e-8,1e-7,1e-6};
  auto idx=static_cast<int>(SendMessageW(presets,CB_GETCURSEL,0,0));if(idx<0||idx>2)idx=0;
  auto audio=vbal::readPcmWav(file);
  vbal::rcLowpass(audio,10000.,caps[idx]);
  vbal::writePcm16Wav(output,audio);
  SetWindowTextW(status,L"Wynik zapisany jako *_RC.wav obok wejscia.");
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
   runButton=add(hwnd,L"BUTTON",L"Przetworz WAV",BS_DEFPUSHBUTTON,20,225,190,44,Process);
   status=add(hwnd,L"STATIC",L"Wybierz plik WAV, aby uruchomic test.",0,20,288,580,26);
   add(hwnd,L"STATIC",L"Prototyp offline — jeszcze bez SPICE i audio na zywo.",0,20,335,580,24);
   return 0;
  }
  case WM_COMMAND:
   if(HIWORD(w)==BN_CLICKED){if(LOWORD(w)==Select)select(hwnd);else if(LOWORD(w)==Process)process(hwnd);}
   return 0;
  case WM_DESTROY:PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE app,HINSTANCE,LPWSTR,int show){
 WNDCLASSW cls{};cls.lpfnWndProc=wndProc;cls.hInstance=app;cls.lpszClassName=L"VBALWindow";cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
 if(!RegisterClassW(&cls))return 1;
 HWND wnd=CreateWindowW(cls.lpszClassName,L"Virtual Breadboard Audio Lab",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,635,420,nullptr,nullptr,app,nullptr);
 if(!wnd)return 1;ShowWindow(wnd,show);UpdateWindow(wnd);
 MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return static_cast<int>(msg.wParam);
}
#endif
