#pragma once

#include <event/Condition.h>
#include <event/EventLoop.h>

#include <map>
#include <memory>

namespace event {

enum IOType {
  Read,
  Write,
};

class IOCondition: public Condition {
public:
  IOCondition(int fd, IOType type) :
      Condition(ConditionType::IO),
      fd(fd),
      type(type) {}

  int fd;
  IOType type;
};

class IOConditionManager: ConditionManager {
public:
  static IOCondition* canRead(int fd);
  static IOCondition* canWrite(int fd);

  IOCondition* canDo(int fd, IOType type);

  virtual void prepareConditions(std::vector<Condition*> const& conditions) override;

private:
  IOConditionManager();

  IOConditionManager(IOConditionManager const& copy) = delete;
  IOConditionManager& operator=(IOConditionManager const& copy) = delete;

  IOConditionManager(IOConditionManager const&& move) = delete;
  IOConditionManager& operator=(IOConditionManager const&& move) = delete;

  static IOConditionManager* instance;
  static IOConditionManager* getInstance();

  std::map<std::pair<IOType, int>, std::unique_ptr<IOCondition>> conditions_;
};

}
