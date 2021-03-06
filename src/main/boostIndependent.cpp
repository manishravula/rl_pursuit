#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <learning/Common.h>
#include <learning/ArffReader.h>
#include <learning/TwoStageTrAdaBoost.h>
#include <factory/ClassifierFactory.h>
#include <factory/AgentFactory.h>
#include <common/Util.h>

std::vector<Feature> getFeatures() {
  std::ifstream in("data/dt/blank.arff");
  ArffReader arff(in);
  in.close();
  return arff.getFeatureTypes();
}

void readAndAddArff(const std::string &filename, Classifier &classifier, bool source, float weight = 1.0, int numSamples = -1) {
  std::cout << "Reading " << filename << " with weight/inst: " << weight << std::endl;
  std::ifstream in(filename.c_str());
  ArffReader arff(in);
  int counter = 0;
  while ((!arff.isDone()) && ((numSamples < 0) || (counter < numSamples))) {
    InstancePtr instance = arff.next();
    instance->weight = weight;
    if (source)
      classifier.addSourceData(instance);
    else
      classifier.addData(instance);
    counter++;
  }
  in.close();
}

std::string getArffName(const std::string &student, const std::string &baseDir) {
  return baseDir + "/train/" + student + ".arff";
}

std::string getDTName(const std::string &student, const std::string &baseDir) {
  return baseDir + "/weighted/only-" + student + ".weka";
}
 
int main(int argc, const char *argv[]) {
  if (argc != 6) {
    std::cerr << "Expected 5 arguments" << std::endl;
    std::cerr << "Usage: boostIndependent targetStudent sourceStudent targetDir sourceDir numTargetInstances" << std::endl;
    return 1;
  }
  std::string targetStudent = argv[1];
  std::string sourceStudent = argv[2];
  std::string targetDir = argv[3];
  std::string sourceDir = argv[4];
  int numTargetInstances = 4 * boost::lexical_cast<int>(argv[5]);

  Json::Value baseLearnerOptions;
  baseLearnerOptions["type"] = "weka";
  baseLearnerOptions["caching"] = false;
  baseLearnerOptions["options"] = "weka.classifiers.trees.REPTree";
  ClassifierPtr (*baseLearner)(const std::vector<Feature>&,const Json::Value&) = &createClassifier;
  std::vector<Feature> features = getFeatures();

  std::cout << "Creating classifier" << std::endl << std::flush;
  TwoStageTrAdaBoost::Params p;
  p.maxBoostingIterations = 10;
  p.numFolds = 2;
  p.savedBestT = -1;
  TwoStageTrAdaBoost classifier(features,false,baseLearner,baseLearnerOptions,p);
  std::cout << "done creating classifier" << std::endl << std::flush;

  readAndAddArff(getArffName(targetStudent,targetDir),classifier,false,1.0,numTargetInstances);
  readAndAddArff(getArffName(sourceStudent,sourceDir),classifier,true);
  classifier.train();
  std::cout << classifier.getBestSourceInstanceWeight() << " " << sourceStudent << std::endl;
  return 0;
}

