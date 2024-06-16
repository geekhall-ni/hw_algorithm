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
  int affinity_machine_num;
  std::vector<int>affinity_machine_id;
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


int main() {
  int l, n, m, N, M; // 任务数，机器数，磁盘数，数据依赖数，环境依赖数

  std::cin >> l;
  std::vector<job*> job(l);
  for(int i = 0; i < l; ++i) {
    std::cin >> job[i]->id >> job[i]->task_size >> job[i]->output_data_size >> job[i]->affinity_machine_num;
    // 读取亲和性机器ID
    for(int j = 0; j < job[i]->affinity_machine_num; ++j) {
      int machineId;
      std::cin >> machineId;
      job[i]->affinity_machine_id.push_back(machineId);
    }
  }

  std::cin >> n; // 读取机器数
  std::vector<machine*> machine(n);
  for(int i = 0; i < n; ++i) {
    std::cin >> machine[i]->id >> machine[i]->power;
  }

  std::cin >> m; // 读取磁盘数
  std::vector<disk*> disk(m);
  for(int i = 0; i < m; ++i) {
    std::cin >> disk[i]->id >> disk[i]->speed >> disk[i]->quota;
    disk[i]->used_space = 0;
  }

  std::cin >> N; // 数据依赖数
  std::vector<std::pair<int, int>> data_dependence_job(N);
  for(int i = 0; i < N; ++i) {
    std::cin >> data_dependence_job[i].first >> data_dependence_job[i].second;
  }

  std::cin >> M; // 环境依赖数
  std::vector<std::pair<int, int>> environment_dependence_job(M);
  for(int i = 0; i < M; ++i) {
    std::cin >> environment_dependence_job[i].first >> environment_dependence_job[i].second;
  }
}
