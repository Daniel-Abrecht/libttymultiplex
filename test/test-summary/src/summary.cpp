#include <summary.h>
#include <map>
#include <queue>
#include <string>
#include <sstream>
#include <iterator>
#include <iostream>

enum sum_mode mode;

struct entry {
  struct entry* parent_node;
  int successful, total;
  std::map<std::string, struct entry> childs;

  void add(int total, int successful, std::queue<std::string>& chain, unsigned depth){
    this->total += total;
    this->successful += successful;
    if(mode == SM_TOTAL_ONLY)
      return;
    if(mode == SM_ONE_LEVEL && depth >= 1)
      return;
    if(chain.empty())
      return;
    std::string name = chain.front();
    chain.pop();
    struct entry& c = childs[name];
    c.add(total, successful, chain, depth+1);
  }
};

static struct entry top;

int add_entry(unsigned long total, unsigned long successful, char* line){
  if(mode == SM_SILENT)
    return 0;
  std::string sline = line;
  std::istringstream iss(sline);
  std::istream_iterator<std::string> isit(iss);
  std::queue<std::string> entries(std::deque<std::string>(isit, std::istream_iterator<std::string>()));
  top.add(total, successful, entries, 0);
  return 0;
}

int print_result(void){
  if(mode == SM_SILENT)
    return 0;
  std::cout << std::endl;
  if(mode == SM_ONE_LEVEL)
    for(std::map<std::string, struct entry>::iterator it=top.childs.begin(); it!=top.childs.end(); it++)
      if(it->second.total > 1)
        std::cout << "Result for " << it->first << ": \t" << it->second.successful << " Successful \t" << (it->second.total - it->second.successful) << " Failed \t" << it->second.total << " Total" << std::endl;
  if(mode == SM_TOTAL_ONLY || mode == SM_ONE_LEVEL)
    std::cout << "Summary: \t" << top.successful << " Successful \t" << (top.total - top.successful) << " Failed \t" << top.total << " Total" << std::endl;
  std::cout << std::endl;
  return 0;
}

