#ifndef TINYGEMM_TINYGEMMKERNELCACHE_HPP
#define TINYGEMM_TINYGEMMKERNELCACHE_HPP


#include <tinygemm/tinygemmsolution.hpp>
namespace tinygemm{
 
std::string get_cache_keys_string(std::string k_dev, std::string k_con, std::string k_geo, std::string k_comment);


class TinygemmCachedSolution {
  public:
    std::string hyperstring;
    tinygemm::TinyGemmSolutionStatistics stats;    
    TinygemmCachedSolution(std::string hyperstring_, tinygemm::TinyGemmSolutionStatistics stats_ ):hyperstring(hyperstring_), stats(stats_) {}//, stats_string(stats_string_) {}
    TinygemmCachedSolution() = default;    
    
    std::string get_string();
    
};


using KernelCache = std::map< std::string, std::map< std::string, std::map<std::string, std::map<std::string, tinygemm::TinygemmCachedSolution> > > >;

KernelCache get_kernel_cache();


TinygemmCachedSolution get_generic_cached_solution(const std::string & constraints_string, const tinygemm::TinyGemmGeometry & gg);


/* [device][constraint][further_comment][geometry] -> cached solution */
extern const KernelCache kernel_cache;

void add_entry(KernelCache & kc, const std::string & k_dev, const std::string & k_con,  const std::string k_geo, const std::string k_comment, tinygemm::TinygemmCachedSolution tgcs);


}


#endif
