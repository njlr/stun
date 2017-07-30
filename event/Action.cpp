#include "event/Action.h"

#include <iostream>

namespace event {

Action::Action(std::vector<Condition*> conditions) : conditions_(conditions) {
  EventLoop::getCurrentLoop()->addAction(this);
}

Action::~Action() { EventLoop::getCurrentLoop()->removeAction(this); }

void Action::invoke() { callback.invoke(); }

bool Action::canInvoke() {
  for (auto condition : conditions_) {
    if (!EventLoop::getCurrentLoop()->hasCondition(condition) ||
        !condition->eval()) {
      return false;
    }
  }

  return true;
}
}
