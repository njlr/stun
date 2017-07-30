#include "event/Timer.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

namespace event {

class TimerManager {
public:
  static void setTimeout(Time target, BaseCondition* condition);
  static void removeTimeout(BaseCondition* condition);

private:
  TimerManager();

  typedef std::pair<Time, BaseCondition*> TimeoutTrigger;

  static TimerManager* instance_;
  std::vector<TimeoutTrigger> targets_;
  Time currentTarget_ = 0;
  timer_t timer_;

  static void handleSignal(int sig, siginfo_t* si, void* uc);

  void sortTargets();
  void updateTimer(Time now);
  void fireUntilTarget();
};

const int kTimerManagerSignal = SIGRTMIN;
const Duration kMillisecondsInASecond = 1000;
const Duration kNanosecondsInAMillisecond = 1000000;

TimerManager* TimerManager::instance_ = new TimerManager();

TimerManager::TimerManager() {
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = &TimerManager::handleSignal;
  sigemptyset(&sa.sa_mask);

  // Bind the timer singal handler
  if (sigaction(kTimerManagerSignal, &sa, NULL) < 0) {
    throw std::runtime_error("Cannot bind timer signal handler.");
  }

  // Create the timer
  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = kTimerManagerSignal;
  sev.sigev_value.sival_ptr = &timer_;
  if (timer_create(CLOCK_MONOTONIC, &sev, &timer_) < 0) {
    throw std::runtime_error("Cannot create timer.");
  }
}

void TimerManager::sortTargets() {
  std::sort(targets_.begin(), targets_.end(), std::greater<TimeoutTrigger>());
}

/* static */ void TimerManager::setTimeout(Time target,
                                           BaseCondition* condition) {
  auto existing =
      std::find_if(instance_->targets_.begin(), instance_->targets_.end(),
                   [condition](TimeoutTrigger const& trigger) {
                     return trigger.second == condition;
                   });

  if (existing == instance_->targets_.end()) {
    instance_->targets_.emplace_back(target, condition);
  } else {
    existing->first = target;
  }

  Time now = Timer::getTimeInMilliseconds();
  instance_->sortTargets();
  instance_->updateTimer(now);
}

/* static */ void TimerManager::removeTimeout(BaseCondition* condition) {
  auto existing =
      std::find_if(instance_->targets_.begin(), instance_->targets_.end(),
                   [condition](TimeoutTrigger const& trigger) {
                     return trigger.second == condition;
                   });

  if (existing != instance_->targets_.end()) {
    instance_->targets_.erase(existing);
  }
}

Time Timer::Timer::getTimeInMilliseconds() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * kMillisecondsInASecond +
          ts.tv_nsec / kNanosecondsInAMillisecond);
}

void TimerManager::handleSignal(int sig, siginfo_t* si, void* uc) {
  instance_->fireUntilTarget();
}

void TimerManager::fireUntilTarget() {
  Time now = Timer::getTimeInMilliseconds();
  Time target = std::max(now, currentTarget_);
  while (!targets_.empty() && targets_.back().first <= target) {
    TimeoutTrigger fired = targets_.back();
    targets_.pop_back();
    fired.second->fire();
  }
  currentTarget_ = 0;
  if (targets_.empty()) {
    return;
  }
  updateTimer(now);
}

void TimerManager::updateTimer(Time now) {
  if (currentTarget_ != 0 && targets_.back().first >= currentTarget_) {
    return;
  }

  Duration timeout =
      std::max(1L, (int64_t)targets_.back().first - (int64_t)now);

  struct itimerspec its;
  its.it_value.tv_sec = timeout / kMillisecondsInASecond;
  its.it_value.tv_nsec =
      (timeout % kMillisecondsInASecond) * kNanosecondsInAMillisecond;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;

  if (timer_settime(instance_->timer_, 0, &its, NULL) < 0) {
    throw std::runtime_error("Cannot set timer time.");
  }

  currentTarget_ = now + timeout;
}

Timer::Timer(Duration timeout) : didFire_(new BaseCondition()) {
  reset(timeout);
}

Timer::~Timer() { TimerManager::removeTimeout(didFire_.get()); }

Condition* Timer::didFire() { return didFire_.get(); }

void Timer::reset() { didFire_.get()->arm(); }

void Timer::reset(Duration timeout) {
  reset();
  Time now = getTimeInMilliseconds();
  target_ = now + timeout;
  TimerManager::setTimeout(target_, didFire_.get());
}

void Timer::extend(Duration timeout) {
  reset();
  target_ += timeout;
  TimerManager::setTimeout(target_, didFire_.get());
}
}
