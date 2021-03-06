from __future__ import print_function
import nifty
import numpy

import numpy
import vigra
import glob
import os
from functools import partial


nrag = nifty.graph.rag
ngala = nifty.graph.gala
ngraph = nifty.graph
G = nifty.graph.UndirectedGraph

def make_dataset(numberOfImages = 10, noise=1.0,shape=(100,100)):

    numpy.random.seed(42)
    imgs = []
    gts = []


    for i in range(numberOfImages):

        gtImg = numpy.zeros(shape)
        gtImg[0:shape[0]/2,:] = 1

        gtImg[shape[0]/4: 3*shape[0]/4, shape[0]/4: 3*shape[0]/4]  = 2

        ra = numpy.random.randint(180)
        #print ra 

        gtImg = vigra.sampling.rotateImageDegree(gtImg.astype(numpy.float32),int(ra),splineOrder=0)
        grad = vigra.filters.gaussianGradientMagnitude(gtImg.astype('float32'),1.0)


        for x in range(6):
            ra2 = numpy.random.randint(low=0,high=2,size=gtImg.size)
            ra2 = ra2.reshape(gtImg.shape)
            #print ra 
            grad*=ra2

        grad = grad.squeeze()

        img = numpy.random.random(shape)*float(0.00001)
        grad+=img
        #if i<1 :
        #    vigra.imshow(img)
        #    vigra.show()

        imgs.append(grad.astype('float32'))
        gts.append(gtImg)


    return imgs,gts

def makeRag(raw, showSeg = False):
    #raw = vigra.gaussianSmoothing(raw,1.0)
    ew = vigra.filters.hessianOfGaussianEigenvalues(-1.0*raw, 2.3)[:,:,0]
    seg, nseg = vigra.analysis.watershedsNew(ew)

    #seg, nseg = vigra.analysis.slicSuperpixels(raw,intensityScaling=4.5, seedDistance=20)

    seg = seg.squeeze()
    if showSeg:
        vigra.segShow(raw, seg)
        vigra.show()

    # get the rag
    seg -= 1
    assert seg.min() == 0
    assert seg.max() == nseg -1 
    return nifty.graph.rag.gridRag(seg)

def makeFeatureOpFromChannel(rag, data, minVal=None, maxVal=None):

    #  feature accumulators
    if minVal is None:
        minVal = float(data.min())

    if maxVal is None:
        maxVal = float(data.max())

    edgeFeatures = nifty.graph.rag.defaultAccEdgeMap(rag, minVal, maxVal)
    nodeFeatures = nifty.graph.rag.defaultAccNodeMap(rag, minVal, maxVal)

    # accumulate features
    nrag.gridRagAccumulateFeatures(graph=rag,data=data,
        edgeMap=edgeFeatures, nodeMap=nodeFeatures)


    fOp = ngala.galaDefaultAccFeature(graph=rag, edgeFeatures=edgeFeatures, nodeFeatures=nodeFeatures)
    
    return fOp,minVal, maxVal

def makeFeatureOp(rag, raw, minVals=None, maxVals=None):


    filterFuc = [
        partial(vigra.filters.gaussianSmoothing,sigma=0.5),
        partial(vigra.filters.gaussianSmoothing,sigma=1.0),
        partial(vigra.filters.gaussianSmoothing,sigma=2.0),
        partial(vigra.filters.gaussianSmoothing,sigma=4.0),
        partial(vigra.filters.gaussianGradientMagnitude,sigma=1.0),
        partial(vigra.filters.gaussianGradientMagnitude,sigma=2.0),
        partial(vigra.filters.gaussianGradientMagnitude,sigma=4.0),
        partial(vigra.filters.hessianOfGaussianEigenvalues,scale=1.0),
        partial(vigra.filters.hessianOfGaussianEigenvalues,scale=2.0),
        partial(vigra.filters.hessianOfGaussianEigenvalues,scale=4.0),
        partial(vigra.filters.structureTensorEigenvalues,innerScale=1.0,outerScale=2.0),
        partial(vigra.filters.structureTensorEigenvalues,innerScale=2.0,outerScale=4.0),
        partial(vigra.filters.structureTensorEigenvalues,innerScale=4.0,outerScale=8.0)
    ]
    fCollection = ngala.galaFeatureCollection(rag)

    minVals_ = []
    maxVals_ = []
    c = 0
    for f in filterFuc:
        res = f(raw).squeeze()
        if res.ndim == 2:
            res = res[:, :, None]
        for c in range(res.shape[2]):
            resC = res[:, :, c]

            if minVals is not None:
                minv = minVals[c]
            else:
                minv = resC.min()
            if maxVals is not None:
                maxv = maxVals[c]
            else:
                maxv = resC.max()
            minVals_.append(minv)
            maxVals_.append(maxv)
            op, _minVal , _maxVal =  makeFeatureOpFromChannel(rag, resC, minVal=minv, maxVal=maxv)
            fCollection.addFeatures(op)
            c +=1

    return fCollection, minVals_, maxVals_

