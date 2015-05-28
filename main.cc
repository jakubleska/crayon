#include <iostream>
#include <regex>
#include <boost/program_options.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/interval_set.hpp>


namespace bpo = boost::program_options;
namespace bil = boost::icl;

enum colors {
  RED=1,
  GREEN=2,
  BLUE=4
};



int main(int argc, char** argv) {
    

  bpo::options_description all("Allowed options");
  bpo::options_description iface("Program options");
  bpo::options_description regex("Regex options");

  std::vector<std::string> red_regexs;
  std::vector<std::string> green_regexs;
  std::vector<std::string> blue_regexs;
  


  iface.add_options()
    ("help", "produce help message")
    ("zsh", "use zsh colors")
    ("bash", "use sh/zsh colors[this is default]")
    ("no-multithread", "disable use mulitple threads (one thread per color, one for intput and one for output)\nhigher performance on one core devices")
  ;
  
 /* 
//      |   cin
//      +_____     
//      |     \ 
//      |     /
//  ----+--- - 
// / \ /|\  \
// |  / | \ |
// | / \|  \| 
// |*  |*  *|  RGB
// \*  \*  */
//  :   :  : +
//   \  |  / |
//    \_|_/  |
//      |    |
//      |   / 
//      |---    
//      |   
//      |   cout  */
  
    
  regex.add_options()
    ("red,r", bpo::value<std::vector<std::string>>(&red_regexs), "regex for red color")
    ("green,g", bpo::value<std::vector<std::string>>(&green_regexs), "regex for green color")
    ("blue,b", bpo::value<std::vector<std::string>>(&blue_regexs), "regex for blue color")
  ;
  
  all.add(iface).add(regex);

  bpo::variables_map optionsmap;
  bpo::store(bpo::command_line_parser(argc, argv).options(all).run(), optionsmap);
  bpo::notify(optionsmap);


  if (optionsmap.count("help")) {
    std::cout << all << "\n";
    return 1;
  }

  if (optionsmap.count("zsh") && optionsmap.count("bash")) {
    std::cout << "Invalid options, can't use --zsh and --bash at once" << std::endl << all << std::endl;
    return 1;
  }

  if (optionsmap.count("zsh")) {
    std::cout << "Use zsh colors" << std::endl;
  }

  if (optionsmap.count("bash")) {
    std::cout << "Use bash colors" << std::endl;
  }

  
  
  std::vector<std::regex> red_re_s;
  std::vector<std::regex> green_re_s;
  std::vector<std::regex> blue_re_s;
  
  std::string s="";
  bool separ_active=false;
  for (auto& val : red_regexs) {
    if(separ_active) s+="|";
    s+=std::string("(")+val+")";
    separ_active=true;
    //red_re_s.push_back(std::regex(val));
  }
  red_re_s.push_back(std::regex(s));
  
  s="";separ_active=false;
  
  for (auto& val : green_regexs) {
    if(separ_active) s+="|";
    s+=std::string("(")+val+")";
    separ_active=true;
    //green_re_s.push_back(std::regex(val));
  }
  green_re_s.push_back(std::regex(s));
  
  s="";separ_active=false;
  
  for (auto& val : blue_regexs) {
    if(separ_active) s+="|";
    s+=std::string("(")+val+")";
    separ_active=true;
    //blue_re_s.push_back(std::regex(val));
  }
  blue_re_s.push_back(std::regex(s));
  
  std::string current_line;
  typedef std::set<int> color_bit;
  
  while(std::getline(std::cin, current_line)){
    

    bil::interval_map<int,color_bit> lineparts; 
    
    //to print no colored parts
    lineparts += std::make_pair(bil::interval<int>::right_open(0,current_line.length()), color_bit({0}));
    
    for (std::size_t rei=0;rei<red_re_s.size();rei++) {
      std::smatch mach_res;
      if ( std::regex_search(current_line, mach_res, red_re_s[rei])) {
        for (int i = 0; i < mach_res.size(); i++) {
          lineparts += std::make_pair(bil::interval<int>::right_open(mach_res.position(i), mach_res.position(i)+mach_res.length(i)), color_bit({RED}));
        }
      }
    }
    
    
    for (std::size_t rei=0;rei<green_re_s.size();rei++) {
      std::smatch mach_res;
      if ( std::regex_search(current_line, mach_res, green_re_s[rei])) {
        for (int i = 0; i < mach_res.size(); i++) {
          lineparts += std::make_pair(bil::interval<int>::right_open(mach_res.position(i), mach_res.position(i)+mach_res.length(i)), color_bit({GREEN}));
        }
      }
    }
    
    
    for (std::size_t rei=0;rei<blue_re_s.size();rei++) {
      std::smatch mach_res;
      if ( std::regex_search(current_line, mach_res, blue_re_s[rei])) {
        for (int i = 0; i < mach_res.size(); i++) {
          lineparts += std::make_pair(bil::interval<int>::right_open(mach_res.position(i), mach_res.position(i)+mach_res.length(i)), color_bit({BLUE}));
        }
      }
    }
    
  
    for (auto & it : lineparts) {
      int val = 0;
      for (auto & col_frag : it.second) {
        val |=  col_frag;
      }
      std::cout<<"\033[0;3"<<val<<"m"<<current_line.substr(it.first.lower(),it.first.upper()-it.first.lower())<<"\033[0;0m";
    }  
    std::cout<<std::endl;
  }

  return 0;
}

