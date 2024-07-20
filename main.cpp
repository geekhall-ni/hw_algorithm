#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

class Job {
public:
  int id;
  int task_size;
  int output_data_size;
  int affinity_machine_num;
  std::vector<int> affinity_machine_id;
};

class Machine {
public:
  int id;
  int power;
};

class Disk {
public:
  int id;
  int speed;
  int quota;
  int used_space; // 剩余空间
};

// 存储任务调度信息
struct ScheduleInfo {
  int taskId;
  int startTime;
  int machineId;
  int diskId;
};

// 辅助函数：解析数据依赖，构造任务到其依赖任务的映射
std::unordered_map<int, std::vector<int>> parseDataDependencies(const std::vector<std::pair<int, int>> &data_dependence_job, int l) {
  std::unordered_map<int, std::vector<int>> dataDependencies;
  for (const auto &dep : data_dependence_job) {
    dataDependencies[dep.second].push_back(dep.first);
  }
  return dataDependencies;
}

// 辅助函数：解析环境依赖，构造任务到其前置任务的映射
std::unordered_map<int, std::vector<int>> parseEnvironmentDependencies(const std::vector<std::pair<int, int>> &environment_dependence_job, int l) {
  std::unordered_map<int, std::vector<int>> envDependencies;
  for (const auto &dep : environment_dependence_job) {
    envDependencies[dep.second].push_back(dep.first);
  }
  return envDependencies;
}





// 计算数据从一个磁盘转移到另一个磁盘所需的时间
int calculateDataTransferTime(int dataSize, Disk *sourceDisk,
                              Disk *targetDisk) {
  if (sourceDisk == nullptr || targetDisk == nullptr) {
    std::cerr << "Error: Invalid Disk pointer." << std::endl;
    exit(EXIT_FAILURE);
  }

  int transferTimeSource = dataSize / sourceDisk->speed;
  int transferTimeTarget = dataSize / targetDisk->speed;
  return std::max(transferTimeSource, transferTimeTarget);
}

// 计算任务在机器上的执行时间
int calculateExecutionTime(Job *task, Machine *machine) {
  if (task->output_data_size == 0) {
    return task->task_size / machine->power;
  } else {
    return (task->task_size + task->output_data_size) / machine->power;
  }
}

int calculateStartTimeForDependencies(
    std::vector<Job *> &jobs, std::vector<Machine *> &machines,
    std::vector<Disk *> &disks,
    std::unordered_map<int, std::vector<int>> &dataDependencies,
    std::unordered_map<int, std::vector<int>> &envDependencies, Job *currentJob,
    int machineId, int diskId, std::vector<ScheduleInfo> &scheduleInfos) {
  int startTime = 0;

  // 处理环境依赖
  if (envDependencies.find(currentJob->id) != envDependencies.end()) {
    for (int envDepId : envDependencies[currentJob->id]) {
      startTime = std::max(
          startTime,
          scheduleInfos[envDepId].startTime +
              calculateExecutionTime(
                  jobs[envDepId], machines[scheduleInfos[envDepId].machineId]) +
              calculateDataTransferTime(jobs[envDepId]->output_data_size,
                                        disks[scheduleInfos[envDepId].diskId],
                                        disks[diskId]));
    }
  }

  // 处理数据依赖
  if (dataDependencies.find(currentJob->id) != dataDependencies.end()) {
    for (int dataDepId : dataDependencies[currentJob->id]) {
      Disk *sourceDisk = disks[scheduleInfos[dataDepId].diskId];
      Disk *targetDisk = disks[diskId];

      if (sourceDisk == nullptr || targetDisk == nullptr) {
        std::cerr << "Error: Invalid Disk pointer." << std::endl;
        exit(EXIT_FAILURE);
      }

      startTime = std::max(
          startTime,
          scheduleInfos[dataDepId].startTime +
              calculateExecutionTime(
                  jobs[dataDepId],
                  machines[scheduleInfos[dataDepId].machineId]) +
              calculateDataTransferTime(jobs[dataDepId]->output_data_size,
                                        sourceDisk, targetDisk));
    }
  }

  return startTime;
}

