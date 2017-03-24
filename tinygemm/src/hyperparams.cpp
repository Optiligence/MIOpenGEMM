#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <algorithm>
#include <random>
#include <limits>

#include <tinygemm/stringutilbase.hpp>
#include <tinygemm/hyperparams.hpp>
#include <tinygemm/tinygemmerror.hpp>
#include <tinygemm/stringutilbase.hpp>
#include <tinygemm/mapkeycheck.hpp>


namespace tinygemm{


namespace hyperparams{

/* TODO : move this out and make constructor which takes seed */
class RandomUtil {

private:
  std::random_device rd;
  std::default_random_engine gen;
  std::uniform_int_distribution<unsigned> unidis;

public:
  RandomUtil():rd(), gen(rd()) {} 
  unsigned get_from_range(unsigned upper){
    return unidis(gen) % upper;
  }
  
  template <typename T>
  void shuffle(unsigned start_index, unsigned end_index, T & t){
    if (end_index > t.size() || start_index > end_index){
      throw tinygemm_error("problem in shuffle");
    }
    std::shuffle(t.begin() + start_index, t.begin() + end_index, gen);
  }
  
  
};

RandomUtil radu;


template <typename T>
std::map<T, unsigned> getVals(unsigned nVals, const std::vector<T> & keys, const std::string & hash){
  std::map<T, unsigned> vals;    
  for (unsigned val = 0; val < nVals; ++val){
    if (keys[val] == T()){
      throw tinygemm_error("It appears as though one of the elements of " + hash +  " has not been added to keys, unitialisation error");
    }
    vals[keys[val]] = val;
    
  }
  return vals;
}

 
SUHP::SUHP(){

  ChiralKeys.resize(nsHP::nChiralHPs);
  NonChiralKeys.resize(nsHP::nNonChiralHPs);
  MatKeys.resize(nsHP::nMatrices);
  HPVals.resize(nsHP::nMatrices);
  HPKeys.resize(nsHP::nMatrices);
  nHPs.resize(nsHP::nMatrices);  
  //GraphKeys.resize(nsHP::nGraphTypes);
  
  
  ChiralKeys[nsHP::MIC] = "MIC";
  ChiralKeys[nsHP::PAD] = "PAD";
  ChiralKeys[nsHP::PLU] = "PLU";
  ChiralKeys[nsHP::LIW] = "LIW";
  ChiralKeys[nsHP::MIW] = "MIW";
  ChiralKeys[nsHP::WOS] = "WOS";
  ChiralVals = getVals(nsHP::nChiralHPs, ChiralKeys, "ChiralKeys");


  NonChiralKeys[nsHP::UNR] = "UNR";
  NonChiralKeys[nsHP::GAL] = "GAL";
  NonChiralKeys[nsHP::PUN] = "PUN";
  NonChiralKeys[nsHP::ICE] = "ICE";
  NonChiralKeys[nsHP::NAW] = "NAW";
  NonChiralKeys[nsHP::UFO] = "UFO"; 
  NonChiralKeys[nsHP::MAC] = "MAC"; 
  NonChiralVals = getVals(nsHP::nNonChiralHPs, NonChiralKeys, "NonChiralKeys");
  

  MatKeys[nsHP::matA] = 'A';
  MatKeys[nsHP::matB] = 'B';
  MatKeys[nsHP::matC] = 'C';
  MatVals = getVals(nsHP::nMatrices, MatKeys, "MatKeys");
  
  HPVals[nsHP::matA] = ChiralVals;
  HPVals[nsHP::matB] = ChiralVals;
  HPVals[nsHP::matC] = NonChiralVals;


  HPKeys[nsHP::matA] = ChiralKeys;
  HPKeys[nsHP::matB] = ChiralKeys;
  HPKeys[nsHP::matC] = NonChiralKeys;


  nHPs[nsHP::matA] = nsHP::nChiralHPs;
  nHPs[nsHP::matB] = nsHP::nChiralHPs;
  nHPs[nsHP::matC] = nsHP::nNonChiralHPs;  

  coupled_parameters.push_back( { {nsHP::matA, nsHP::MIC}, {nsHP::matB, nsHP::MIC} } );
  coupled_parameters.push_back( { {nsHP::matC, nsHP::UFO}, {nsHP::matC, nsHP::PUN} } );

  //GraphKeys[nsHP::ChiralType] = ChiralKeys;
  //GraphKeys[nsHP::NonChiralType] = NonChiralKeys;

}

const SUHP suHP;

}

namespace hyperparams{

const std::map<unsigned, std::vector<unsigned> > graph_binary = 
{   {0, {1}},
    {1, {0}}    };

void Graph::update_range(){
  range.resize(graph.size());
  for (unsigned i = 0; i < graph.size(); ++i){
    for (auto & x : graph[i]){
      range[i].push_back(x.first);
    }
  }
}


std::vector<unsigned> Graph::get_start_range(unsigned hpi, const tinygemm::TinyGemmGeometry & gg) const{
  
  if ((gg_start_range[hpi] == false) == (default_start_range[hpi].size() == 0)){
    std::stringstream errm;
    errm << "Either one of gg_start_range or defalut_start_range should be defined for " << (*ptr_graphkeys)[hpi] << ". Not 0 or 2, but 1 of them.";
    throw tinygemm_error(errm.str());
  }
  
  if (default_start_range[hpi].size() != 0){
    return default_start_range[hpi];
  }
  
  else{
    if (!gg_start_range[hpi]){
      throw tinygemm_error("default_start_range not defined here");
    }
    return gg_start_range[hpi](gg);
  }
}


Graph::Graph(unsigned size): graph (size), default_start_range (size), gg_start_range (size) {}

void Graph::initialise(){
  set_ptr_graphkeys();
  set_edges();
  update_range();
  set_start_range();
  confirm_start_is_subset();
}

void Graph::confirm_start_is_subset(){
  //TODO
}

ChiralGraph::ChiralGraph() : Graph(nsHP::nChiralHPs){}

NonChiralGraph::NonChiralGraph() : Graph(nsHP::nNonChiralHPs){}

//AChiralGraph::NonChiralGraph() : Graph(nsHP::nNonChiralHPs){}

void ChiralGraph::set_ptr_graphkeys(){ ptr_graphkeys = &suHP.ChiralKeys; }
void NonChiralGraph::set_ptr_graphkeys(){ ptr_graphkeys = &suHP.NonChiralKeys; }



void AChiralGraph::set_chirality_specific(){
  gg_start_range[nsHP::MIC] = [](const tinygemm::TinyGemmGeometry & gg){ return std::vector<unsigned> {8}; };  
}

void BChiralGraph::set_chirality_specific(){
  gg_start_range[nsHP::MIC] = [](const tinygemm::TinyGemmGeometry & gg){ return std::vector<unsigned> {8}; };  
}




void ChiralGraph::set_start_range(){
  
  default_start_range[nsHP::MIC] = {};
  default_start_range[nsHP::PAD] = {1};    
  default_start_range[nsHP::PLU] = {nsHP::no, nsHP::yes};  
  default_start_range[nsHP::LIW] = {nsHP::no};  
  default_start_range[nsHP::MIW] = {nsHP::yes};
  default_start_range[nsHP::WOS] = {0};

  set_chirality_specific();

}

void ChiralGraph::set_edges(){
  
  graph[nsHP::MIC] =
  { {1, {2,3} },
    {2, {1,3,4} },
    {3, {1,2,4} },
    {4, {2,3,5,6} },
    {5, {2,4,6} },
    {6, {4,5,8} },
    {8, {4,6} }    };
    
  graph[nsHP::PAD] = 
  { {0, {1}   },
    {1, {0, 2}}, 
    {2, {1},  }     };
  
  graph[nsHP::PLU] = 
  {  graph_binary  };


  graph[nsHP::LIW] = 
  {  graph_binary  };


  graph[nsHP::MIW] = 
  {  graph_binary  };

  
  graph[nsHP::WOS] = 
  {  {0, {}}   };// for now, no copying TODO(jn) incorporate
}


void NonChiralGraph::set_start_range(){

  default_start_range[nsHP::UNR]= {8, 16};
  default_start_range[nsHP::NAW]= {16, 64};
  default_start_range[nsHP::GAL]= {nsGAL::byrow, nsGAL::bycol, nsGAL::sucol};
  default_start_range[nsHP::MAC]= {nsMAC::a8b8, nsMAC::a16b16};
  default_start_range[nsHP::ICE] = {1};
  default_start_range[nsHP::PUN] = {nsHP::no, nsHP::yes};
  default_start_range[nsHP::UFO] = {nsHP::no};

}

void NonChiralGraph::set_edges(){
  
  graph[nsHP::UNR] = 
  { {8, {16} },
    {16, {8,32} },
    {32, {16, 64} },
    {64, {16, 32} }  };
  
  graph[nsHP::NAW] = 
  { {64, {16} },
    {16, {64} }  };
  
  graph[nsHP::GAL] = 
  { {nsGAL::byrow, {nsGAL::bycol, nsGAL::sucol}   },
    {nsGAL::bycol, {nsGAL::byrow, nsGAL::sucol}   },
    {nsGAL::sucol, {nsGAL::byrow, nsGAL::bycol}   }   };

  graph[nsHP::MAC] = 
  {
    // to be added foe 32 in work group GPUs
    //{nsMAC::a4b8, {nsMAC::a8b8}},
    //{nsMAC::a8b4, {nsMAC::a8b8}},

    //{nsMAC::a8b16, {nsMAC::a8b8, nsMAC::a16b16}},
    //{nsMAC::a16b8, {nsMAC::a8b8, nsMAC::a16b16}},

    //{nsMAC::a1b1, {nsMAC::a4b4}},
    //{nsMAC::a4b4, {nsMAC::a8b8, nsMAC::a8b8}},
    {nsMAC::a8b8, {nsMAC::a16b16}},
    {nsMAC::a16b16, {nsMAC::a8b8}},
  };
  
  graph[nsHP::ICE] = 
  { {1,  {2}},
    {2,  {1,3,4}},
    {3,  {1,2,4,6}},
    {4,  {1,3,5,7}},
    {5,  {1,2,4,6,8}},
    {6,  {1,3,5,7,9}},
    {7,  {4,6,8,10}},
    {8,  {1,5,7,9,11}},
    {9,  {6,8,10,12}},
    {10, {1,7,9,11,13}},
    {11, {8,10,12,14}},
    {12, {1,9,11,13,14}},
    {13, {10,12,14}},
    {14, {1,11,13}}   };

  graph[nsHP::PUN] = 
  {  graph_binary  };
  
  graph[nsHP::UFO] =
  {  graph_binary  };
  
}



AChiralGraph get_graph_a(){
  AChiralGraph graph_a;
  graph_a.initialise();
  return graph_a;
}
AChiralGraph a_graph(get_graph_a());


BChiralGraph get_graph_b(){
  BChiralGraph graph_b;
  graph_b.initialise();
  return graph_b;
}
BChiralGraph b_graph(get_graph_b());


NonChiralGraph get_non_chiral_graph(){
  NonChiralGraph non_chiral_graph;
  non_chiral_graph.initialise();
  return non_chiral_graph;
}
NonChiralGraph non_chiral_graph(get_non_chiral_graph());

std::vector<std::vector<unsigned>> get_params_from_string(const std::string & hyperstring, bool expect_full_hyperstring){


  /* TODO only generate this when an error emerges */
  std::stringstream ssghe;
  ssghe << "the " << (expect_full_hyperstring == true ? "`full'" : "`partial'") << " hyperstring received here is :\n";
  ssghe << "         " << hyperstring << "\n";
  ssghe << "an example of a full hyperstring correctly formated is:\n";
  ssghe << "         ";
  ssghe << "A_MIC8_PAD1_PLU0_LIW0_MIW1_WOS0__B_MIC6_PAD1_PLU0_LIW0_MIW1_WOS0__C_UNR16_GAL3_PUN0_ICE1_NAW64_UFO0_MAC2\n";
  std::string generic_hyperstring_errm = ssghe.str();

  std::string shortkey;
  unsigned val;  

  /* set params to be of the correct shape, filled with nsHP::undefined */
  std::vector<std::vector<unsigned>> params (nsHP::nMatrices);
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    params[mi].resize(suHP.nHPs[mi]);
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      params[mi][hpi] = nsHP::undefined;
    }
  }
    
