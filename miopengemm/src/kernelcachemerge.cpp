/*******************************************************************************
 * Copyright (C) 2017 Advanced Micro Devices, Inc. All rights reserved.
 *******************************************************************************/
#include <functional>
#include <unordered_map>
#include <chrono>
#include <miopengemm/devmiogemm.hpp>
#include <miopengemm/setabcw.hpp>
#include <miopengemm/kernelcachemerge.hpp>
#include <algorithm>

namespace MIOpenGEMM{

// sequence for a fair penalty shoot-out. 
std::vector<bool> get_thue_morse(size_t length){
  std::vector<bool> thue_morse {true, false};
  while (thue_morse.size() < length){
    for (auto x : thue_morse){
      thue_morse.push_back(!x);
    }
  } 
  thue_morse.resize(length);
  return thue_morse;
}

template <typename TFl>
void populate(const std::vector<CacheKey> & cache_keys, const KernelCache & kc1, const KernelCache & kc2, KernelCache & kc, const Halt & halt, owrite::Writer& mowri){
    
  Offsets        offsets = get_zero_offsets();
  CLHint         devhint;
  
  // we set the CPU memory once for all geometries.
  // This is much faster than once for each geometry using Boas
  mowri.bw[OutPart::MER] << "generating random matrices on CPU ... " << Flush;
  setabcw::CpuMemBundle<TFl> cmb(get_geometries(cache_keys), offsets);
  mowri.bw[OutPart::MER] << "done. Will perform Thue–Morse ABBABAAB 1-on-1." << Endl;
  for (size_t i = 0; i < cache_keys.size(); ++i)
  {
     auto ck = cache_keys[i];
    if (kc1.at(ck) == kc2.at(ck)){
      kc.add(ck, kc1.at(ck));
      mowri.bw[OutPart::MER] << "[ss]" << Flush;
      continue;
    }
    mowri.bw[OutPart::MER] << '\n' << "(" <<  i << " / " << cache_keys.size() << ")";    
    mowri.bw[OutPart::MER] << ck.gg.get_string() << Endl; 

    mowri.bw[OutPart::MER] << "soln1 : " << kc1.at(ck).get_string() << Endl;
    mowri.bw[OutPart::MER] << "soln2 : " << kc2.at(ck).get_string() << Endl;

   
    // having two Divas means that each opposing kernel only needs be compiled once. Optional.
    dev::Diva<TFl> diva1(ck.gg, offsets, cmb.r_mem, mowri, devhint);
    dev::Diva<TFl> diva2(ck.gg, offsets, cmb.r_mem, mowri, devhint);

    mowri.bw[OutPart::MER] << "Two divas generated" << Endl;

    std::vector<double> times_kc1;
    std::vector<double> times_kc2;

    size_t kc1_wins = 0;
    size_t kc2_wins = 0;
    
    std::string prefix = std::to_string(i) + "/" + std::to_string(cache_keys.size());
    prefix.resize(8, ' ');
    
    auto act_kcx = [&halt, &mowri, &ck, &prefix](const KernelCache & kcx, std::string frag, std::vector<double> & times, dev::Diva<TFl> & diva){
      mowri.bw[OutPart::MER] << '<' << frag  << Flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(80));
      std::vector<double> ltimes = diva.benchgemm({kcx.at(ck)}, halt).back();
      
      double zoo = *std::min_element(ltimes.begin(), ltimes.end());//.back();
      //std::cout << "times : ";
      //for (auto & x : ltimes){
        //std::cout << x << "   ";
      //}
      //std::cout << std::endl;
      
      mowri.bw[OutPart::MER] << '>' << Flush;
      times.push_back(zoo);
    };

    
    
    for (auto kc1_first : get_thue_morse(11)){
      if (kc1_first){
        act_kcx(kc1, "1", times_kc1, diva1);
        act_kcx(kc2, "2", times_kc2, diva2);
      }
      else{
        act_kcx(kc2, "2", times_kc2, diva2);
        act_kcx(kc1, "1", times_kc1, diva1);
      }
      mowri.bw[OutPart::MER] << '|' << Flush; 
      
      kc1_wins += (times_kc1.back() < times_kc2.back());
      kc2_wins += (times_kc2.back() < times_kc1.back());
    }
    mowri.bw[OutPart::MER] << Endl;
    for (unsigned ri = 0; ri < times_kc1.size(); ++ri){
      auto g1 = ck.gg.get_gflops(times_kc1[ri]);
      auto g2 = ck.gg.get_gflops(times_kc2[ri]);
            
      mowri.bw[OutPart::MER] << stringutil::get_char_padded(g1,8) << " \t ";
      if (g1 > g2){
        mowri.bw[OutPart::MER] << '>';
      }
      else{
        mowri.bw[OutPart::MER] << "<=";
      }
      mowri.bw[OutPart::MER] << " \t "
      << stringutil::get_char_padded(g2,8) << Endl;
    }
    
    if (kc1_wins > kc2_wins){
      mowri.bw[OutPart::MER] << "kc1 won, " << kc1_wins << ':' << kc2_wins << '.' << '\n';
      kc.add(ck, kc1.at(ck));
    }
    else{
      mowri.bw[OutPart::MER] << "kc2 won, " << kc2_wins << ':' << kc1_wins << '.' << '\n';
      kc.add(ck, kc2.at(ck));
    }
    mowri.bw[OutPart::MER] << '\n';     
  }
  
  mowri.bw[OutPart::MER] << '\n'; 
}
  
KernelCache get_merged(const KernelCache & kc1, const KernelCache & kc2, const Halt & halt, owrite::Writer& mowri){

  KernelCache kc; 
  std::map<char, std::vector<CacheKey>> in_both;
    
  size_t from_kc1 {0};
  size_t from_kc2 {0};
  size_t undetermined {0};
  for (auto & k1 : kc1.get_keys()){
    if (!kc2.check_for(k1).is_present){
      kc.add(k1, kc1.at(k1));
      ++from_kc1;
    }
    else{
      if (in_both.count(k1.gg.floattype) == 0){
        in_both[k1.gg.floattype] = {};
      }
      in_both[k1.gg.floattype].push_back(k1);
      ++undetermined;
    }
  }
  
  for (auto & k2 : kc2.get_keys()){
    if (!kc1.check_for(k2).is_present){
      kc.add(k2, kc2.at(k2));
      ++from_kc2;
    }
  }
  
  mowri.bw[OutPart::MER] << "from kc1 : " << from_kc1 << ", from kc2 : " << from_kc2 << ", to be determined : " << undetermined << Endl;
  
  for (auto & x : in_both){
    switch (std::get<0>(x)){
      case 'f': populate<float>(in_both['f'], kc1, kc2, kc, halt, mowri); break;
      case 'd': populate<double>(in_both['f'], kc1, kc2, kc, halt, mowri); break;
      default : throw miog_error("unrecognised floattype in get_merged");
    }
  }
  
  return kc;
}

}
