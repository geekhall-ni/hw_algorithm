#include <algorithm>
#include <climits>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>

// 任务类
class Task {
public:
  int id;
  int size;
  int output_size;
  std::vector<int> affinitive_machines; // 亲和机器列表
  std::vector<int> dependencies; // 数据依赖
  std::vector<int> env_dependencies; // 环境依赖
};

// 机器类
class Machine {
public:
  int id;
  int power;
};

// 磁盘类
class Disk {
public:
  int id;
  int speed;
  int quota;
};

// 遗传算法参数
const int POPULATION_SIZE = 100;
const int MAX_GENERATIONS = 1000;
const double MUTATION_RATE = 0.3;
const double CROSSOVER_RATE = 0.8;

int fitness(const std::vector<Task> &tasks, const std::vector<Machine> &machines,
            const std::vector<Disk> &disks, const std::vector<int> &schedule) {
  int makespan = 0;
  std::vector<int> task_end_times(tasks.size(), 0);
  std::vector<int> machine_end_times(machines.size(), 0);
  std::vector<int> disk_end_times(disks.size(), 0);
  std::vector<int> disk_data_usage(disks.size(), 0);
  std::vector<std::vector<std::pair<int, int>>> machine_tasks(machines.size());

  for (int i = 0; i < tasks.size(); i++) {
    int task_id = schedule[i * 3];
    int machine_id = schedule[i * 3 + 1];
    int disk_id = schedule[i * 3 + 2];

    // 验证任务、机器、磁盘ID的合法性
    if (task_id < 0 || task_id >= tasks.size() ||
        machine_id < 0 || machine_id >= machines.size() ||
        disk_id < 0 || disk_id >= disks.size()) {
      return INT_MAX; // 无效的任务、机器或磁盘ID
    }

    // 检查任务与机器的亲和性约束
    if (std::find(tasks[task_id].affinitive_machines.begin(), tasks[task_id].affinitive_machines.end(), machine_id) == tasks[task_id].affinitive_machines.end()) {
      return INT_MAX; // 不符合亲和性约束
    }

    int task_start_time = 0;

    // 计算任务开始时间（考虑数据和环境依赖）
    for (int dep : tasks[task_id].dependencies) {
      if (dep >= 0 && dep < tasks.size()) {
        task_start_time = std::max(task_start_time, task_end_times[dep]);
      }
    }

    for (int env_dep : tasks[task_id].env_dependencies) {
      if (env_dep >= 0 && env_dep < tasks.size()) {
        task_start_time = std::max(task_start_time, task_end_times[env_dep]);
      }
    }

    // 确保任务开始时间不早于机器和磁盘的空闲时间
    task_start_time = std::max(task_start_time, machine_end_times[machine_id]);
    task_start_time = std::max(task_start_time, disk_end_times[disk_id]);

    // 计算读取时间
    int read_time = 0;
    for (int dep : tasks[task_id].dependencies) {
      if (dep >= 0 && dep < tasks.size()) {
        read_time += (double)tasks[dep].output_size / disks[disk_id].speed;
      }
    }

    // 计算执行时间和写入时间
    int execute_time = (tasks[task_id].size + machines[machine_id].power - 1) / machines[machine_id].power; // 向上取整
    int write_time = (tasks[task_id].output_size + disks[disk_id].speed - 1) / disks[disk_id].speed; // 向上取整

    // 计算任务结束时间
    int task_end_time = task_start_time + read_time + execute_time + write_time;
    task_end_times[task_id] = task_end_time;
    machine_end_times[machine_id] = std::max(machine_end_times[machine_id], task_end_time);
    disk_end_times[disk_id] = std::max(disk_end_times[disk_id], task_end_time);

    disk_data_usage[disk_id] += tasks[task_id].output_size;

    // 检查磁盘配额是否超出
    if (disk_data_usage[disk_id] > disks[disk_id].quota) {
      return INT_MAX; // 磁盘配额超出
    }

    // 检查机器上任务是否重叠
    for (auto &interval : machine_tasks[machine_id]) {
      if (task_start_time < interval.second) {
        return INT_MAX; // 任务重叠
      }
    }
    machine_tasks[machine_id].emplace_back(task_start_time, task_end_time);

    // 更新makespan
    makespan = std::max(makespan, task_end_time);
  }

  return makespan;
}


// 初始化种群
std::vector<std::vector<int>> init_population(int population_size,
                                              int num_tasks, int num_machines,
                                              int num_disks) {
  std::vector<std::vector<int>> population;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis_machine(0, num_machines - 1);
  std::uniform_int_distribution<> dis_disk(0, num_disks - 1);

  for (int i = 0; i < population_size; i++) {
    std::vector<int> individual(num_tasks * 3);
    for (int j = 0; j < num_tasks; j++) {
      individual[j * 3] = j; // 任务ID分配
      individual[j * 3 + 1] = dis_machine(gen); // 随机选择机器
      individual[j * 3 + 2] = dis_disk(gen); // 随机选择磁盘
    }

    population.push_back(individual);
  }
  return population;
}

