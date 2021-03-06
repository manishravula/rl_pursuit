#ifndef MODEL_RSATLNKK
#define MODEL_RSATLNKK

/*
File: Model.h
Author: Samuel Barrett
Description: an abstract model for planning
Created:  2011-08-23
Modified: 2011-08-23
*/

#include <string>

template<class State, class Action>
class Model {
public:
  Model () {}
  virtual ~Model () {}

  virtual void setState(const State &state) = 0;
  virtual void takeAction(const Action &action, float &reward, State &state, bool &terminal) = 0;

  virtual std::string generateDescription(unsigned int indentation = 0) = 0;
};

#endif /* end of include guard: MODEL_RSATLNKK */
