#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
namespace fs=std::filesystem;
constexpr int START=1001,STOP=1002,SESSION=1003,DESC=1004;
HWND sBox,dBox;std::ofstream file;bool rec=false;unsigned events=0;std::wstring live=L"Waiting";
std::string n8(std::wstring w){int n=WideCharToMultiByte(CP_UTF8,0,w.data(),w.size(),0,0,0,0);std::string r(n,0);WideCharToMultiByte(CP_UTF8,0,w.data(),w.size(),r.data(),n,0,0);return r;}
std::string ts(){auto t=time(0);tm m{};localtime_s(&m,&t);char b[40];strftime(b,40,"%Y%m%d-%H%M%S",&m);return b;}
void log(char* e){if(!rec)return;file<<e<<","<<ts()<<"\n";events++;live=L"Events: "+std::to_wstring(events);}
void start(){wchar_t a[128];GetWindowTextW(sBox,a,128);fs::create_directories("captures");file.open("captures/"+n8(a)+"_"+ts()+".csv");file<<"event,time\n";rec=1;events=0;live=L"Recording";}
void stop(){if(file)file.close();rec=0;}
LRESULT CALLBACK p(HWND h,UINT m,WPARAM w,LPARAM l){switch(m){case WM_CREATE:sBox=CreateWindowW(L"EDIT",L"S_Pen",WS_CHILD|WS_VISIBLE|WS_BORDER,20,20,200,25,h,(HMENU)SESSION,0,0);dBox=CreateWindowW(L"EDIT",L"Test",WS_CHILD|WS_VISIBLE|WS_BORDER,20,55,200,25,h,(HMENU)DESC,0,0);CreateWindowW(L"BUTTON",L"Start",WS_CHILD|WS_VISIBLE,250,20,100,30,h,(HMENU)START,0,0);CreateWindowW(L"BUTTON",L"Stop",WS_CHILD|WS_VISIBLE,250,55,100,30,h,(HMENU)STOP,0,0);return 0;case WM_COMMAND:if(LOWORD(w)==START)start();if(LOWORD(w)==STOP)stop();return 0;case WM_POINTERDOWN:log("DOWN");InvalidateRect(h,0,1);return 0;case WM_POINTERUP:log("UP");InvalidateRect(h,0,1);return 0;case WM_POINTERUPDATE:log("UPDATE");InvalidateRect(h,0,1);return 0;case WM_PAINT:{PAINTSTRUCT ps;HDC d=BeginPaint(h,&ps);TextOutW(d,20,110,live.c_str(),live.size());Rectangle(d,20,150,450,350);TextOutW(d,40,170,L"AREA DE TESTE DA CANETA",25);EndPaint(h,&ps);return 0;}case WM_DESTROY:stop();PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,w,l);}
int WINAPI wWinMain(HINSTANCE i,HINSTANCE,LPWSTR,int){WNDCLASSW c{};c.lpfnWndProc=p;c.hInstance=i;c.lpszClassName=L"GPM";RegisterClassW(&c);CreateWindowW(L"GPM",L"Galaxy Pen Event Monitor P002.2",WS_OVERLAPPEDWINDOW|WS_VISIBLE,100,100,500,450,0,0,i,0);MSG m{};while(GetMessageW(&m,0,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}return 0;}