// 选择操作
std::vector<std::vector<int>> selection(const std::vector<std::vector<int>> &population,
                                        const std::vector<double> &fitnesses) {
  std::vector<std::vector<int>> new_population;
  std::vector<double> probabilities(fitnesses.size());
  double sum_fitness = 0.0;

  for (double fit : fitnesses) {
    sum_fitness += fit;
  }

  if (sum_fitness == 0) {
    std::cerr << "总适应度为零，无法进行选择。" << std::endl;
    return new_population; // 返回空的种群
  }

  for (int i = 0; i < fitnesses.size(); i++) {
    probabilities[i] = (fitnesses[i] > 0) ? fitnesses[i] / sum_fitness : 0;
  }
  for (int i = 0; i < fitnesses.size(); i++) {
    probabilities[i] = fitnesses[i] / sum_fitness;
  }

  std::discrete_distribution<> dist(probabilities.begin(), probabilities.end());
  std::mt19937 gen(std::random_device{}());

  for (int i = 0; i < POPULATION_SIZE; i++) {
    int index = dist(gen);
    new_population.push_back(population[index]);
  }

  return new_population;
}


// 交叉操作
std::vector<std::vector<int>> crossover(const std::vector<int> &parent1,
                                        const std::vector<int> &parent2) {
  std::vector<int> offspring1 = parent1;
  std::vector<int> offspring2 = parent2;

  if (static_cast<double>(std::rand()) / RAND_MAX < CROSSOVER_RATE) {
    int crossover_point = std::rand() % (parent1.size() / 3);

    for (int i = crossover_point; i < parent1.size() / 3; i++) {
      std::swap(offspring1[i * 3], offspring2[i * 3]);
      std::swap(offspring1[i * 3 + 1], offspring2[i * 3 + 1]);
      std::swap(offspring1[i * 3 + 2], offspring2[i * 3 + 2]);
    }
  }

  return {offspring1, offspring2};
}

// 变异操作
void mutation(std::vector<int> &individual, int num_tasks, int num_machines, int num_disks) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dis(0.0, 1.0);
  std::uniform_int_distribution<> dis_machine(0, num_machines - 1);
  std::uniform_int_distribution<> dis_disk(0, num_disks - 1);

  for (int i = 0; i < individual.size() / 3; i++) {
    if (dis(gen) < MUTATION_RATE) {
      individual[i * 3 + 1] = dis_machine(gen); // 变异机器ID
      individual[i * 3 + 2] = dis_disk(gen); // 变异磁盘ID
    }
  }
}