  auto megafrags = stringutil::split(hyperstring, "__");
  
  for (auto & megafrag : megafrags){
    char matrixkey = megafrag[0];

    if (matrixkey != 'A' && matrixkey != 'B' && matrixkey != 'C'){
      std::stringstream ss;
      ss << "\nWhile reading hyperstring in get_params_from_string. `matrixkey' should be A,B or C, not `" << matrixkey << "'.\n";
      ss << generic_hyperstring_errm;
      throw tinygemm_error(ss.str());
    }

    auto matrix_val = suHP.MatVals.at(matrixkey);
    auto keyvalfrags = stringutil::split(megafrag.substr(2), "_");
    if (expect_full_hyperstring && (keyvalfrags.size() < suHP.HPKeys[matrix_val].size())){
      std::stringstream ss;
      ss << "While processing frag section " << suHP.MatKeys[matrix_val] << ".\n";
      ss << "There appear to be too few parameters (" << keyvalfrags.size() << " as opposed to " << suHP.HPKeys[matrix_val].size() << ")";
      ss << generic_hyperstring_errm;
      throw tinygemm_error(ss.str());
    }
    
    for (auto & x : keyvalfrags){
      std::tie(shortkey, val) = stringutil::splitnumeric(x);
      auto start = suHP.HPKeys[matrix_val].begin();
      auto end  = suHP.HPKeys[matrix_val].end();
      if(std::find(start, end, shortkey) == end) {
        std::stringstream ss;
        ss << "While processing frag section " << suHP.MatKeys[matrix_val] << ".\n";
        ss << "in get_params_from_string in hyperparams.cpp   :   unrecognised : " + shortkey << ".\n";
        ss << generic_hyperstring_errm;
        throw tinygemm_error(ss.str());
      }
      /* We have confirmed that shortkey is valid, this is safe */
      auto shortkey_val = suHP.HPVals[matrix_val].at(shortkey);
      if (shortkey_val < params[matrix_val].size()){
        params[matrix_val][shortkey_val] = val;
      }
      else{
        throw tinygemm_error("in get_params_from_string, cannot insert as out of bounds.");
      }
    }
  }
  return params;
}

