#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>
#include <string>
#include <ges/dispatcher.hpp>

ges::dispatcher events;

struct MyEvent {
  std::string msg;
};

struct OnCollision {
  uint64_t entity;
  uint64_t other;
  float force;
};

int main()
{
  
}