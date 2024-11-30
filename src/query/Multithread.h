#ifndef MULTITHREADS_H
#define MULTITHREADS_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../db/Database.h"
#include "QueryResult.h"

#define MIN_THREAD_REGION_SIZE 2000

class Thread_pool {
private:
  std::mutex mut;
  std::vector<std::thread> thread_vector;
  std::queue<std::function<void()>> task_queue;
  std::condition_variable condition_vari;
  std::atomic<bool> is_closed; // indice whether the thread pool is closed
  std::atomic<int> idle_thread_num;

  void push_task(const std::function<void()> &task) {
    std::lock_guard<std::mutex> const lock{mut};
    task_queue.push(task);
    condition_vari.notify_one();
  }

  std::function<void()> get_task() {
    std::unique_lock<std::mutex> lock{mut};
    if (is_closed)
      return std::function<void()>();
    while (task_queue.empty() && !is_closed)
      condition_vari.wait(lock);
    if (task_queue.empty()) // Double check after wake up
      return std::function<void()>();
    std::function<void()> task = std::move(task_queue.front());
    task_queue.pop();
    condition_vari.notify_one();
    return task;
  }

  void init_thread() {
    while (!is_closed.load()) {
      std::function<void()> const task = get_task();
      if (task) {
        idle_thread_num--;
        task();
        idle_thread_num++;
      }
    }
  }

public:
  Thread_pool() {
    idle_thread_num = 1;
    is_closed = false;
    thread_vector.emplace_back(&Thread_pool::init_thread, this);
  }

  void set_thread(int thread_num) {
    idle_thread_num = thread_num;
    for (int i = 0; i < thread_num; i++)
      thread_vector.emplace_back(&Thread_pool::init_thread, this);
  }

  int get_task_num() { return (int)task_queue.size(); }

  int get_idle_thread_num() { return idle_thread_num; }

  template <class F, class... Args>
  auto add_task(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    if (!task) {
      throw std::runtime_error("Failed to create task");
    }

    std::future<return_type> future = task->get_future();

    push_task([task]() { (*task)(); });

    return future;
  }

  ~Thread_pool() {
    is_closed.store(true);
    condition_vari.notify_all();
    for (std::thread &thread : thread_vector) {
      if (thread.joinable())
        thread.join();
    }
    while (!task_queue.empty())
      task_queue.pop();
    thread_vector.clear();
  }
};

#endif