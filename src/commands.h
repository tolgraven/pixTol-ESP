#pragma once

class Action { //wrap fns or whatever in uniform (String args) way so can be acted upon ezy
  public:
    Action(const String& id, );
  // std::pair<String, 
  // each possible arg needs to declare its type for later conversion back
  //
  // std::function<void(const String&)>; //one way. but will need moar flex
  // also want actions to return shit, presumably. ie for querying state of stuff, without
  // having print logic until Outputter...

  String _id, type;
  bool execute() {
  }
};
class ActionRunner {
  public:
  ActionRunner() {}
  ~ActionRunner()

  bool register()
};
//dunno if this shit could be fit into curr general design, so for now...
//this is for text commands, replacing curr mqtt/homie only crap
//homie should of course only pass on the input
// so bindings can be reused for serial, web, ssh(?) haha
// 

//instead of manual binds, and considering not often triggers,
//auto-wrap possible functions (bc arg crap) and
//
//let classes register all its eligeble fns?
// loose ones and other classes then we have to

// still, we gotta fit it. Make sure Buffer<String> actually works then?
// 