int main() {

  // 初始化随机数种子
  std::srand(static_cast<unsigned int>(std::time(nullptr)));

  int num_tasks, num_machines, num_disks;
  std::cin >> num_tasks;

  if (num_tasks <= 0) {
    std::cerr << "无效的任务数量。" << std::endl;
    return -1;
  }

  std::vector<Task> tasks(num_tasks);
  for (int i = 0; i < num_tasks; i++) {
    std::cin >> tasks[i].id >> tasks[i].size >> tasks[i].output_size;
    tasks[i].id -= 1; // 转换为从0开始
    if (tasks[i].id < 0 || tasks[i].size <= 0 || tasks[i].output_size < 0) {
      std::cerr << "无效的任务数据。" << std::endl;
      return -1;
    }

    int num_affinitive_machines;
    std::cin >> num_affinitive_machines;
    for (int j = 0; j < num_affinitive_machines; j++) {
      int affinitive_machine;
      std::cin >> affinitive_machine;
      affinitive_machine -= 1; // 转换为从0开始
      if (affinitive_machine < 0 || affinitive_machine >= num_machines) {
        std::cerr << "无效的机器ID。" << std::endl;
        return -1;
      }
      tasks[i].affinitive_machines.push_back(affinitive_machine);
    }
  }

  std::cin >> num_machines;
  if (num_machines <= 0) {
    std::cerr << "无效的机器数量。" << std::endl;
    return -1;
  }

  std::vector<Machine> machines(num_machines);
  for (int i = 0; i < num_machines; i++) {
    std::cin >> machines[i].id >> machines[i].power;
    machines[i].id -= 1; // 转换为从0开始
    if (machines[i].id < 0 || machines[i].power <= 0) {
      std::cerr << "无效的机器数据。" << std::endl;
      return -1;
    }
  }

  std::cin >> num_disks;
  if (num_disks <= 0) {
    std::cerr << "无效的磁盘数量。" << std::endl;
    return -1;
  }

  std::vector<Disk> disks(num_disks);
  for (int i = 0; i < num_disks; i++) {
    std::cin >> disks[i].id >> disks[i].speed >> disks[i].quota;
    disks[i].id -= 1; // 转换为从0开始
    if (disks[i].id < 0 || disks[i].speed <= 0 || disks[i].quota < 0) {
      std::cerr << "无效的磁盘数据。" << std::endl;
      return -1;
    }
  }

  // 读取数据依赖
  int num_data_dependencies;
  std::cin >> num_data_dependencies;
  if (num_data_dependencies > 0) {  // 确保有数据依赖时才读取
    for (int i = 0; i < num_data_dependencies; i++) {
      int task_id1, task_id2;
      std::cin >> task_id1 >> task_id2;
      task_id1 -= 1; // 转换为从0开始
      task_id2 -= 1; // 转换为从0开始
      if (task_id1 < 0 || task_id1 >= num_tasks || task_id2 < 0 || task_id2 >= num_tasks) {
        std::cerr << "无效的数据依赖。" << std::endl;
        return -1;
      }
      tasks[task_id2].dependencies.push_back(task_id1);
    }
  }

  // 读取环境依赖
  int num_env_dependencies;
  std::cin >> num_env_dependencies;
  if (num_env_dependencies > 0) {  // 确保有环境依赖时才读取
    for (int i = 0; i < num_env_dependencies; i++) {
      int task_id1, task_id2;
      std::cin >> task_id1 >> task_id2;
      task_id1 -= 1; // 转换为从0开始
      task_id2 -= 1; // 转换为从0开始
      if (task_id1 < 0 || task_id1 >= num_tasks || task_id2 < 0 || task_id2 >= num_tasks) {
        std::cerr << "无效的环境依赖。" << std::endl;
        return -1;
      }
      tasks[task_id2].env_dependencies.push_back(task_id1);
    }
  }


  // 初始化种群
  std::vector<std::vector<int>> population =
      init_population(POPULATION_SIZE, num_tasks, num_machines, num_disks);

  // 遗传算法主循环
  for (int generation = 0; generation < MAX_GENERATIONS; generation++) {
    std::vector<double> fitnesses(POPULATION_SIZE);
    for (int i = 0; i < POPULATION_SIZE; i++) {
      int fit = fitness(tasks, machines, disks, population[i]);
      fitnesses[i] = (fit == INT_MAX) ? 0.0 : 1.0 / fit;
    }

    // 选择操作
    std::vector<std::vector<int>> new_population = selection(population, fitnesses);

    // 交叉操作
    std::vector<std::vector<int>> offspring;
    for (int i = 0; i < POPULATION_SIZE / 2; i++) {
      auto children = crossover(new_population[i], new_population[POPULATION_SIZE - i - 1]);
      offspring.push_back(children[0]);
      offspring.push_back(children[1]);
    }

    // 变异操作
    for (auto &ind : offspring) {
      mutation(ind, num_tasks, num_machines, num_disks);
    }

    // 更新种群
    population = new_population;
  }

  // 输出结果
  std::vector<int> task_end_times(num_tasks, 0);
  int min_makespan = INT_MAX;
  std::vector<int> best_individual;

  for (int i = 0; i < POPULATION_SIZE; i++) {
    int current_makespan = fitness(tasks, machines, disks, population[i]);
    if (current_makespan < min_makespan) {
      min_makespan = current_makespan;
      best_individual = population[i];
    }
  }

  // 打印最佳个体的任务调度情况
  std::vector<int> best_task_end_times(num_tasks, 0);
  for (int j = 0; j < num_tasks; j++) {
    int task_id = best_individual[j * 3];
    int machine_id = best_individual[j * 3 + 1];
    int disk_id = best_individual[j * 3 + 2];

    // 计算任务开始时间
    int task_start_time = 0;
    for (int dep : tasks[task_id].dependencies) {
      task_start_time = std::max(task_start_time, best_task_end_times[dep]);
    }

    int execute_time = (double)tasks[task_id].size / machines[machine_id].power;
    int read_time = 0;
    for (int dep : tasks[task_id].dependencies) {
      read_time += (tasks[dep].output_size + disks[disk_id].speed - 1) / disks[disk_id].speed; // 向上取整
    }

    int write_time = (double)tasks[task_id].output_size / disks[disk_id].speed;

    int task_end_time = task_start_time + read_time + execute_time + write_time;
    best_task_end_times[task_id] = task_end_time;

    // 输出任务调度情况
    std::cout << task_id + 1 << " " << task_start_time << " "
              << machine_id + 1 << " " << disk_id + 1 << std::endl;
  }

  return 0;
}
