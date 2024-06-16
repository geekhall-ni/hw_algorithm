#include <iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<vector>

class job{
public:
  int id;
  int task_size;
  int output_data_size;

  std::vector<int> data_dependence_job;
  std::vector<int> affinity_machine;

};

class machine{
public:
  int id;
  int power;

};

class disk{
public:
  int id;
  int speed;
  int quota;
  int used_space; //剩余空间

};

class task{
public:
  std::vector<job*> job;
  std::vector<machine*> machine;
  std::vector<disk*> disk;

  std::vector<int> environment_dependence;
};

std::vector<task*> m_parser()
{
  std::ifstream ifs;
  ifs.open("",std::ios::in);
  if(!ifs.is_open())
  {
    std::cout << "fail to open the file" << std::endl;
  }
  else
  {

  }
}
int main() {

}
