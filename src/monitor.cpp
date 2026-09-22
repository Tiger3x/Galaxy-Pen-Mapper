#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

constexpr int IDC_START=1001;
constexpr int IDC_STOP=1002;
constexpr int IDC_SESSION=1003;
constexpr int IDC_DESC=1004;

std::ofstream logFile;
bool recording=false;
std::wstring session=L"S_Pen";
std::wstring description=L"";
unsigned long events=0;
HWND sessionBox=nullptr;
HWND descBox=nullptr;

std::string now(){
 auto t=std::time(nullptr); std::tm tm{}; localtime_s(&tm,&t);
 std::ostringstream s; s<<std::put_time(&tm,"%Y%m%d-%H%M%S"); return s.str();
}
std::string utf8(const std::wstring& w){
 int n=WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);
 std::string r(n,0); WideCharToMultiByte(CP_UTF8,0,w.data(),(int)w.size(),r.data(),n,nullptr,nullptr); return r;
}
std::string csv(const std::string& s){return "\""+s+"\"";}
void start(){
 wchar_t b[128]; GetWindowTextW(sessionBox,b,128); session=b;
 GetWindowTextW(descBox,b,128); description=b;
 fs::create_directories("captures");
 std::string file="captures/"+utf8(session)+"_"+now()+".csv";
 logFile.open(file);
 logFile<<"session,description,timestamp,event\n";
 logFile<<csv(utf8(session))<<","<<csv(utf8(description))<<","<<now()<<",START\n";
 recording=true;
}
void stop(){if(logFile){logFile<<csv(utf8(session))<<","<<csv(utf8(description))<<","<<now()<<",STOP\n";logFile.close();} recording=false;}
LRESULT CALLBACK proc(HWND h,UINT m,WPARAM w,LPARAM l){
 switch(m){
 case WM_CREATE:
  sessionBox=CreateWindowW(L"EDIT",L"S_Pen",WS_CHILD|WS_VISIBLE|WS_BORDER,20,20,220,25,h,(HMENU)IDC_SESSION,0,0);
  descBox=CreateWindowW(L"EDIT",L"teste",WS_CHILD|WS_VISIBLE|WS_BORDER,20,55,220,25,h,(HMENU)IDC_DESC,0,0);
  CreateWindowW(L"BUTTON",L"Iniciar captura",WS_CHILD|WS_VISIBLE,260,20,130,30,h,(HMENU)IDC_START,0,0);
  CreateWindowW(L"BUTTON",L"Parar",WS_CHILD|WS_VISIBLE,260,55,130,30,h,(HMENU)IDC_STOP,0,0);
  return 0;
 case WM_COMMAND:
  if(LOWORD(w)==IDC_START) start();
  if(LOWORD(w)==IDC_STOP) stop();
  return 0;
 case WM_DESTROY: stop(); PostQuitMessage(0); return 0;
 }
 return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE i,HINSTANCE,LPWSTR,int){
 WNDCLASSW c{}; c.lpfnWndProc=proc; c.hInstance=i; c.lpszClassName=L"GalaxyPenMapper"; RegisterClassW(&c);
 HWND h=CreateWindowW(c.lpszClassName,L"Galaxy Pen Event Monitor P002.2",WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,500,200,0,0,i,0);
 MSG msg{}; while(GetMessageW(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);} return 0;
}