std::string XHPs::get_string() const{
  std::stringstream ss;
  for (unsigned hpi = 0; hpi < vs.size() - 1; ++hpi){
    ss << (*ptr_hpkeys)[hpi] << vs[hpi] << "_";
  }
  ss << (*ptr_hpkeys).back() << vs.back();
  return ss.str();
}

void XHPs::check() const{
  for (unsigned i = 0; i < vs.size(); ++i){
    auto start = ptr_hpgraph->range[i].begin();
    auto end = ptr_hpgraph->range[i].end();
    if (vs[i] == nsHP::undefined || (std::find(start, end, vs[i]) == end)) {
      std::stringstream errm;
      errm << "\nIn XHPs::check(). It appears as though `" << vs[i] << "' is not a valid value for " << (*ptr_hpkeys)[i] << ".\n"; 
      errm << "The valid values are,\n         [";
      for (auto & x : ptr_hpgraph->range[i]){
        errm << " " <<  x << " ";
      }
      errm << "]\n";
      throw tinygemm_error(errm.str());
    }
  }
}

void HyperParams::checks() const{
  for (auto & x : v_xhps){
    x.check();
  }
}


void HyperParams::replace(const std::vector<std::vector<unsigned>> & params){
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      v_xhps[mi].vs[hpi] = params.at(mi).at(hpi);
    }
  }
}

