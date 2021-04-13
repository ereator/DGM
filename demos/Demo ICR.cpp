#include "DNN.h"
#include "DGM.h"
#include "VIS.h"
#include "DGM/timer.h"

namespace dgm = DirectGraphicalModels;

/**
 * Applies the Sigmoid Activation function
 *
 * @param the value at each node
 * @return a number between 0 and 1.
 */
float applySigmoidFunction(float val)
{
	float sigmoid = 1 / (1 + exp(-val));
	return sigmoid;
}

void backPropagate(std::vector<dgm::dnn::ptr_neuron_t>& vpLayerA,
				   std::vector<dgm::dnn::ptr_neuron_t>& vpLayerB,
				   std::vector<dgm::dnn::ptr_neuron_t>& vpLayerC,
				   std::vector<float>& vResultErrorRate,
				   float learningRate)
{
	Mat DeltaWjk(vpLayerB.size(), vpLayerC.size(), CV_32FC1);
	Mat DeltaWik(vpLayerA.size(), vpLayerB.size(), CV_32FC1);
	std::vector<float> DeltaJ(vpLayerB.size());
	
	for(size_t i = 0; i < vpLayerB.size(); i++) {
		float nodeVal = 0;
		for(size_t j = 0; j < vpLayerC.size(); j++) {
			nodeVal += vpLayerB[i]->getWeight(j) * vResultErrorRate[j];
			DeltaWjk.at<float>(i, j) = learningRate * vResultErrorRate[j]* vpLayerB[i]->getNodeValue();
		}
		float sigmoid = applySigmoidFunction(vpLayerB[i]->getNodeValue());
		DeltaJ[i] = nodeVal * sigmoid * (1-sigmoid);
	}

	for(size_t i = 0; i < vpLayerA.size(); i++) {
		for(size_t j = 0; j < vpLayerB.size(); j++) {
			DeltaWik.at<float>(i ,j) = learningRate * DeltaJ[j] * vpLayerA[i]->getNodeValue();
			float oldWeight = vpLayerA[i]->getWeight(j);
			vpLayerA[i]->setWeight(j, oldWeight + DeltaWik.at<float>(i, j));
		}
	}

	for(size_t i = 0; i < vpLayerB.size(); i++) {
		for(size_t j = 0; j < vpLayerC.size(); j++) {
			float oldWeight = vpLayerB[i]->getWeight(j);
			vpLayerB[i]->setWeight(j, oldWeight + DeltaWjk.at<float>(i ,j));
		}
	}
}

void dotProd(std::vector<dgm::dnn::ptr_neuron_t>& vpLayerA, std::vector<dgm::dnn::ptr_neuron_t>& vpLayerB) {
	for(size_t i = 0 ; i < vpLayerB.size(); i++) {
		float value = 0;
		for(const auto& a : vpLayerA)
			value += a->getWeight(i) * a->getNodeValue();

		value = applySigmoidFunction(value);
		vpLayerB[i]->setNodeValue(value);
	}
}

/**
 * Reads the digits numerical value in a decimal notation
 *
 * @param file to read, and the number of digits to read
 * @return an array of digit labels
 */
std::vector<byte> readGroundTruth(const std::string& fileName)
{
	std::vector<byte> res;
	std::ifstream inFile;
	inFile.open(fileName.c_str());

	if (inFile.is_open()) {
		int val;
		while (!inFile.eof()) {
			inFile >> val;
			res.push_back(static_cast<byte>(val));
		}
		inFile.close();
	}
	return res;
}

/**
 * Reads the image pixel value
 *
 * @param image to read, and the number of images to read
 * @return an array of pixel values for each image
 */
Mat readImgData(const std::string& fileName, size_t dataSize, size_t numNeurons)
{
	Mat res(dataSize, numNeurons, CV_32SC1);
		
	for(int m = 0; m < res.rows; m++)
	{
		std::stringstream ss;
		ss << std::setfill('0') << std::setw(4);
		ss << m;
		std::string number = ss.str();
		std::string path = fileName + number + ".png";
		std::string image_path = samples::findFile(path);

		Mat img = imread(image_path, 0);
		
		int l = 0;
		for(int i = 0; i < img.rows; i++) {
			for(int j = 0; j < img.cols; j++) {
				int value = abs((int)img.at<uchar>(i,j) - 255);
				res.at<int>(m, l) = value;
				l++;
			}
		}
	}
	return res;
}


