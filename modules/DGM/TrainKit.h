// Factory class for constructing node and edge training objects
// Written by Sergey Kosov for Project X in 2018
#pragma once

#include "TrainNode.h"
#include "TrainNodeCvKNN.h"
#include "TrainNodeKNN.h"
#include "TrainNodeNaiveBayes.h"
#include "TrainNodeGM.h"
#include "TrainNodeGMM.h"
#include "TrainNodeCvGMM.h"
#include "TrainNodeCvGM.h"
#include "TrainNodeCvSVM.h"
#include "TrainNodeCvRF.h"
#include "TrainNodeMsRF.h"
#include "TrainNodeCvANN.h"

#include "TrainEdge.h"
#include "TrainEdgePotts.h"
#include "TrainEdgePottsCS.h"
#include "TrainEdgePrior.h"
#include "TrainEdgeConcat.h"
#include "TrainTriplet.h"

#include "TrainLink.h"
#include "TrainLinkNested.h"

#include "macroses.h"

namespace DirectGraphicalModels {
    using general_parameters = std::map<std::string, std::string>;
    
    /**
     *
     */
    namespace TrainKit {
        enum class randomModelNode { Bayes, GM, GMM, CvGM, CvGMM, KNN, CvKNN, CvRF, MsRF, CvANN, CvSVM };
        enum class randomModelEdge { Potts, PottsCS, Prior, Concat };
        enum class randomModelLink { Nested };
        
        /**
         * @brief Creates new node trainer
         */
        std::shared_ptr<CTrainNode> createNodeTrainer(byte nStates, word nFeatures, randomModelNode trainer, const general_parameters &params = general_parameters())
        {
            switch(trainer) {
                case randomModelNode::Bayes:    return std::make_shared<CTrainNodeBayes>(nStates, nFeatures);
                case randomModelNode::GM:       return std::make_shared<CTrainNodeGM>(nStates, nFeatures);
				case randomModelNode::GMM: {
					TrainNodeGMMParams p = TRAIN_NODE_GMM_PARAMS_DEFAULT;
					auto it = params.find("dist_Etreshold");
					if (it != params.end()) p.dist_Etreshold = std::stod(it->second);
					it = params.find("dist_Mtreshold");
					if (it != params.end()) p.dist_Mtreshold = std::stod(it->second);
					it = params.find("div_KLtreshold");
					if (it != params.end()) p.div_KLtreshold = std::stod(it->second);
					it = params.find("maxGausses");
					if (it != params.end()) p.maxGausses = static_cast<word>(std::stoi(it->second));
					it = params.find("minSamples");
					if (it != params.end()) p.minSamples = static_cast<size_t>(std::stoi(it->second));
					return std::make_shared<CTrainNodeGMM>(nStates, nFeatures, p);
				}
                case randomModelNode::CvGM:     return std::make_shared<CTrainNodeCvGM>(nStates, nFeatures);
				case randomModelNode::CvGMM: {
					TrainNodeCvGMMParams p = TRAIN_NODE_CV_GMM_PARAMS_DEFAULT;
					auto it = params.find("covariance_matrix_type");
					if (it != params.end()) p.covariance_matrix_type = std::stoi(it->second);
					it = params.find("epsilon");
					if (it != params.end()) p.epsilon = std::stod(it->second);
					it = params.find("maxCount");
					if (it != params.end()) p.maxCount = std::stoi(it->second);
					it = params.find("maxSamples");
					if (it != params.end()) p.maxSamples = static_cast<size_t>(std::stoi(it->second));
					it = params.find("numGausses");
					if (it != params.end()) p.numGausses = static_cast<word>(std::stoi(it->second));
					it = params.find("term_criteria_type");
					if (it != params.end()) p.term_criteria_type = std::stoi(it->second);
					return std::make_shared<CTrainNodeCvGMM>(nStates, nFeatures, p);
				}
                case randomModelNode::KNN:      return std::make_shared<CTrainNodeKNN>(nStates, nFeatures);
                case randomModelNode::CvKNN:    return std::make_shared<CTrainNodeCvKNN>(nStates, nFeatures);
                case randomModelNode::CvRF:     return std::make_shared<CTrainNodeCvRF>(nStates, nFeatures);
                case randomModelNode::MsRF:
#ifdef USE_SHERWOOD
                    return std::make_shared<CTrainNodeMsRF>(nStates, nFeatures);
#else
                    DGM_WARNING("Microsoft Random Forest model is not supported. Please enable flag USE_SHERWOOD in CMake.");
                    return nullptr;
#endif
                case randomModelNode::CvANN:    return std::make_shared<CTrainNodeCvANN>(nStates, nFeatures);
                case randomModelNode::CvSVM:    return std::make_shared<CTrainNodeCvSVM>(nStates, nFeatures);
                default:
                    DGM_ASSERT_MSG(false, "Required model is not supported");
                    return nullptr;
            }
        }
        