def makeEdgeGt(rag, gt):
    # get the gt
    nodeGt = nrag.gridRagAccumulateLabels(rag, gt)
    uvIds = rag.uvIds()
    edgeGt = (nodeGt[uvIds[:,0]] != nodeGt[uvIds[:,1]]).astype('double')
    return edgeGt

def test_mcgala():



    # get the dataset
    imgs,gts = make_dataset(10, noise=4.5, shape=(200,200))

    Obj = G.MulticutObjective
    CG = G.EdgeContractionGraph
    CGObj = CG.MulticutObjective


    greedyFactory = Obj.greedyAdditiveFactory()
    ilpFactory = Obj.multicutIlpFactory(ilpSolver='cplex',
        addThreeCyclesConstraints=False,
        addOnlyViolatedThreeCyclesConstraints=False
        #memLimit= 0.01
    )
    fmFactoryA = CGObj.fusionMoveBasedFactory(
        #fusionMove=CGObj.fusionMoveSettings(mcFactory=greedyFactory),
        fusionMove=CGObj.fusionMoveSettings(mcFactory=ilpFactory),
        #proposalGen=nifty.greedyAdditiveProposals(sigma=30,nodeNumStopCond=-1,weightStopCond=0.0),
        proposalGen=CGObj.watershedProposals(sigma=1,seedFraction=0.1),
        numberOfIterations=10,
        numberOfParallelProposals=40, # no effect if nThreads equals 0 or 1
        numberOfThreads=40,
        stopIfNoImprovement=2,
        fuseN=2,
    )




    ragTrain  = makeRag(imgs[0], showSeg= True)
    fOpTrain, minVal, maxVal = makeFeatureOp(ragTrain, imgs[0])
    edgeGt = makeEdgeGt(ragTrain, gts[0])


    cOrderSettigns = G.galaContractionOrderSettings(
        mcMapFactory=fmFactoryA,
        runMcMapEachNthTime=10)

    # gala class
    settings = G.galaSettings(threshold0=0.1, threshold1=0.9, thresholdU=0.1,
                              numberOfEpochs=1, numberOfTrees=200,
                              contractionOrderSettings=cOrderSettigns
                              #mapFactory=fmFactoryA,
                              #perturbAndMapFactory=fmFactoryB
                              )
    gala = G.gala(settings)



    trainingInstance  = ngala.galaTrainingInstance(ragTrain, fOpTrain, edgeGt)
    gala.addTrainingInstance(trainingInstance)
    gala.train()


    for x in range(3,10):

        ragTest  = makeRag(imgs[x], showSeg=False)
        fOpTest, minVal, maxVal = makeFeatureOp(ragTest, imgs[x], minVal, maxVal)
        instance  = ngala.galaInstance(ragTest, fOpTest)
        edgeGt = makeEdgeGt(ragTest, gts[x])

        nodeRes = gala.predict(instance)

        pixelNodeRes = nrag.projectScalarNodeDataToPixels(ragTest,nodeRes,-1)
        vigra.segShow(imgs[x], pixelNodeRes)
        vigra.show()


        #for edge, uv in enumerate(ragTest.uvIds()):
        #    print(edge,edgeGt[edge],nodeRes[uv[0]]!=nodeRes[uv[1]])



test_mcgala()
