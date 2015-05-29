/*
        Copyright (c) 2015 Jakub Leska, All Rights Reserved
        
        This file is part of crayon (http://leska.me/projects/crayon).
        
        Crayon is free software: you can redistribute it and/or modify
        it under the terms of the GNU Lesser General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.
        
        Crayon is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.
        
        You should have received a copy of the GNU Lesser General Public License
        along with Cryaon.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @todo improve thread synchronization
 */

#include <cstdlib>
#include <iostream>
#include <regex>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <boost/program_options.hpp>
#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <boost/icl/interval_set.hpp>


 
std::mutex m;
std::condition_variable cv;
std::string data;
bool ready = false;
int processed = 0;


///

namespace bpo = boost::program_options;
namespace bil = boost::icl;

enum colors_e {
  RED=1,
  GREEN=2,
  BLUE=4
};

typedef std::string color_map_t[8];
typedef std::set<int> color_bit_t;



color_map_t bash_colors = {
  "\033[0;0m",
  "\033[0;31m",
  "\033[0;32m",
  "\033[0;33m",
  "\033[0;34m",
  "\033[0;35m",
  "\033[0;36m",
  "\033[0;37m"
};

color_map_t bash_reverse_colors = {
  "\033[0;0m",
  "\033[0;31m",
  "\033[0;32m",
  "\033[0;33m",
  "\033[0;34m",
  "\033[0;35m",
  "\033[0;36m",
  "\033[0;30m"
};


std::mutex red_read_mut, red_write_mut;
std::mutex green_read_mut, green_write_mut;
std::mutex blue_read_mut, blue_write_mut;


std::vector<std::regex> parse_regexes(std::vector<std::string> red_regexs, bool regex_concat_allowed) {

  std::string concatenated_regexes = "";
  bool separ_active = false;

  std::vector<std::regex> re_s;

  for (auto & val : red_regexs) {
    
    if ( val == "" ) continue;
    
    try {
      if ( regex_concat_allowed ) {
        if (separ_active) {
          concatenated_regexes += "|";
        }

        concatenated_regexes += std::string("(") + val + ")";
        separ_active = true;
      } else {
        re_s.push_back(std::regex(val));
      }
    } catch (std::regex_error& e) {
      std::cerr<<"Cant convert to regex '"<<val<<"'"<<std::endl;
    }
  }

  if ( regex_concat_allowed ) {
    if ( concatenated_regexes != "" ) re_s.push_back(std::regex(concatenated_regexes));
  }

  return re_s;
}

void color_process(const std::string& str, const std::vector<std::regex>& re_s, colors_e color, bil::interval_map<int, color_bit_t>& lineparts) {

  
  for (std::size_t rei = 0; rei < re_s.size(); rei++) {
    
    
//     std::smatch mach_res;
// 
//     if ( std::regex_search(str, mach_res, re_s[rei]), std::regex_constants::format_sed | std::regex_constants::match_any) {
//       for (std::size_t i = 0; i < mach_res.size(); i++) {
//         lineparts += std::make_pair(bil::interval<int>::right_open(mach_res.position(i), mach_res.position(i) + mach_res.length(i)), color_bit_t( {color}));
//       }
//     }
    
    
    std::sregex_iterator next(str.begin(), str.end(), re_s[rei]);
    std::sregex_iterator end;
    while (next != end) {
      std::smatch match = *next;
      lineparts += std::make_pair(bil::interval<int>::right_open(match.position(0), match.position(0) + match.length(0)), color_bit_t( {color}));
      next++;
    } 
  }
}

std::string colorize(const std::string& str, const color_map_t& color_map, bil::interval_map<int, color_bit_t>& lineparts){
  
  //to print no colored parts
  std::string colorized_str;
  lineparts += std::make_pair(bil::interval<int>::right_open(0, str.length()), color_bit_t({0}));

  for (auto & it : lineparts) {
    int val = 0;

    for (auto & col_frag : it.second) {
      val |=  col_frag;
    }
    
    colorized_str += color_map[val] + str.substr(it.first.lower(), it.first.upper() - it.first.lower());

  }

  return colorized_str + "\033[0;0m";;
}