int main()
{
	const word 		nStates					= 10; 	// 10 digits
	const size_t    numNeuronsInputLayer   	= 784;
    const size_t    numNeuronsHiddenLayer	= 60;
    const size_t    numNeuronsOutputLayer  	= 10;
    const size_t	numTrainSamples  		= 4000;
	const size_t 	numTestSamples    		= 2000;

	
	Mat		trainDataBin   	= readImgData("../../../data/digits/train/digit_", numTrainSamples, numNeuronsInputLayer);
	auto	trainDataDigit 	= readGroundTruth("../../../data/digits/train_gt.txt");
	assert(trainDataDigit.size() == numTrainSamples);
	
	Mat 	testDataBin   	= readImgData("../../../data/digits/test/digit_", numTestSamples, numNeuronsInputLayer );
	auto 	testDataDigit  	= readGroundTruth("../../../data/digits/test_gt.txt");
	assert(testDataDigit.size() == numTestSamples);


    std::vector<dgm::dnn::ptr_neuron_t> vpInputLayer;
    std::vector<dgm::dnn::ptr_neuron_t> vpHiddenLayer;
    std::vector<dgm::dnn::ptr_neuron_t> vpOutputLayer;

    for (size_t i = 0; i < numNeuronsInputLayer; i++)
        vpInputLayer.push_back( std::make_shared<dgm::dnn::CNeuron>(numNeuronsHiddenLayer, 0) );

    for (size_t i = 0; i < numNeuronsHiddenLayer; i++)
        vpHiddenLayer.push_back( std::make_shared<dgm::dnn::CNeuron>(numNeuronsOutputLayer) );

    for (size_t i = 0; i < numNeuronsOutputLayer; i++)
        vpOutputLayer.push_back( std::make_shared<dgm::dnn::CNeuron>(0) );


    for (size_t i = 0; i < vpHiddenLayer.size(); i++)
        vpHiddenLayer[i]->generateRandomWeights();

    for(size_t i = 0; i < vpInputLayer.size(); i++)
        vpInputLayer[i]->generateRandomWeights();

	// ==================== TRAINING DIGITS ====================
	dgm::Timer::start("Training...");
	for(int s = 0; s < numTrainSamples; s++) {
        
		for(size_t i = 0; i < vpInputLayer.size(); i++) {
            float val = static_cast<float>(trainDataBin.at<int>(s ,i)) / 255;
            vpInputLayer[i]->setNodeValue(val);
        }

        dotProd(vpInputLayer, vpHiddenLayer);
        dotProd(vpHiddenLayer, vpOutputLayer);
    
        std::vector<float> vResultErrorRate(numNeuronsOutputLayer);
		for(size_t i = 0; i < vResultErrorRate.size(); i++) {
			vResultErrorRate[i] = (trainDataDigit[s] == i) ? 1 : 0;
			vResultErrorRate[i] -= vpOutputLayer[i]->getNodeValue();
		}

        backPropagate(vpInputLayer, vpHiddenLayer, vpOutputLayer, vResultErrorRate, 0.1f);
    } // samples
	dgm::Timer::stop();

	// ==================== TESTING DIGITS ====================
	dgm::CCMat confMat(nStates);
	dgm::vis::CMarker marker;
	dgm::Timer::start("Testing...");
	for(size_t s = 0; s < numTestSamples; s++) {
		 for(size_t i = 0; i < vpInputLayer.size(); i++) {
			 float val = static_cast<float>(testDataBin.at<int>(s, i)) / 255;
			 vpInputLayer[i]->setNodeValue(val);
		 }

		 dotProd(vpInputLayer, vpHiddenLayer);
		 dotProd(vpHiddenLayer, vpOutputLayer);

		 double *allPredictionsforDigits = new double[numNeuronsOutputLayer];
		 for(size_t i=0 ; i < vpOutputLayer.size(); i++) {
			 allPredictionsforDigits[i] = vpOutputLayer[i]->getNodeValue();
		 }

		 float maxAccuracy = 0;
		 byte   number;
		 for(size_t i=0 ; i < vpOutputLayer.size(); i++) {
			 if(allPredictionsforDigits[i] >= maxAccuracy) {
				 maxAccuracy = allPredictionsforDigits[i];
				 number = static_cast<byte>(i);
			 }
		 }
		confMat.estimate(number, testDataDigit[s]);
        //printf("prediction [%d] for digit %d with %.3f%s at position %zu \n", number, testDataDigit[z], maxAccuracy, "%", z);
	} // samples
	dgm::Timer::stop();
	printf("Accuracy = %.2f%%\n", confMat.getAccuracy());
	
	// Confusion matrix
	Mat cMat    = confMat.getConfusionMatrix();
	Mat cMatImg = marker.drawConfusionMatrix(cMat, dgm::vis::MARK_BW);
	imshow("Confusion Matrix", cMatImg);
	
	waitKey();
	
	
	return 0;
}


