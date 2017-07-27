/*******************************************************************************
 * Copyright (C) 2017 Advanced Micro Devices, Inc. All rights reserved.
 *******************************************************************************/
#include <miopengemm/kernelcachemerge.hpp>
#include <miopengemm/findparams.hpp>
  
MIOpenGEMM::KernelCache get_kernel_cache2()
{
  MIOpenGEMM::KernelCache kc;
  #include "/home/james/test26/cacheentries.txt"
  return kc;
}




int main()
{
  using namespace MIOpenGEMM;  

  KernelCache kernel_cache2 = get_kernel_cache2();
  
  owrite::Writer mowri(Ver::E::MERGE, "");

  Halt halt  = {{0, 5}, {0, 0.2}};
  auto kcn = get_merged(kernel_cache, kernel_cache2, halt, mowri);
  std::ofstream floper("/home/james/test26/merged_cache26.txt", std::ios::out);    
  for (auto & ck : kcn.get_keys()){
    //std::cout << ck.get_string() << std::endl;
    floper << '\n' << get_cache_entry_string(ck, kcn.at(ck));
  }
  floper.close();
  return 0;
}
