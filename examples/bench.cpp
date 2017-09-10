/*******************************************************************************
 * Copyright (C) 2017 Advanced Micro Devices, Inc. All rights reserved.
 *******************************************************************************/
#include <string>
#include <miopengemm/redirection.hpp>
#include <miopengemm/tinytwo.hpp>

int main()
{
  using namespace MIOpenGEMM;


  Geometry gg("tC0_tA0_tB0_colMaj1_m4096_n7000_k4096_lda4096_ldb4096_ldc4096_ws0_f32");
  HyPas    hp("A_MIC8_PAD2_PLU0_LIW0_MIW1_WOS0_VEW1__B_MIC8_PAD2_PLU1_LIW0_MIW1_WOS0_VEW1__C_UNR16_"
           "GAL1_PUN0_ICE1_IWI0_SZT0_MAD1_NAW64_UFO0_MAC256_SKW10_AFI1_MIA1");

  CLHint         devhint;
  Offsets        offsets = get_zero_offsets();
  owrite::Writer mowri(Ver::E::TERMWITHDEPS, "");
  dev::TinyTwo   boa(gg, offsets, mowri, devhint);

  for (unsigned i = 0; i < 1; ++i)
  {
    boa.benchgemm({hp}, {{0, 5}, {0., 100.}});
  }

  return 0;
}