        /**
         * @brief Creates new edge trainer
         */
        std::shared_ptr<CTrainEdge> createEdgeTrainer(byte nStates, word nFeatures, randomModelEdge edgeTrainer, randomModelNode nodeTrainer, const general_parameters &params = general_parameters())
        {
            switch(edgeTrainer) {
                case randomModelEdge::Potts:    return std::make_shared<CTrainEdgePotts>(nStates, nFeatures);
                case randomModelEdge::PottsCS:  return std::make_shared<CTrainEdgePottsCS>(nStates, nFeatures);
                case randomModelEdge::Prior:    return std::make_shared<CTrainEdgePrior>(nStates, nFeatures);
                case randomModelEdge::Concat:
                    switch(nodeTrainer) {
                        case randomModelNode::Bayes:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeBayes, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::GM:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeGM, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::GMM:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeGMM, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::CvGM:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvGM, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::CvGMM:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvGMM, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::KNN:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeKNN, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::CvKNN:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvKNN, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::CvRF:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvRF, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::MsRF:
#ifdef USE_SHERWOOD
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeMsRF, CDiffFeaturesConcatenator>>(nStates, nFeatures);
#else
                            DGM_WARNING("Microsoft Random Forest model is not supported. Please enable flag USE_SHERWOOD in CMake.");
                            return nullptr;
#endif
                        case randomModelNode::CvANN:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvANN, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        case randomModelNode::CvSVM:
                            return std::make_shared<CTrainEdgeConcat<CTrainNodeCvSVM, CDiffFeaturesConcatenator>>(nStates, nFeatures);
                        default:
                            DGM_ASSERT_MSG(false, "Required model is not supported");
                            return nullptr;
                    }
                default:
                    DGM_ASSERT_MSG(false, "Required model is not supported");
                    return nullptr;
            }
        }
        
        /**
         * @brief Creates a new link trainer
         */
        std::shared_ptr<CTrainLink> createLinkTrainer(byte nStatesBase, byte nStatesOccl, word nFeatures, randomModelNode nodeTrainer, const general_parameters &params = general_parameters())
        {
            switch(nodeTrainer) {
                case randomModelNode::Bayes:
                    return std::make_shared<CTrainLinkNested<CTrainNodeBayes>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::GM:
                    return std::make_shared<CTrainLinkNested<CTrainNodeGM>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::GMM:
                    return std::make_shared<CTrainLinkNested<CTrainNodeGMM>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::CvGM:
                    return std::make_shared<CTrainLinkNested<CTrainNodeCvGM>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::CvGMM:
                    return std::make_shared<CTrainLinkNested<CTrainNodeGMM>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::KNN:
                    return std::make_shared<CTrainLinkNested<CTrainNodeKNN>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::CvKNN:
                    return std::make_shared<CTrainLinkNested<CTrainNodeCvKNN>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::CvRF:
                    return std::make_shared<CTrainLinkNested<CTrainNodeCvRF>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::MsRF:
#ifdef USE_SHERWOOD
                    return std::make_shared<CTrainLinkNested<CTrainNodeMsRF>>(nStatesBase, nStatesOccl, nFeatures);
#else
                    DGM_WARNING("Microsoft Random Forest model is not supported. Please enable flag USE_SHERWOOD in CMake.");
                    return nullptr;
#endif
                case randomModelNode::CvANN:
                    return std::make_shared<CTrainLinkNested<CTrainNodeCvANN>>(nStatesBase, nStatesOccl, nFeatures);
                case randomModelNode::CvSVM:
                    return std::make_shared<CTrainLinkNested<CTrainNodeCvSVM>>(nStatesBase, nStatesOccl, nFeatures);
                default:
                    DGM_ASSERT_MSG(false, "Required model is not supported");
                    return nullptr;
            }
        }
    }
}
