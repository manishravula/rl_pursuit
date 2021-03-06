#ifndef MCTS_MJ647W13
#define MCTS_MJ647W13

/*
File: MCTS.h
Author: Samuel Barrett
Description: a monte-carlo tree search
Created:  2011-08-23
Modified: 2011-12-13
*/

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <sstream>
#include "Model.h"
#include "ValueEstimator.h"
#include <common/Util.h>
#include <controller/ModelUpdater.h>

//#define MCTS_DEBUG

#ifdef MCTS_DEBUG
#define MCTS_OUTPUT(x) std::cout << x << std::endl
#else
#define MCTS_OUTPUT(x) ((void) 0)
#endif

template<class State, class Action>
class MCTS {
public:
  MCTS (boost::shared_ptr<ValueEstimator<State,Action> > valueEstimator, boost::shared_ptr<ModelUpdater> modelUpdater, unsigned int numPlayouts, double maxPlanningTime, unsigned int maxDepth, int pruningMemorySize);
  virtual ~MCTS () {}

  void search(const State &startState);
  Action selectWorldAction(const State &state);
  void restart();
  std::string generateDescription(unsigned int indentation = 0);
  void pruneOldVisits() {
    valueEstimator->pruneOldVisits(pruningMemorySize);
  }

private:
  void checkInternals();
  void rollout(const State &startState);

private:
  boost::shared_ptr<ValueEstimator<State,Action> > valueEstimator;
  boost::shared_ptr<ModelUpdater> modelUpdater;
  unsigned int numPlayouts;
  double maxPlanningTime;
  unsigned int maxDepth;
  bool valid;
  double endPlanningTime;
  int pruningMemorySize;
};

////////////////////////////////////////////////////////////////////////////

template<class State, class Action>
MCTS<State,Action>::MCTS(boost::shared_ptr<ValueEstimator<State,Action> > valueEstimator, boost::shared_ptr<ModelUpdater> modelUpdater, unsigned int numPlayouts, double maxPlanningTime, unsigned int maxDepth, int pruningMemorySize):
  valueEstimator(valueEstimator),
  modelUpdater(modelUpdater),
  numPlayouts(numPlayouts),
  maxPlanningTime(maxPlanningTime),
  maxDepth(maxDepth),
  pruningMemorySize(pruningMemorySize)
{
  checkInternals();
}

template<class State, class Action>
void MCTS<State,Action>::search(const State &startState) {
  endPlanningTime = getTime() + maxPlanningTime;
  for (unsigned int i = 0; (numPlayouts == 0) || (i < numPlayouts); i++) {
    MCTS_OUTPUT("-----------------------------------");
    MCTS_OUTPUT("ROLLOUT: " << i);
    if ((maxPlanningTime > 0) && (getTime() > endPlanningTime))
      break;
    rollout(startState);
    MCTS_OUTPUT("-----------------------------------");
  }
}

template<class State, class Action>
Action MCTS<State,Action>::selectWorldAction(const State &state) {
  return valueEstimator->selectWorldAction(state);
}

template<class State, class Action>
void MCTS<State,Action>::restart() {
  valueEstimator->restart();
}

template<class State, class Action>
std::string MCTS<State,Action>::generateDescription(unsigned int indentation) {
  std::stringstream ss;
  std::string prefix = indent(indentation);
  ss << prefix << "num playouts: " << numPlayouts << "\n";
  ss << prefix << "max planning time: " << maxPlanningTime << "\n";
  ss << prefix << "max depth: " << maxDepth << "\n";
  ss << prefix << "pruning memory size: " << pruningMemorySize << "\n";
  ss << prefix << "ValueEstimator:\n";
  ss << valueEstimator->generateDescription(indentation+1) << "\n";
  //ss << prefix << "Model:\n";
  //ss << model->generateDescription(indentation+1) << "\n"; // TODO removed during model change
  return ss.str();
}


template<class State, class Action>
void MCTS<State,Action>::checkInternals() {
  if (maxPlanningTime < 0) {
    std::cerr << "Invalid maxPlanningTime, must be >= 0" << std::endl;
    valid = false;
  }
  if ((numPlayouts == 0) && (maxPlanningTime <= 0)) {
    std::cerr << "Must stop planning at some point, either specify numPlayouts or maxPlanningTime" << std::endl;
    valid = false;
  }
}

template<class State, class Action>
void MCTS<State,Action>::rollout(const State &startState) {
  MCTS_OUTPUT("------------START ROLLOUT--------------");
  boost::shared_ptr<Model<State,Action> > model = modelUpdater->selectModel(startState);
  State state(startState);
  State newState;
  Action action;
  float reward;
  bool terminal = false;
  valueEstimator->startRollout();

  for (unsigned int depth = 0; (depth < maxDepth) || (maxDepth == 0); depth++) {
    MCTS_OUTPUT("MCTS State: " << state << " " << "DEPTH: " << depth);
    if (terminal || ((maxPlanningTime > 0) && (getTime() > endPlanningTime)))
      break;
    action = valueEstimator->selectPlanningAction(state);
    //std::cout << action << std::endl;
    model->takeAction(action,reward,newState,terminal);
    modelUpdater->updateSimulationAction(action,newState);
    valueEstimator->visit(state,action,reward);
    state = newState;
  }

  valueEstimator->finishRollout(state,terminal);
  MCTS_OUTPUT("------------STOP  ROLLOUT--------------");
}

#endif /* end of include guard: MCTS_MJ647W13 */
