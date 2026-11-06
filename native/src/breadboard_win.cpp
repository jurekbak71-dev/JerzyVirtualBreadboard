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
#include <commdlg.h>
#include <sstream>
#include "audio.hpp"
#include "circuit.hpp"
#include "spice_render.hpp"
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
namespace {
ComPtr<ICoreWebView2Controller> controller;
ComPtr<ICoreWebView2> webview;
HWND windowHandle{};
bool bridgeSmoke=false;
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

std::wstring wideError(const std::string& message){
 const int length=MultiByteToWideChar(CP_UTF8,0,message.c_str(),-1,nullptr,0);
 if(length<=0)return L"Nieznany blad";
 std::wstring result(static_cast<size_t>(length),L'\0');
 MultiByteToWideChar(CP_UTF8,0,message.c_str(),-1,result.data(),length);
 result.resize(static_cast<size_t>(length-1));return result;
}
bool configureSolver(){
 const auto base=appDirectory();
 for(const auto& candidate:{base/L"ngspice"/L"Spice64"/L"bin",base/L"Spice64"/L"bin"}){
  if(!std::filesystem::exists(candidate/L"ngspice_con.exe"))continue;
  const auto library=candidate.parent_path()/L"share"/L"ngspice";
  if(!std::filesystem::exists(library/L"scripts"/L"spinit"))continue;
  wchar_t existing[32768]{};GetEnvironmentVariableW(L"PATH",existing,32768);
  const auto newPath=candidate.wstring()+L";"+existing;
  SetEnvironmentVariableW(L"PATH",newPath.c_str());
  SetEnvironmentVariableW(L"SPICE_LIB_DIR",library.c_str());
  SetEnvironmentVariableW(L"SPICE_EXEC_DIR",candidate.c_str());
  return true;
 }
 return false;
}
void sendToEditor(const std::wstring& message){
 if(webview)webview->PostWebMessageAsString(message.c_str());
}
void simulateFromEditor(HWND hwnd,const std::wstring& request){
 if(request.size()<9 || request.rfind(L"VBAL_SIM\n",0)!=0)return;
 try{
  std::wstring body=request.substr(9);
  const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,body.c_str(),static_cast<int>(body.size()),nullptr,0,nullptr,nullptr);
  if(bytes<=0 || bytes>65536)throw std::runtime_error("Invalid circuit size");
  std::string source(static_cast<size_t>(bytes),'\0');
  WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,body.c_str(),static_cast<int>(body.size()),source.data(),bytes,nullptr,nullptr);
  const auto circuit=vbal::parseEditorSpiceElements(source);
  if(!configureSolver())throw std::runtime_error("Bundled ngspice 47 is not available");
  wchar_t selected[32768]{};
  OPENFILENAMEW cfg{};cfg.lStructSize=sizeof(cfg);cfg.hwndOwner=hwnd;cfg.lpstrFile=selected;cfg.nMaxFile=32768;
  cfg.lpstrFilter=L"Pliki audio WAV\0*.wav\0\0";cfg.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
  if(!GetOpenFileNameW(&cfg)){sendToEditor(L"ANULOWANO: Nie wybrano pliku WAV.");return;}
  const auto path=std::filesystem::path(selected);
  const auto output=path.parent_path()/(path.stem().wstring()+L"_BREADBOARD.wav");
  sendToEditor(L"Przetwarzanie WAV przez ngspice...");
  auto audio=vbal::readPcmWav(path);
  if(audio.channels<1||audio.samples.size()/audio.channels>500000)
   throw std::runtime_error("Maximum 500000 frames per channel in SPICE mode");
  audio=vbal::renderGraphWithNgspice(audio,circuit);
  vbal::writePcm16Wav(output,audio);
  sendToEditor(L"GOTOWE: "+output.wstring());
  MessageBoxW(hwnd,(L"Zapisano wynik obwodu:\n"+output.wstring()).c_str(),L"Breadboard — SPICE",MB_OK|MB_ICONINFORMATION);
 }catch(const std::exception& e){
  sendToEditor(L"BLAD: "+wideError(e.what()));
  MessageBoxW(hwnd,wideError(e.what()).c_str(),L"Blad symulacji SPICE",MB_OK|MB_ICONERROR);
 }
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
    EventRegistrationToken messageToken{};
    webview->add_WebMessageReceived(
      Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [hwnd](ICoreWebView2*,ICoreWebView2WebMessageReceivedEventArgs* args)->HRESULT{
          LPWSTR raw=nullptr;
          if(SUCCEEDED(args->TryGetWebMessageAsString(&raw))&&raw){
            const std::wstring request(raw);
            CoTaskMemFree(raw);
            if(bridgeSmoke && request==L"VBAL_PING"){
              PostQuitMessage(0);
            }else if(!bridgeSmoke){
              simulateFromEditor(hwnd,request);
            }
          }
          return S_OK;
        }).Get(),&messageToken);
    if(bridgeSmoke){
      EventRegistrationToken navigationToken{};
      webview->add_NavigationCompleted(
        Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [](ICoreWebView2* sender,ICoreWebView2NavigationCompletedEventArgs* args)->HRESULT{
            BOOL success=FALSE;args->get_IsSuccess(&success);
            if(!success){PostQuitMessage(6);return S_OK;}
            sender->ExecuteScript(L"window.chrome.webview.postMessage('VBAL_PING')",nullptr);
            return S_OK;
          }).Get(),&navigationToken);
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
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR commandLine,int show){
 bridgeSmoke=commandLine && std::wstring(commandLine).find(L"--bridge-smoke")!=std::wstring::npos;
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
