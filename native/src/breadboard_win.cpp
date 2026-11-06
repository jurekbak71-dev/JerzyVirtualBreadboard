#ifdef _WIN32
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wrl.h>
#include <filesystem>
#include <string>
#include <system_error>
#include <shlwapi.h>
#include <WebView2.h>
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
namespace {
ComPtr<ICoreWebView2Controller> controller;
ComPtr<ICoreWebView2> webview;
HWND windowHandle{};
void fail(HWND parent,const wchar_t* text) {
 MessageBoxW(parent,text,L"Virtual Breadboard — WebView2",MB_OK|MB_ICONERROR);
}
std::filesystem::path appDirectory(){
 wchar_t name[32768]{};if(!GetModuleFileNameW(nullptr,name,32768))return {};
 return std::filesystem::path(name).parent_path();
}
std::wstring userDataPath(){
 wchar_t data[32768]{};
 auto count=GetEnvironmentVariableW(L"LOCALAPPDATA",data,32768);
 auto location=(count>0&&count<32768)?std::filesystem::path(data):std::filesystem::temp_directory_path();
 auto dir=location/L"JerzyVirtualBreadboard"/L"WebView2";
 std::error_code ec;std::filesystem::create_directories(dir,ec);
 return dir.wstring();
}
void openEditor(HWND hwnd) {
 auto page=appDirectory()/L"web"/L"editor.html";
 if(!std::filesystem::is_regular_file(page)){
  fail(hwnd,L"Brak pliku web\\editor.html. Rozpakuj caly pakiet ZIP, zachowujac folder web.");
  return;
 }
 LPWSTR available=nullptr;
 auto runtimeStatus=GetAvailableCoreWebView2BrowserVersionString(nullptr,&available);
 if(available)CoTaskMemFree(available);
 if(FAILED(runtimeStatus)){
  fail(hwnd,L"Nie znaleziono Microsoft Edge WebView2 Runtime. Zainstaluj oficjalny WebView2 Runtime od Microsoft.");
  return;
 }
 const auto profile=userDataPath();
 const HRESULT result=CreateCoreWebView2EnvironmentWithOptions(nullptr,profile.c_str(),nullptr,
 Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
 [hwnd,page](HRESULT hr,ICoreWebView2Environment* env)->HRESULT{
  if(FAILED(hr)||!env){fail(hwnd,L"Nie udalo sie utworzyc srodowiska WebView2.");return S_OK;}
  return env->CreateCoreWebView2Controller(hwnd,
   Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
   [hwnd,page](HRESULT created,ICoreWebView2Controller* c)->HRESULT{
    if(FAILED(created)||!c){fail(hwnd,L"Nie udalo sie utworzyc kontrolki WebView2.");return S_OK;}
    controller=c;
    if(FAILED(controller->get_CoreWebView2(&webview))||!webview){
     fail(hwnd,L"Nie udalo sie uruchomic silnika WebView2.");return S_OK;
    }
    RECT bounds{};GetClientRect(hwnd,&bounds);controller->put_Bounds(bounds);
    wchar_t url[32768]{};DWORD length=32768;
    const auto file=page.wstring();
    if(FAILED(UrlCreateFromPathW(file.c_str(),url,&length,0))){
     fail(hwnd,L"Nie mozna otworzyc lokalnej strony HTML.");return S_OK;
    }
    webview->Navigate(url);
    return S_OK;
   }).Get());
 }).Get());
 if(FAILED(result))fail(hwnd,L"Nie udalo sie zainicjowac WebView2.");
}
LRESULT CALLBACK windowProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam){
 switch(msg){
 case WM_CREATE:
  PostMessageW(hwnd,WM_APP+1,0,0);
  return 0;
 case WM_APP+1:
  openEditor(hwnd);
  return 0;
 case WM_SIZE:
  if(controller){RECT bounds{};GetClientRect(hwnd,&bounds);controller->put_Bounds(bounds);}
  return 0;
 case WM_DESTROY:
  webview.Reset();controller.Reset();PostQuitMessage(0);return 0;
 default:return DefWindowProcW(hwnd,msg,wParam,lParam);
 }
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show){
 const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 if(FAILED(initialized))return 2;
 WNDCLASSW cls{};cls.lpfnWndProc=windowProc;cls.hInstance=instance;
 cls.lpszClassName=L"VirtualBreadboardWebView2";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
 cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
 if(!RegisterClassW(&cls)){CoUninitialize();return 3;}
 windowHandle=CreateWindowExW(0,cls.lpszClassName,L"Virtual Breadboard Audio Lab — Edytor obwodow",
 WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,1380,900,
 nullptr,nullptr,instance,nullptr);
 if(!windowHandle){CoUninitialize();return 4;}
 ShowWindow(windowHandle,show);UpdateWindow(windowHandle);
 MSG message{};while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
 CoUninitialize();return static_cast<int>(message.wParam);
}
#endif
