//
// Created by Defender on 2024/6/14.
//

#ifndef CodeCraftSDK_DATABASE_H_
#define CodeCraftSDK_DATABASE_H_

#include<iostream>
#include<vector>

class job{
public:
  int id;
  int task_size;
  int output_data_size;

  std::vector<int> data_dependence;
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

  int current_usage;

};

class task{
public:
  std::vector<job*> job;
  std::vector<machine*> machine;
  std::vector<disk*> disk;

  std::vector<int> environment_dependence;


};
#endif // CodeCraftSDK_DATABASE_H_