int main(int argc, char** argv) {

  bpo::options_description all("Allowed options");
  bpo::options_description iface("Program options");
  bpo::options_description regex("Regex options");

  std::vector<std::string> red_regexs;
  std::vector<std::string> green_regexs;
  std::vector<std::string> blue_regexs;
  
  bool concatenation_allowed = true;
  bool use_multithread = true;
  bool use_reverse_colors = false;
  


  iface.add_options()
    ("help", "produce help message")
    ("zsh", "use zsh colors")
    ("bash", "use sh/zsh colors[this is default]")
    ("reverse-colors,R", "use reverse color binding (black <-> white)")
    ("no-concat,n", "disallow regex concatenation")
    ("no-multithread,N", "disable use mulitple threads (one thread per color, one for intput and one for output)\nhigher performance on one core devices")
  ;
    
  
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
    return EXIT_SUCCESS;
  }

  if (optionsmap.count("zsh") && optionsmap.count("bash")) {
    std::cout << "Invalid options, can't use --zsh and --bash at once" << std::endl << all << std::endl;
    return EXIT_FAILURE;
  }

  
  std::vector<std::regex> red_re_s = parse_regexes(red_regexs, concatenation_allowed);
  std::vector<std::regex> green_re_s = parse_regexes(green_regexs, concatenation_allowed);
  std::vector<std::regex> blue_re_s = parse_regexes(blue_regexs, concatenation_allowed);

  if (optionsmap.count("reverse-colors")) {
    std::cerr<<"Using reverse colors"<<std::endl;
    use_reverse_colors = true;
  }
    
  color_map_t* active_color_map;
  
  if (optionsmap.count("zsh")) {
    std::cerr << "Use zsh colors, Sorry not implemented!" << std::endl;
    return EXIT_FAILURE;
  }

  //if (optionsmap.count("bash")) {
    if(use_reverse_colors){
      active_color_map = &bash_reverse_colors;
    } else {
      active_color_map = &bash_colors;
    }
  //}
  
  if (optionsmap.count("no-concat")) {
    concatenation_allowed = false;
  }
  
  if (optionsmap.count("no-multithread")) {
    use_multithread = false;
  }
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
  
 
  std::string current_line;
  
  if( use_multithread ) {
    
    std::thread worker_red([]()
    {
      while(true){
        // Wait until main() sends data
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{return ready;});
    
        // after the wait, we own the lock.
        std::cout << "Worker thread is processing data red\n";
        data += " r";
    
        // Send data back to main()
        processed++;
        std::cout << "Worker thread signals data processing completed\n";
    
        // Manual unlocking is done before notifying, to avoid waking up
        // the waiting thread only to block again (see notify_one for details)
        lk.unlock();
        cv.notify_one();
      }
    });
    
    std::thread worker_blue([]()
    {
      while(true){
        // Wait until main() sends data
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{return ready;});
    
        // after the wait, we own the lock.
        std::cout << "Worker thread is processing data blue\n";
        data += " b";
    
        // Send data back to main()
        processed++;
        std::cout << "Worker thread signals data processing completed\n";
    
        // Manual unlocking is done before notifying, to avoid waking up
        // the waiting thread only to block again (see notify_one for details)
        lk.unlock();
        cv.notify_one();
      }
    });
    
    std::thread worker_green([]()
    {
       while(true){
        // Wait until main() sends data
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, []{return ready;});
    
        // after the wait, we own the lock.
        std::cout << "Worker thread is processing data green\n";
        data += " g";
    
        // Send data back to main()
        processed ++;
        std::cout << "Worker thread signals data processing completed\n";
    
        // Manual unlocking is done before notifying, to avoid waking up
        // the waiting thread only to block again (see notify_one for details)
        lk.unlock();
        cv.notify_one();
       }
    });
 
    while(std::getline(std::cin, current_line)){
      data = current_line;
      // send data to the worker thread
      {
          std::lock_guard<std::mutex> lk(m);
          ready = true;
          std::cout << "main() signals data ready for processing\n";
      }
      cv.notify_all();
  
      // wait for the worker
      {
          std::unique_lock<std::mutex> lk(m);
          cv.wait(lk, []{
            bool rv = processed==3;
            processed = 0;
            return rv;
          });
      }
      std::cout << "Back in main(), data = " << data << '\n';
    }
 
    worker_red.join();
    worker_green.join();
    worker_blue.join();
    
    
  } else {
    while (std::getline(std::cin, current_line)) {

      bil::interval_map<int, color_bit_t> lineparts;

      color_process(current_line, red_re_s, RED, lineparts);
      color_process(current_line, green_re_s, GREEN, lineparts);
      color_process(current_line, blue_re_s, BLUE, lineparts);
      
      std::cout<< colorize(current_line, *active_color_map, lineparts)<<std::endl;

    }
  }
  
  

  return 0;
}