// 主要的调度函数
void scheduleTasks(std::vector<Job *> &jobs, std::vector<Machine *> &machines,
                   std::vector<Disk *> &disks,
                   std::unordered_map<int, std::vector<int>> &dataDependencies,
                   std::unordered_map<int, std::vector<int>> &envDependencies,
                   std::vector<ScheduleInfo> &scheduleInfos,
                   int currentJobIndex, int l, int m, int &makespan) {
  if (currentJobIndex == l) {
    makespan = std::max(
        makespan,
        scheduleInfos.back().startTime +
            calculateExecutionTime(jobs.back(),
                                   machines[scheduleInfos.back().machineId]));
    return;
  }

  Job *currentJob = jobs[currentJobIndex];

  for (int machineId : currentJob->affinity_machine_id) {
    for (int diskId = 0; diskId < m; ++diskId) {
      if (disks[diskId]->used_space + currentJob->output_data_size <=
          disks[diskId]->quota) {
        int originalDiskSpace = disks[diskId]->used_space;
        disks[diskId]->used_space += currentJob->output_data_size;

        int startTime = calculateStartTimeForDependencies(
            jobs, machines, disks, dataDependencies, envDependencies,
            currentJob, machineId, diskId, scheduleInfos);

        std::cout << "Attempting to schedule job "<< currentJob->id<< std::endl;
        std::cout << "Calculated start time for job "<< currentJob->id << " : "<< startTime<< std::endl;
        std::cout << "Recursively scheduling job "<< currentJob->id << " with start time "<< startTime<< std::endl;

        int executionTime =
            calculateExecutionTime(currentJob, machines[machineId]);
        int transferTime = calculateDataTransferTime(
            currentJob->output_data_size, disks[diskId], disks[diskId]);

        if (startTime + executionTime + transferTime< makespan) {
          scheduleInfos.push_back(
              {currentJob->id, startTime, machineId, diskId});
          scheduleTasks(jobs, machines, disks, dataDependencies,
                        envDependencies, scheduleInfos, currentJobIndex + 1, l,
                        m, makespan);
          scheduleInfos.pop_back();
        }

        disks[diskId]->used_space = originalDiskSpace;

        std::cout << "Backtracking from job"<< currentJob->id<< std::endl;
      }
    }
  }
}