/* go through the params, and where it is not nHP::undefined, use its value to replace this */
void HyperParams::replace_where_source_defined(const std::vector<std::vector<unsigned>> & params){
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      if (params[mi][hpi] != nsHP::undefined){
        v_xhps[mi].vs[hpi] = params[mi][hpi];
      }
    }
  }
}

void HyperParams::replace_undefined_randomly(const tinygemm::TinyGemmGeometry & gg){
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      if (v_xhps[mi].vs[hpi] == nsHP::undefined){
        //select randomly from ptr_hpgraph->default_start_range. Not quite random as it depends on gg. 
        
        

        auto a_range = v_xhps[mi].ptr_hpgraph->get_start_range(hpi, gg);
        unsigned index = radu.get_from_range (a_range.size());
        v_xhps[mi].vs[hpi] = a_range[index];
      }
    }
  }
}




HyperParams::HyperParams():
v_xhps {
{&suHP.ChiralKeys, &a_graph, nsHP::nChiralHPs}, 
{&suHP.ChiralKeys, &b_graph, nsHP::nChiralHPs}, 
{&suHP.NonChiralKeys, &non_chiral_graph, nsHP::nNonChiralHPs}} 
{
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      v_xhps[mi].vs[hpi] = nsHP::undefined;
    }
  }
}  

HyperParams::HyperParams(const std::vector<std::vector<unsigned>> & params):HyperParams()
{
  replace(params);
  checks();
}


HyperParams::HyperParams(const std::string & hyperstring):HyperParams(get_params_from_string(hyperstring, true)){}





HyperParams get_cacheless_default(const tinygemm::TinyGemmGeometry & gg){
  HyperParams hp("A_MIC8_PAD1_PLU0_LIW0_MIW1_WOS0__B_MIC6_PAD1_PLU0_LIW0_MIW1_WOS0__C_UNR16_GAL3_PUN0_ICE1_NAW64_UFO0_MAC5");
  //whittle for now ?  
  return hp;
}


