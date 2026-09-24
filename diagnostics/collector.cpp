#include <windows.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <devpkey.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "Pairing.h"

namespace {
constexpr wchar_t Target[]=L"HID\\WCOM016C&Col01\\5&a6b5543&0&0000";
struct Device {
    HANDLE handle=INVALID_HANDLE_VALUE;
    ~Device(){if(handle!=INVALID_HANDLE_VALUE) CloseHandle(handle);}
    Device()=default;Device(const Device&)=delete;Device& operator=(const Device&)=delete;
    void Open(const wchar_t* name){handle=CreateFileW(name,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(handle==INVALID_HANDLE_VALUE)throw std::runtime_error("Observador indisponivel. Nenhum driver foi instalado.");}
    DWORD Ioctl(DWORD code,void* input,DWORD inSize,void* output,DWORD outSize){
        DWORD done=0;if(!DeviceIoControl(handle,code,input,inSize,output,outSize,&done,nullptr))
            throw std::runtime_error("Falha no canal de diagnostico: "+std::to_string(GetLastError()));return done;
    }
    void Stop() noexcept{DWORD n=0;if(handle!=INVALID_HANDLE_VALUE)DeviceIoControl(handle,GALAXY_DIAG_STOP,nullptr,0,nullptr,0,&n,nullptr);}
};
std::vector<std::wstring> Stack(){
    DEVINST node{};wchar_t id[128]{};wcscpy_s(id,Target);
    if(CM_Locate_DevNodeW(&node,id,CM_LOCATE_DEVNODE_NORMAL)!=CR_SUCCESS)throw std::runtime_error("COL01 ausente.");
    ULONG status=0,problem=0;
    if(CM_Get_DevNode_Status(&status,&problem,node,0)!=CR_SUCCESS || problem || !(status&DN_STARTED))
        throw std::runtime_error("COL01 nao esta saudavel/iniciado.");
    DEVPROPTYPE type=0;ULONG bytes=0;
    if(CM_Get_DevNode_PropertyW(node,&DEVPKEY_Device_Stack,&type,nullptr,&bytes,0)!=CR_BUFFER_SMALL ||
        bytes<4 || bytes>65536 || bytes%sizeof(wchar_t))throw std::runtime_error("Pilha nao consultavel.");
    std::vector<wchar_t> data(bytes/sizeof(wchar_t));
    if(CM_Get_DevNode_PropertyW(node,&DEVPKEY_Device_Stack,&type,reinterpret_cast<PBYTE>(data.data()),&bytes,0)!=CR_SUCCESS ||
        type!=DEVPROP_TYPE_STRING_LIST)throw std::runtime_error("Pilha invalida.");
    std::vector<std::wstring> result;size_t pos=0;
    while(pos<data.size() && data[pos]){
        size_t end=pos;while(end<data.size() && data[end])++end;
        if(end==data.size())throw std::runtime_error("Pilha truncada.");
        result.emplace_back(data.data()+pos,end-pos);pos=end+1;
    }
    return result;
}
void Save(const std::filesystem::path& file,const std::vector<GALAXY_DIAG_RECORD>& records){
    std::ofstream out(file);out.exceptions(std::ios::badbit|std::ios::failbit);
    out<<"session,sequence,request_token,begin_100ns,end_100ns,status,length,valid,raw_hex\n";
    for(const auto& r:records){
        out<<r.Session<<','<<r.Sequence<<','<<r.Token<<','<<r.Begin<<','<<r.End<<','<<r.Status<<','<<r.Length<<','<<unsigned(r.Valid)<<',';
        if(r.Valid)for(unsigned i=0;i<15;++i){if(i)out<<' ';out<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(r.Report[i]);}
        out<<std::dec<<'\n';
    }
    out.flush();
}
struct Stream {
    std::vector<GALAXY_DIAG_RECORD> records;
    unsigned long long lost=0,started=0,completed=0,lastSequence=0;
    void Drain(Device& device,unsigned role,unsigned long long session,bool requireActive){
        GALAXY_DIAG_BATCH_DATA b{};
        if(device.Ioctl(GALAXY_DIAG_DRAIN,nullptr,0,&b,sizeof(b))!=sizeof(b) ||
            b.Version!=GALAXY_DIAG_VERSION || b.Role!=role || b.Count>GALAXY_DIAG_BATCH ||
            b.Session!=session || b.Attached!=1 || b.Ready!=1 || b.Active>1 || (requireActive && !b.Active))
            throw std::runtime_error("Protocolo, sessao ou estado mudou; captura inconclusiva.");
        lost=b.Lost;started=b.Started;completed=b.Completed;
        if(records.size()+b.Count>200000)throw std::runtime_error("Limite de memoria atingido; captura interrompida.");
        for(unsigned i=0;i<b.Count;++i){const auto& r=b.Records[i];
            if(r.Session!=session || r.Sequence<=lastSequence || r.End<r.Begin || r.Valid>1 || (r.Valid && r.Length!=15))
                throw std::runtime_error("Registro inconsistente.");
            lastSequence=r.Sequence;records.push_back(r);
        }
    }
};
void PressureSummary(std::ostream& out,const char* label,const Stream& stream){
    size_t contacts=0,saturated=0;unsigned min=4095,max=0;
    for(const auto& r:stream.records)if(diag::ValidPen(r) && r.Report[1]==0x2c){
        const unsigned p=unsigned(r.Report[6])|(unsigned(r.Report[7])<<8);
        ++contacts;if(p==4095)++saturated;min=std::min(min,p);max=std::max(max,p);
    }
    out<<label<<": reports="<<stream.records.size()<<" lost="<<stream.lost<<" started="<<stream.started
       <<" completed="<<stream.completed<<" contacts_0x2c="<<contacts<<" saturated="<<saturated;
    if(contacts)out<<" raw="<<min<<".."<<max;out<<'\n';
}
}
int wmain(int argc,wchar_t** argv){
    try {
        if(argc!=2 && argc!=3){std::cout<<"--preflight | --capture NEW_OUTPUT_DIRECTORY\n";return 2;}
        const bool capture=argc==3 && std::wstring(argv[1])==L"--capture";
        if(!capture && !(argc==2 && std::wstring(argv[1])==L"--preflight"))return 2;
        const auto stack=Stack();
        for(const auto& s:stack)std::wcout<<s<<L'\n';
        if(!diag::StackAllowed(stack)){
            std::wcerr<<L"BLOQUEADO: exige A > PenS2Helper > B > mshidkmdf, sem Corrector. Nenhuma alteracao feita.\n";return 3;
        }
        if(!capture){std::cout<<"Pilha esperada. Leituras e pareamento ainda nao comprovados.\n";return 0;}
        // Exclusive creation: never reuse or overwrite an existing capture folder.
        const std::filesystem::path folder(argv[2]);
        if(!std::filesystem::create_directory(folder))throw std::runtime_error("Diretorio ja existe; nao sobrescrever.");
        Device a,b; a.Open(L"\\\\.\\GalaxyPenDiagA1");b.Open(L"\\\\.\\GalaxyPenDiagB1");
        GALAXY_DIAG_START_DATA start{GALAXY_DIAG_VERSION,0,0,0,0};
        if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&start.Session),24,BCRYPT_USE_SYSTEM_PREFERRED_RNG)!=0)
            throw std::runtime_error("Gerador aleatorio indisponivel.");
        if(!start.Session || !(start.Key0|start.Key1))throw std::runtime_error("Sessao aleatoria invalida.");
        const auto session=start.Session;
        try {a.Ioctl(GALAXY_DIAG_START,&start,sizeof(start),nullptr,0);b.Ioctl(GALAXY_DIAG_START,&start,sizeof(start),nullptr,0);}
        catch(...){SecureZeroMemory(&start,sizeof(start));throw;}
        SecureZeroMemory(&start,sizeof(start));
        Stream upper,lower;std::string issue;
        std::cout<<"Observacao por 30 segundos. Use apenas pressao confortavel. Nao corrige nem injeta entrada.\n";
        try {
            const auto end=GetTickCount64()+30000;auto nextCheck=GetTickCount64();
            while(GetTickCount64()<end){
                if(GetTickCount64()>=nextCheck){if(Stack()!=stack)throw std::runtime_error("Pilha mudou.");nextCheck=GetTickCount64()+1000;}
                upper.Drain(a,1,session,true);lower.Drain(b,2,session,true);Sleep(10);
            }
        } catch(const std::exception& e){issue=e.what();}
        a.Stop();b.Stop();
        try { // Drain bounded residual data; in-flight reads at stop are not recorded.
            for(unsigned i=0;i<GALAXY_DIAG_RING/GALAXY_DIAG_BATCH+1;++i){upper.Drain(a,1,session,false);lower.Drain(b,2,session,false);}
            if(Stack()!=stack)throw std::runtime_error("Pilha mudou ao encerrar.");
        }catch(const std::exception& e){issue+=std::string(" ")+e.what();}
        Save(folder/L"after-samsung.csv",upper.records);Save(folder/L"before-samsung.csv",lower.records);
        std::ofstream report(folder/L"result.txt");report.exceptions(std::ios::badbit|std::ios::failbit);
        report<<"Session: "<<session<<"\nVerified stack: A > PenS2Helper > B > mshidkmdf\n";
        report<<"Issue: "<<(issue.empty()?"none":issue)<<'\n';
        PressureSummary(report,"Before Samsung",lower);PressureSummary(report,"After Samsung",upper);
        const auto c=diag::Compare(upper.records,lower.records);
        report<<"Matched="<<c.matched<<" equal="<<c.equal<<" pressureChanged="<<c.pressureChanged
              <<" otherChanged="<<c.otherChanged<<" invalid="<<c.invalid<<" ambiguous="<<c.ambiguous
              <<" unpairedUpper="<<c.unpairedUpper<<" unpairedLower="<<c.unpairedLower<<'\n';
        const bool complete=issue.empty() && !upper.lost && !lower.lost && c.matched>0 && !c.invalid && !c.ambiguous;
        report<<"Usable matched subset: "<<(complete?"yes":"no / inconclusive")<<'\n';
        report<<"Unpaired requests are not evidence of changed pressure. Equal pairs do not prove compatibility.\n";
        report.flush();std::cout<<"Captura salva. Interpretar result.txt; nenhuma conclusao automatica sobre hardware.\n";
        return complete?0:4;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
