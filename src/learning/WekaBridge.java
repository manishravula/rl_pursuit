import java.io.*;
import java.util.*;
import weka.core.*;
import weka.classifiers.Classifier;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.ByteOrder;

public class WekaBridge {
	static Classifier classifier;

  static private native int init(String memSegName, int numFeatures, int numClasses);
  static private native byte readCommand(double[][] features,double[] weight, int[] n);
  static private native void writeDistr(double[] distr);
  static private native void send();
  static private native String readMsg();


  static {
    System.loadLibrary("WekaBridge");
  }

  public static void main(String [] argv) {

    try {
      String memSegName = argv[0];
      String dataName = argv[1];
      String classifierName = argv[2];
      String[] opts = new String[argv.length-3];
      for (int i = 0; i < opts.length; i++) {
        opts[i] = argv[i+3];
      }

      weka.core.WekaPackageManager.loadPackages(false, false);
      // get the data groups set up
      Instances trainData = new Instances(new BufferedReader(new FileReader(dataName)));
      trainData.setClassIndex(trainData.numAttributes()-1);
      Instances testData = new Instances(trainData,0);
      // initialize the memory for communication
      byte commandByte = '\0';
      Instance inst;
      int numInstances = init(memSegName,trainData.numAttributes(),trainData.numClasses());
      // create the classifier
      classifier = (Classifier)Utils.forName(Classifier.class,classifierName,opts);
      //classifier.buildClassifier(trainData);
      // wait for commands
      double[][] features = new double[numInstances][trainData.numAttributes()];
      double[] weight = new double[numInstances];
      double[] distr;
      int[] n = new int[1];
      int weightInd = 0;
      while (true) {
        commandByte = readCommand(features,weight,n);
        if (commandByte == 'e') {
          //System.out.println("Asked to exit");
          break;
        }
        switch (commandByte) {
          case 't':
            if (weightInd != 0) {
              if (weightInd != trainData.size()) {
                System.out.println("BAD WEIGHT SIZE: " + weightInd + " expected " + trainData.size());
              }
              weightInd = 0;
            }
            //System.out.format("TRAINING %d%n",trainData.numInstances());
            //for (int i = 0; i < trainData.numInstances(); i++) {
              //System.out.println(trainData.instance(i).toString());
            //}
            classifier.buildClassifier(trainData);
            break;
          case 'x':
            trainData.delete();
            break;
          case 'c':
            assert(n[0] == 1): "Only classify 1 instance at once";
            inst = new DenseInstance(weight[0],features[0]);
            testData.add(inst);
            distr = classifier.distributionForInstance(testData.lastInstance());
            writeDistr(distr);
            testData.delete();
            break;
          case 'a':
            for (int i = 0; i < n[0]; i++) {
              inst = new DenseInstance(weight[i],features[i]);
              trainData.add(inst);
              //System.out.println("New inst:" + inst);
            }
            break;
          case 'p':
            System.out.println(classifier.toString());
            break;
          case 'w': {
            String outFilename = readMsg();
            FileWriter out = new FileWriter(outFilename);
            out.write(classifier.toString());
            out.close();
            break;
          }
          case 's':
            String outFilename = readMsg();
            System.out.println("Saving weka classifier from " + outFilename);
            FileOutputStream fos = new FileOutputStream(outFilename);
            ObjectOutputStream out = new ObjectOutputStream(fos);
            out.writeObject(classifier);
            out.close();
            break;
          case 'l':
            String inFilename = readMsg();
            System.out.println("Loading weka classifier from " + inFilename);
            FileInputStream fis = new FileInputStream(inFilename);
            ObjectInputStream in = new ObjectInputStream(fis);
            classifier = (Classifier)in.readObject();
            in.close();
            break;
          case 'r':
            for (int i = 0; i < n[0]; i++) {
              trainData.get(weightInd).setWeight(weight[i]);
              weightInd++;
            }
            break;
        }
        send();
      }

/*
      // load packages
      // deal with command line options

      //BufferedReader in = new Buffnew DataInputStream(new FileInputStream(toWekaName));
      BufferedReader in = new BufferedReader(new FileReader(toWekaName));
          //= new BufferedReader(new InputStreamReader(in));
      PrintStream out = new PrintStream(new FileOutputStream(fromWekaName));
      
      // build an initial classifier
			classifier.buildClassifier(trainData);

      while (true) {
        line = in.readLine();
        if ((line == null) || line.equals("exit")) {
          break;
        } else if (line.equals("add")) {
          Instance inst = readInst(in,trainData.numAttributes());
          trainData.add(inst);
        } else if (line.equals("train")) {
          classifier.buildClassifier(trainData);
        } else if (line.equals("classify")) {
          Instance inst = readInst(in,trainData.numAttributes());
          testData.add(inst);
          double[] distr = classifier.distributionForInstance(testData.lastInstance());
          testData.delete();
          for (int i = 0; i < distr.length; i++) {
            out.format("%f ",distr[i]);
          }
          out.format("%n");
        } else if (line.equals("")) {
          continue;
        } else if (line.equals("save")) {
          line = in.readLine();
          System.out.println("SAVING " + line);
          saveModel(line);
        } else {
          System.err.println("ERROR: unexpected line:");
          System.err.println(line);
          break;
        }
      }

      
      in.close();
      out.close();
      return;
*/
    } catch (Exception e) {
      System.err.println("EXCEPTION:");
      System.err.println(e.toString());
      e.printStackTrace();
    }
  }
  public static Instance readInst(BufferedReader in, int numFeatures) throws java.io.IOException {
    double weight;
    double[] vals = new double[numFeatures];
    String line = in.readLine();
    String[] strVals = line.split(",");
    assert(strVals.length == numFeatures + 1);

    for (int i = 0; i < numFeatures; i++) {
      vals[i] = Double.parseDouble(strVals[i]);
    }
    weight = Double.parseDouble(strVals[numFeatures]);
    return new DenseInstance(weight,vals);
  }

	public static void saveModel(String filename) {
		try {
			weka.core.SerializationHelper.write(filename,classifier);
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
	}
}

