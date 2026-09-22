#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

constexpr int IDC_START=1001;
constexpr int IDC_STOP=1002;
constexpr int IDC_SESSION=1003;
constexpr int IDC_DESC=1004;

HWND sessionBox=nullptr;
HWND descBox=nullptr;
std::ofstream logFile;
bool recording=false;
unsigned long events=0;

std::string stamp(){
 auto t=std::time(nullptr); std::tm tm{}; localtime_s(&tm,&t);
 std::ostringstream s; s<<std::put_time(&tm,"%Y%m%d-%H%M%S"); return s.str();
}

std::string narrow(const std::wstring& w){
 if(w.empty()) return "";
 int n=WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);
 std::string r(n,0); WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),r.data(),n,nullptr,nullptr); return r;
}

std::string csv(const std::string& s){return "\""+s+"\"";}

void startCapture(){
 wchar_t s[128]{},d[256]{};
 GetWindowTextW(sessionBox,s,128);
 GetWindowTextW(descBox,d,256);
 fs::create_directories("captures");
 logFile.open("captures/"+narrow(s)+"_"+stamp()+".csv");
 logFile<<"session,description,timestamp,event\n";
 logFile<<csv(narrow(s))<<","<<csv(narrow(d))<<","<<stamp()<<",START\n";
 recording=true;
 events=0;
}

void stopCapture(){
 if(logFile){logFile<<"STOP\n";logFile.close();}
 recording=false;
}

LRESULT CALLBACK wnd(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:
  sessionBox=CreateWindowW(L"EDIT",L"S_Pen",WS_CHILD|WS_VISIBLE|WS_BORDER,20,20,220,25,h,(HMENU)IDC_SESSION,0,0);
  descBox=CreateWindowW(L"EDIT",L"Teste da caneta",WS_CHILD|WS_VISIBLE|WS_BORDER,20,55,220,25,h,(HMENU)IDC_DESC,0,0);
  CreateWindowW(L"BUTTON",L"Iniciar captura",WS_CHILD|WS_VISIBLE,260,20,130,30,h,(HMENU)IDC_START,0,0);
  CreateWindowW(L"BUTTON",L"Parar",WS_CHILD|WS_VISIBLE,260,55,130,30,h,(HMENU)IDC_STOP,0,0);
  return 0;
 case WM_COMMAND:
  if(LOWORD(w)==IDC_START) startCapture();
  if(LOWORD(w)==IDC_STOP) stopCapture();
  return 0;
 case WM_PAINT:{
  PAINTSTRUCT ps{}; HDC dc=BeginPaint(h,&ps);
  Rectangle(dc,20,110,460,350);
  TextOutW(dc,40,130,L"AREA DE TESTE DA CANETA",25);
  TextOutW(dc,40,160,L"Sessao e descricao serao usadas no CSV",37);
  EndPaint(h,&ps); return 0;}
 case WM_DESTROY:
  stopCapture(); PostQuitMessage(0); return 0;
 }
 return DefWindowProcW(h,m,w,l);
}

int WINAPI wWinMain(HINSTANCE i,HINSTANCE,LPWSTR,int){
 WNDCLASSW c{}; c.lpfnWndProc=wnd; c.hInstance=i; c.lpszClassName=L"GalaxyPenMapper"; RegisterClassW(&c);
 CreateWindowW(c.lpszClassName,L"Galaxy Pen Event Monitor P002.2",WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,520,430,0,0,i,0);
 MSG msg{}; while(GetMessageW(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);} return 0;
}