int main() {
  int l, n, m, N, M; // 任务数，机器数，磁盘数，数据依赖数，环境依赖数

  std::cin >> l; // 读取任务数
  std::vector<Job *> job(l, nullptr);
  for (int i = 0; i < l; ++i) {
    job[i] = new class Job();
    std::cin >> job[i]->id >> job[i]->task_size >> job[i]->output_data_size >>
        job[i]->affinity_machine_num;
    // 读取亲和性机器ID
    for (int j = 0; j < job[i]->affinity_machine_num; ++j) {
      int machineId;
      std::cin >> machineId;
      job[i]->affinity_machine_id.push_back(machineId);
    }
  }

  std::cin >> n; // 读取机器数
  std::vector<Machine *> machine(n, nullptr);
  for (int i = 0; i < n; ++i) {
    machine[i] = new class Machine();
    std::cin >> machine[i]->id >> machine[i]->power;
  }

  std::cin >> m; // 读取磁盘数
  std::vector<Disk *> disk(m, nullptr);
  for (int i = 0; i < m; ++i) {
    disk[i] = new class Disk();
    std::cin >> disk[i]->id >> disk[i]->speed >> disk[i]->quota;
    disk[i]->used_space = 0;
  }

  std::cin >> N; // 数据依赖数
  std::vector<std::pair<int, int>> data_dependence_job(N);
  for (int i = 0; i < N; ++i) {
    std::cin >> data_dependence_job[i].first >> data_dependence_job[i].second;
  }

  std::cin >> M; // 环境依赖数
  std::vector<std::pair<int, int>> environment_dependence_job(M);
  for (int i = 0; i < M; ++i) {
    std::cin >> environment_dependence_job[i].first >>
        environment_dependence_job[i].second;
  }

//  // 输出所有读入的数据
//  std::cout << "Job:" << std::endl;
//  for (int i = 0; i < l; ++i) {
//    std::cout << "id:" << job[i]->id << " task_size:" << job[i]->task_size
//              << " output_data_size:" << job[i]->output_data_size
//              << " affinity_machine_num:" << job[i]->affinity_machine_num
//              << " affinity_machine_id:";
//    for (int affinity_id : job[i]->affinity_machine_id) {
//      std::cout << affinity_id << " ";
//    }
//    std::cout << std::endl;
//  }
//  std::cout << std::endl;
//
//  std::cout << "Machine:" << std::endl;
//  for (int i = 0; i < n; ++i) {
//    std::cout << "id:" << machine[i]->id << " power:" << machine[i]->power
//              << std::endl;
//  }
//  std::cout << std::endl;
//
//  std::cout << "Disk:" << std::endl;
//  for (int i = 0; i < m; ++i) {
//    std::cout << "id:" << disk[i]->id << " speed:" << disk[i]->speed
//              << " quota:" << disk[i]->quota
//              << " used_space:" << disk[i]->used_space << std::endl;
//  }
//  std::cout << std::endl;
//
//  std::cout << N << std::endl;
//  std::cout << "data_dependence_job:" << std::endl;
//  for (int i = 0; i < N; ++i) {
//    std::cout << "first:" << data_dependence_job[i].first
//              << " second:" << data_dependence_job[i].second << std::endl;
//  }
//  std::cout << std::endl;
//
//  std::cout << M << std::endl;
//  std::cout << "environment_dependence_job:" << std::endl;
//  for (int i = 0; i < M; ++i) {
//    std::cout << "first:" << environment_dependence_job[i].first
//              << " second:" << environment_dependence_job[i].second
//              << std::endl;
//  }
//  std::cout << std::endl;

  // 解析依赖关系
  std::unordered_map<int, std::vector<int>> dataDependencies =
      parseDataDependencies(data_dependence_job, l);
  std::unordered_map<int, std::vector<int>> envDependencies =
      parseEnvironmentDependencies(environment_dependence_job, l);

//  // 输出解析后的依赖关系
//  std::cout << "Parsed data dependencies: "<< std::endl;
//  for (const auto &kv : dataDependencies) {
//    std::cout << kv.first << ": ";
//    for (int dep : kv.second) {
//      std::cout<< dep << " ";
//    }
//    std::cout<< std::endl;
//  }
//
//  std::cout << "Parsed environment dependencies: "<< std::endl;
//  for (const auto &kv : envDependencies) {
//    std::cout << kv.first << ": ";
//    for (int dep : kv.second) {
//      std::cout<< dep << " ";
//    }
//    std::cout<< std::endl;
//  }

  // 初始化调度信息和最大完成时间
  std::vector<ScheduleInfo> scheduleInfos; // 声明 scheduleInfos
  int makespan = INT_MAX;

  // 调度任务
  scheduleTasks(job, machine, disk, dataDependencies, envDependencies,
                scheduleInfos, 0, l, m, makespan);

  // 输出调度结果
  std::cout << "Final Schedule:" << std::endl;
  for (const auto &info : scheduleInfos) {
    std::cout << "Task " << info.taskId << ": Start Time=" << info.startTime
              << ", Machine=" << info.machineId << ", Disk=" << info.diskId
              << std::endl;
  }
  std::cout << "Makespan: " << makespan << std::endl;

  // 释放内存
  for (auto &j : job)
    delete j;
  for (auto &m : machine)
    delete m;
  for (auto &d : disk)
    delete d;

  return 0;
}
