#pragma once

#include <string>
#include <unordered_map>

#include "base.h"
#include "log.h"
#include "util.h"

namespace tol {
// Ultimately want to be able to store any fn/args cb dynamically,
// and parse eg json -> tuple -> call or construct using template funkiness.
// Sooo keep lurning til got that down, but for now maybe wrapping will do.
// This will then be used by cmdline/mqtt/web/restore from settings to actually set up
// system.
// 
// template<class... Args>
// void DEBUG2(const Args&&... args) {
// using command_t = std::function<void(const std::string& subcommand, const std::string& args)>;
using command_t = std::function<void(const std::string& subcommand,
                                     const std::vector<std::string>& args)>;

//class Command: public Named {
//  public:
//    Command(const std::string& id, const std::string& type, command_t cmd):
//      Named(id, type), cmd(cmd);
//  // std::pair<std::string,
//  // each possible arg needs to declare its type for later conversion back
//  //
//  // std::function<void(const std::string&)>; //one way. but will need moar flex
//  // also want actions to return shit, presumably. ie for querying state of stuff, without
//  // having print logic until Outputter...

//  bool execute() {}
//};
class CommandRunner { // right so this incarnation = dumb = parse strings n shit
  public:
  CommandRunner() {}
  ~CommandRunner() {}

  void add(const std::string& id, command_t command) {
    cmds.emplace(id, command);
  }
  void execute(const std::string& id, const std::string& args) {
    if(auto it = cmds.find(id); it != cmds.end()) {
      
      it->second(id, util::split(args, "\n"));
    } else {
      lg.err("Command not found " + (std::string)id.c_str(), "CommandRunner");
    }
  }

  std::unordered_map<std::string, command_t> cmds;
  // std::tuple
};

}
