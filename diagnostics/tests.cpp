#include <windows.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include "Pairing.h"
#include "../driver/shared/GalaxyPenDiagToken.h"
void Check(bool ok,const char* label){if(!ok){std::cerr<<label<<'\n';std::exit(1);}}
GALAXY_DIAG_RECORD Record(unsigned long long begin, unsigned long long end) {
    GALAXY_DIAG_RECORD r{};r.Session=1;r.Token=5;r.Begin=begin;r.End=end;
    r.Valid=1;r.Length=15;r.Report[0]=2;r.Report[1]=0x2c;r.Report[6]=0xff;r.Report[7]=0x0f;return r;
}
int main(){
    static_assert(sizeof(GALAXY_DIAG_START_DATA)==32);
    static_assert(sizeof(GALAXY_DIAG_RECORD)==64);
    static_assert(sizeof(GALAXY_DIAG_BATCH_DATA)==4152);
    // Official SipHash reference vector for message bytes 00..07, key 00..0f.
    Check(GalaxyDiagToken(0x0706050403020100ULL,0x0706050403020100ULL,0x0f0e0d0c0b0a0908ULL)
        ==0x93f5f5799a932462ULL,"SipHash vector");
    Check(GalaxyDiagToken(1,2,3)!=GalaxyDiagToken(1,2,4),"Key isolation");
    auto ring=std::make_unique<GALAXY_DIAG_BUFFER>();
    auto r=Record(1,5);auto original=r;
    for(unsigned i=0;i<GALAXY_DIAG_RING+7;++i)GalaxyDiagPush(ring.get(),&r);
    Check(ring->Count==GALAXY_DIAG_RING && ring->Lost==7,"Bounded ring loss");
    Check(std::memcmp(original.Report,r.Report,15)==0,"Report preservation");
    GALAXY_DIAG_RECORD batch[GALAXY_DIAG_BATCH]{};unsigned long long expected=1;
    while(auto n=GalaxyDiagPop(ring.get(),batch))for(unsigned i=0;i<n;++i)Check(batch[i].Sequence==expected++,"FIFO");
    Check(GalaxyDiagPop(ring.get(),batch)==0,"Empty ring");
    GalaxyDiagPush(ring.get(),&r);Check(GalaxyDiagPop(ring.get(),batch)==1 && batch[0].Sequence==2056,"Sequence gap");
    std::vector<GALAXY_DIAG_RECORD>a{Record(1,10)},b{Record(2,9)};
    auto result=diag::Compare(a,b);Check(result.equal==1,"Equal pair");
    b[0].Report[6]=0xf0;result=diag::Compare(a,b);Check(result.pressureChanged==1 && !result.otherChanged,"Pressure difference");
    b[0].Report[1]=0x21;result=diag::Compare(a,b);Check(result.otherChanged==1,"Flags difference");
    b[0].Token=8;Check(diag::Compare(a,b).unpairedUpper==1,"Reissued request must not match by coordinates");
    b[0]=Record(2,11);Check(diag::Compare(a,b).matched==0,"Time bounds");
    b[0]=Record(2,9);b[0].Session=2;Check(diag::Compare(a,b).matched==0,"Session isolation");
    b[0]=Record(2,9);b.push_back(b[0]);Check(diag::Compare(a,b).ambiguous==1,"Duplicate lower");
    b.resize(1);a.push_back(a[0]);Check(diag::Compare(a,b).ambiguous==2,"Duplicate upper");
    a.resize(1);b[0].Valid=0;Check(diag::Compare(a,b).invalid==1,"Invalid buffer");
    b[0]=Record(2,9);a.push_back(Record(20,30));b.push_back(Record(21,29));
    Check(diag::Compare(a,b).equal==2,"IRP reuse with disjoint intervals");
    Check(!diag::StackAllowed({L"\\Driver\\GalaxyPenCol01Corrector",L"\\Driver\\PenS2Helper",L"\\Driver\\mshidkmdf"}),"Production stack blocked");
    const std::vector<std::wstring> stack{L"\\Driver\\GalaxyPenDiagA",L"\\Driver\\PenS2Helper",L"\\Driver\\GalaxyPenDiagB",L"\\Driver\\mshidkmdf"};
    Check(diag::StackAllowed(stack),"Expected stack");auto wrong=stack;std::swap(wrong[0],wrong[2]);
    Check(!diag::StackAllowed(wrong),"Wrong stack ordering");
    wrong=stack;wrong.insert(wrong.begin(),L"\\Driver\\OtherFilter");
    Check(!diag::StackAllowed(wrong),"Unknown filter blocked");
    a={Record(1,10)};b={Record(2,9)};b[0].Status=0xc0000120;
    Check(diag::Compare(a,b).invalid==1,"Cancelled request not pressure evidence");
    b[0]=Record(2,9);b[0].Report[7]=0xff;
    Check(diag::Compare(a,b).invalid==1,"Out of range pressure");
    a.clear();b.clear();
    for(unsigned long long i=0;i<10000;++i){a.push_back(Record(i*10,i*10+9));b.push_back(Record(i*10+1,i*10+8));}
    Check(diag::Compare(a,b).equal==10000,"Repeated IRP identity bounded comparison");
    std::cout<<"Diagnostic protocol, keyed tokens, bounded buffer, exact-request matching and stack gate passed.\n";
}