HyperParams get_default(const tinygemm::TinyGemmGeometry & gg){
  
  if (gg.m < 8 || gg.n < 8){
    std::cout << "really skinny/thin matrix, returning a default TinyGemmSolution based on gg and constraint_string without searching/benching " << std::endl;
    throw tinygemm_error("sort this out");
  }
    
  /* TODO : rather get default from cache, based on gg (?) */
  auto hp = get_cacheless_default(gg);
  
  throw tinygemm_error("get_default not yet enabled (in paper writing mode)");
  return hp;  
}
  
bool HyperParams::operator == (const HyperParams & hpr){
  return get_string() == hpr.get_string(); 
}

std::string HyperParams::get_string() const{
  std::stringstream ss;
  for (unsigned mi : {nsHP::matA, nsHP::matB}){
    ss << suHP.MatKeys[mi] << "_" << v_xhps[mi].get_string() << "__";
  }
  ss << 'C' << "_" << v_xhps[nsHP::matC].get_string();
  return ss.str();
}

std::vector<HyperParams> HyperParams::get_one_aways(){
  
  std::vector<HyperParams> one_aways;
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      unsigned value = v_xhps[mi].vs[hpi];
      for (auto & newval : v_xhps[mi].ptr_hpgraph->graph[hpi].at(value)){
        HyperParams hp(*this);
        hp.v_xhps[mi].vs[hpi] = newval;
        one_aways.push_back(hp);        
      }
    }
  }
  unsigned n_uncoupled = one_aways.size();
  
  
  for (auto & couple_p : suHP.coupled_parameters){

    auto first = std::get<0>(couple_p);
    auto first_m = std::get<0>(first);
    auto first_p = std::get<1>(first);
    auto first_value = v_xhps[first_m].vs[first_p];
    
    auto second = std::get<1>(couple_p);
    auto second_m = std::get<0>(second);
    auto second_p = std::get<1>(second);
    auto second_value = v_xhps[second_m].vs[second_p];

    for (auto & new_first_val : v_xhps[first_m].ptr_hpgraph->graph[first_p].at(first_value)){
      for (auto & new_second_val : v_xhps[second_m].ptr_hpgraph->graph[second_p].at(second_value)){      
        HyperParams hp(*this);
        
        hp.v_xhps[first_m].vs[first_p] = new_first_val;
        hp.v_xhps[second_m].vs[second_p] = new_second_val;
        one_aways.push_back(hp);
      
      }
    }
  }
  
  unsigned n_total = one_aways.size();

  /* shuffle them, which bounds the expected time to finding an improvement 
   * (prevents pathological case of all improving kernels at end of vector)  */

  /* shuffle the true one aways */
  radu.shuffle(0, n_uncoupled, one_aways);
  
  
  /* shuffle the two aways (coupled) */
  radu.shuffle(n_uncoupled, n_total, one_aways);
  
  /* shuffle the custom kernels. What? Custom kernels? */
  

  return one_aways;
}
  

//std::make_tuple(100, 32, 26939, 26939, 26939, 100, true, false), 
/* see dev/python/deepbench/deepbench_results.py : this is generated by get_kernel_cache_string, based on results running find with allotted_time =  30 seconds per problem, with three starting kernels for
 * small, medium, large: On a Fiji! 
 * TODO : regenerate with longer runs and more problems.
 * TODO : should not be a single vector, this has linear find time. At least seperate out isColMajor, tA, tB  
 * TODO : figure out how to make cache contain only reduced problems .... very important! */


     
HyperParams get_hp_start(FindStartType fst, std::string constraint_string, const tinygemm::TinyGemmGeometry & gg){

  auto constraint_params = get_params_from_string(constraint_string, false);

  HyperParams hyper_param_start;
  if (fst == FindStartType::Default){
    hyper_param_start = get_default(gg);
    hyper_param_start.replace_where_source_defined(constraint_params);
  }
  
  else if (fst == FindStartType::Random){
    hyper_param_start.replace(constraint_params);
    hyper_param_start.replace_undefined_randomly(gg);
  }
  
  hyper_param_start.checks();  
  return hyper_param_start;
}




bool HyperParams::satisfies_where_source_defined(const std::vector<std::vector<unsigned>> & params){
  /* filtering out if violates the constraint string */
  bool constraints_satisfied = true;
  for (unsigned mi = 0; mi < nsHP::nMatrices; ++mi){
    for (unsigned hpi = 0; hpi < suHP.nHPs[mi]; ++hpi){
      if (params[mi][hpi] != nsHP::undefined && v_xhps[mi].vs[hpi] != params[mi][hpi]){
        constraints_satisfied = false;
        break;
      }
    }
  }
  return constraints_satisfied;
}


} 
}
