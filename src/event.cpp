#include "event.h"

namespace tol {

namespace event {


// Msg::Msg(int msgId):
//   // msgId_(msgId), uniqueId_(generateUniqueId()) {}
//   Named(),
//   msgId_(msgId) {}


void Queue::put(Msg&& msg) {
  queue_.push(msg.move());
}

std::unique_ptr<Msg> Queue::get() {
  std::unique_ptr<Msg> msg{nullptr};
  if(!queue_.empty()) {
    msg = queue_.front()->move();
    queue_.pop();
  }
  return msg;
}

Queue bus;

}
}
