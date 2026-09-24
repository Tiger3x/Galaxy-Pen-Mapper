#pragma once
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include "../driver/shared/GalaxyPenDiag.h"

namespace diag {
inline bool StackAllowed(const std::vector<std::wstring>& stack) {
    // Actual top-to-bottom DEVPKEY_Device_Stack, not registration order.
    const std::vector<std::wstring> expected{L"\\Driver\\GalaxyPenDiagA", L"\\Driver\\PenS2Helper",
        L"\\Driver\\GalaxyPenDiagB", L"\\Driver\\mshidkmdf"};
    return stack == expected; // No correction filter or unknown component accepted.
}
inline bool Within(const GALAXY_DIAG_RECORD& upper, const GALAXY_DIAG_RECORD& lower) {
    return upper.Session && upper.Session == lower.Session && upper.Token == lower.Token &&
        upper.Begin <= lower.Begin && lower.Begin <= lower.End && lower.End <= upper.End;
}
struct Comparison {
    size_t matched{}, equal{}, pressureChanged{}, otherChanged{}, invalid{}, unpairedUpper{}, unpairedLower{}, ambiguous{};
};
inline bool ValidPen(const GALAXY_DIAG_RECORD& r) {
    return r.Valid == 1 && (r.Status & 0x80000000u) == 0 && r.Length == 15 && r.Report[0] == 2 &&
        (unsigned(r.Report[6]) | (unsigned(r.Report[7]) << 8)) <= 4095;
}
inline Comparison Compare(const std::vector<GALAXY_DIAG_RECORD>& upper,
                          const std::vector<GALAXY_DIAG_RECORD>& lower) {
    Comparison result;
    std::unordered_map<unsigned long long, std::vector<size_t>> tokens;
    for (size_t i=0; i<lower.size(); ++i) tokens[lower[i].Token].push_back(i);
    for(auto& entry:tokens) std::sort(entry.second.begin(),entry.second.end(),
        [&](size_t a,size_t b){return lower[a].Begin<lower[b].Begin;});
    std::vector<std::vector<size_t>> candidates(upper.size());
    std::vector<size_t> uses(lower.size());
    size_t work=0;
    for (size_t i=0; i<upper.size(); ++i) {
        const auto it=tokens.find(upper[i].Token); if(it==tokens.end()) continue;
        auto start=std::lower_bound(it->second.begin(),it->second.end(),upper[i].Begin,
            [&](size_t j,unsigned long long t){return lower[j].Begin<t;});
        for(;start!=it->second.end() && lower[*start].Begin<=upper[i].End;++start) {
            if(++work>1000000) throw std::runtime_error("Excessive ambiguous intervals; no conclusion.");
            const auto j=*start;
            if(Within(upper[i],lower[j])) {candidates[i].push_back(j);++uses[j];}
        }
    }
    std::vector<bool> used(lower.size());
    for(size_t i=0;i<upper.size();++i) {
        if(candidates[i].empty()) {++result.unpairedUpper;continue;}
        if(candidates[i].size()!=1 || uses[candidates[i][0]]!=1) {++result.ambiguous;continue;}
        const size_t j=candidates[i][0];used[j]=true;++result.matched;
        const auto& a=upper[i]; const auto& b=lower[j];
        if(!ValidPen(a)||!ValidPen(b)) {++result.invalid;continue;}
        bool sameOther=true;
        for(size_t k=0;k<15;++k) if(k!=6 && k!=7 && a.Report[k]!=b.Report[k]) sameOther=false;
        const bool samePressure=a.Report[6]==b.Report[6] && a.Report[7]==b.Report[7];
        if(sameOther && samePressure) ++result.equal;
        if(!samePressure) ++result.pressureChanged;
        if(!sameOther) ++result.otherChanged;
    }
    result.unpairedLower=static_cast<size_t>(std::count(used.begin(),used.end(),false));
    return result;
}
}
