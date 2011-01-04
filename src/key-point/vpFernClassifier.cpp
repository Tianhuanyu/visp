/****************************************************************************
 *
 * $Id$
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2010 by INRIA. All rights reserved.
 * 
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.GPL at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional 
 * Edition License.
 *
 * See http://www.irisa.fr/lagadic/visp/visp.html for more information.
 * 
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://www.irisa.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 * 
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *
 * Description:
 * Class that implements the Fern classifier and the YAPE detector thanks
 * to the OpenCV library.
 *
 * Authors:
 * Romain Tallonneau
 *
 *****************************************************************************/

#include <visp/vpConfig.h>

#if (VISP_HAVE_OPENCV_VERSION >= 0x020000) // Require opencv >= 2.0.0

#include <visp/vpFernClassifier.h>
#include <visp/vpImageTools.h>
#include <visp/vpImageConvert.h>
#include <visp/vpColor.h>
#include <visp/vpDisplay.h>

/*!
  Basic constructor 
  
*/
vpFernClassifier::vpFernClassifier():vpBasicKeyPoint(), gen(0, 256, 5, true, 0.6, 1.5, -CV_PI/2, CV_PI/2, -CV_PI/2, CV_PI/2)
{
  init();
}

/*!
  Constructor using the provided data file to load the object given by its name.
  
  If the name of the object does not correspond in the file, a 
  vpException::ioError is thrown.
  
  \param _dataFile : the name of the file saved after a training (with the 
    record methods).
  \param _objectName : the name of the object to load
*/
vpFernClassifier::vpFernClassifier(const std::string& _dataFile, const std::string& _objectName)
{
  init();
  this->load(_dataFile, _objectName);
}

/*!
  Basic destructor
  
*/
vpFernClassifier::~vpFernClassifier()
{
  if(curIplImg != NULL){
    curIplImg->imageData = NULL;
    cvReleaseImageHeader(&curIplImg);
    curIplImg = NULL;
  }  
}

/*!
  initialise any OpenCV parameters
  
  The classifier need to be trained again or to be loaded from an external file.
*/
void 
vpFernClassifier::init()
{
  hasLearn = false; 
  nbClassfier = 100;
  ClassifierSize = 11;
  nbPoints = 200;
  curIplImg = NULL;
  blurImage = true;
  radiusBlur = 7;
  sigmaBlur = 1;
  patchSize = 32;
  radius = 7;
  threshold = 20;
  nbOctave = 2;
  nbView = 2000;
  dist = 2;  
  nbMinPoint = 10;
}




/*! 
  Train the classifier.
*/
void
vpFernClassifier::train()
{
    // initialise detector
  cv::LDetector d(radius, threshold, nbOctave, nbView, patchSize, dist);
  
    //blur 
  cv::Mat obj = (cv::Mat)curIplImg;
  
  if(this->getBlurSetting()){
    cv::GaussianBlur(obj, obj, cv::Size(getBlurSize(), getBlurSize()), getBlurSigma(), getBlurSigma());
  }
  
    // build pyramid 
  std::vector<cv::Mat> objpyr;
  cv::buildPyramid(obj, objpyr, d.nOctaves-1);
  
    // patch generator
//  std::vector<cv::KeyPoint> imgKeypoints;
  
    // getPoints
  d.getMostStable2D(obj, objKeypoints, 100, gen);
  
  ldetector = d;
  
    // train classifier
  modelROI = cv::Rect(0, 0, objpyr[0].cols, objpyr[0].rows);
  ldetector.getMostStable2D(objpyr[0], modelPoints, 100, gen);
  
  fernClassifier.trainFromSingleView(objpyr[0], modelPoints,
                                     patchSize, (int)modelPoints.size(), 100, 11, 10000,
                                     cv::FernClassifier::COMPRESSION_NONE, gen);

  /* from OpenCV format to ViSP format */
  referenceImagePointsList.resize(0);
  for (unsigned int i = 0; i < modelPoints.size(); i += 1){
    vpImagePoint ip( 
        modelPoints[i].pt.y + modelROI_Ref.y, 
        modelPoints[i].pt.x + modelROI_Ref.x);
    referenceImagePointsList.push_back(ip);
  }
 
    // set flag
  hasLearn = true;
}

/*!

  Build the list of reference points. The computation of the points is
  made all over the image I. It also includes the training of the fern 
  classifier.
  
  \warning thie method can take up to several minutes depending on the 
  parameters of the classifier and on the size of the image.

  \param _I : The gray scaled image where the refrence points are computed.

  \return the number of reference points.
*/
unsigned int 
vpFernClassifier::buildReference(const vpImage<unsigned char> &_I)
{
  this->setImage(_I);
  
  train();

  _reference_computed = true;
  return objKeypoints.size();

}

/*!

  Build the list of reference points. The computation of the points is
  made only on a part of the image. This part is a rectangle defined
  by its top left corner, its height and its width. The parameters of
  this rectangle must be given in pixel. It also includes the training of the 
  fern classifier.
  
  \warning the method can take up to several minutes depending on the 
  parameters of the classifier and on the size of the image.

  \param _I : The gray scaled image where the refrence points are computed.
  \param _iP : The top left corner of the rectangle.
  \param _height : height of the rectangle (in pixel).
  \param _width : width of the rectangle (in pixel).

  \return the number of reference points.
*/
unsigned int 
vpFernClassifier::buildReference(const vpImage<unsigned char> &_I, 
	      vpImagePoint &_iP, 
	      const unsigned int _height, const unsigned int _width)
{
  if((_iP.get_i()+_height) >= _I.getHeight()
     || (_iP.get_j()+_width) >= _I.getWidth()) {
    vpTRACE("Bad size for the subimage");
    throw(vpException(vpImageException::notInTheImage ,
		      "Bad size for the subimage"));
  }
    
  vpImage<unsigned char> subImage;
  vpImageTools::createSubImage(_I,
			       (unsigned int)_iP.get_i(),
			       (unsigned int)_iP.get_j(),
			       _height, _width, subImage);
  this->setImage(subImage);
  
    /* initialise a structure containing the region of interest used in the 
    reference image */
  modelROI_Ref.x = (int)_iP.get_u();
  modelROI_Ref.y = (int)_iP.get_v();
  modelROI_Ref.width = (int)_width;
  modelROI_Ref.height = (int)_height;  
  
  train();

  return objKeypoints.size(); 
}

/*!

  Build the list of reference points. The computation of the points is
  made only on a part of the image. This part is a rectangle. The
  parameters of this rectangle must be given in pixel. It also includes the 
  training of the fern classifier.
  
  \warning the method can take up to several minutes depending on the 
  parameters of the classifier and on the size of the image.

  \param _I : The gray scaled image where the refrence points are computed.
  \param _rectangle : The rectangle which defines the interesting part
  of the image.

  \return The number of reference points.
*/
unsigned int 
vpFernClassifier::buildReference(const vpImage<unsigned char> &_I, 
	      const vpRect& _rectangle)
{
  vpImagePoint iP;
  iP.set_i(_rectangle.getTop());
  iP.set_j(_rectangle.getLeft());
  return (this->buildReference(_I, iP,
			       (unsigned int)_rectangle.getHeight(),
			       (unsigned int)_rectangle.getWidth()));
}

/*!
  Compute the points of interest in the current image and try to match them 
  using the trained classifier. The matched pairs can be found with the 
  getMatchedPointByRef function. 
  
  \exception vpException::notInitialised if the classifier has not been trained.
  
  \param _I : The gray scaled image where the points are computed.

  \return The number of point which have been matched.
*/
unsigned int 
vpFernClassifier::matchPoint(const vpImage<unsigned char> &_I)
{
  if(!hasLearn){
    vpERROR_TRACE("The object has not been learned. ");
    throw vpException(vpException::notInitialized , "object is not learned, load database or build the reference ");
  }

  setImage(_I);
  // resize image  
//  cv::resize(_image, image, Size(), 1./imgscale, 1./imgscale, INTER_CUBIC);
  cv::Mat img = this->curIplImg;
  
  if(this->getBlurSetting()){
    cv::GaussianBlur(img, img, cv::Size(this->getBlurSize(), this->getBlurSize()), this->getBlurSigma(), this->getBlurSigma());
  }
  
  std::vector<cv::Mat> imgPyr;
  cv::buildPyramid(img, imgPyr, ldetector.nOctaves-1);
  
    //matching
  cv::Mat H;  
  
  ldetector(imgPyr, imgKeypoints, 500);

  unsigned int m = modelPoints.size();
  unsigned int n = imgKeypoints.size();
  std::vector<int> bestMatches(m, -1);
  std::vector<float> maxLogProb(m, -FLT_MAX);
  std::vector<float> signature;
  unsigned int totalMatch = 0;

  
  /* part of code from OpenCV planarObjectDetector */
  currentImagePointsList.resize(0);
  matchedReferencePoints.resize(0);

  for(unsigned int i = 0; i < n; i++ ){
    cv::KeyPoint kpt = imgKeypoints[i];
    kpt.pt.x /= (float)(1 << kpt.octave);
    kpt.pt.y /= (float)(1 << kpt.octave);
    int k = fernClassifier(imgPyr[(unsigned int)kpt.octave], kpt.pt, signature);
    if( k >= 0 && (bestMatches[(unsigned int)k] < 0 || signature[(unsigned int)k] > maxLogProb[(unsigned int)k]) ){
      maxLogProb[(unsigned int)k] = signature[(unsigned int)k];
      bestMatches[(unsigned int)k] = (int)i;
      totalMatch++;
      
      vpImagePoint ip_cur( imgKeypoints[i].pt.y, imgKeypoints[i].pt.x);
    
      currentImagePointsList.push_back(ip_cur);
      matchedReferencePoints.push_back((unsigned int)k);
      
    }
  }

  refPt.resize(0);
  curPt.resize(0);
  for(unsigned int i = 0; i < m; i++ ){
    if( bestMatches[i] >= 0 ){
      refPt.push_back(modelPoints[i].pt);
      curPt.push_back(imgKeypoints[(unsigned int)bestMatches[i]].pt);
    }
  }

  return totalMatch;
}



/*!
  Compute the points of interest in the specified region of the current image 
  and try to recognise them using the trained classifier. The matched pairs can 
  be found with the getMatchedPointByRef function. The homography between the 
  two planar surfaces is also computed.
  
  \param _I : The gray scaled image where the points are computed.
  \param _iP : the top left corner of the rectangle defining the ROI
  \param _height : the height of the ROI
  \param _width : the width of the ROI

  \return the number of point which have been matched.
*/
unsigned int 
vpFernClassifier::matchPoint(const vpImage<unsigned char> &_I, 
	  vpImagePoint &_iP, 
	  const unsigned int _height, const unsigned int _width)
{
  if((_iP.get_i()+_height) >= _I.getHeight()
     || (_iP.get_j()+_width) >= _I.getWidth()) {
    vpTRACE("Bad size for the subimage");
    throw(vpException(vpImageException::notInTheImage ,
		      "Bad size for the subimage"));
  }

  vpImage<unsigned char> subImage;

  vpImageTools::createSubImage(_I,
			       (unsigned int)_iP.get_i(),
			       (unsigned int)_iP.get_j(),
			       _height, _width, subImage);

  return this->matchPoint(subImage);
}

/*!
  Compute the points of interest in the specified region of the current image 
  and try to recognise them using the trained classifier. The matched pairs can 
  be found with the getMatchedPointByRef function. The homography between the 
  two planar surfaces is also computed.
  
  \param _I : The gray scaled image where the points are computed.
  \param _rectangle : the rectangle defining the ROI

  \return the number of point which have been matched.
*/
unsigned int 
vpFernClassifier::matchPoint(const vpImage<unsigned char> &_I, 
	  const vpRect& _rectangle)
{
  vpImagePoint iP;
  iP.set_i(_rectangle.getTop());
  iP.set_j(_rectangle.getLeft());
  return (this->matchPoint(_I, iP,
			   (unsigned int)_rectangle.getHeight(),
			   (unsigned int)_rectangle.getWidth()));
}


/*!
  This function displays the matched reference points and the matched
  points computed in the current image. The reference points are
  displayed in the image Ireference and the matched points coming from
  the current image are displayed in the image Icurrent. It is
  possible to set Ireference and Icurrent with the same image when
  calling the method.

  \param _Iref : The image where the matched refrence points are
  displayed.

  \param _Icurrent : The image where the matched points computed in the
  current image are displayed.
*/
void 
vpFernClassifier::display(const vpImage<unsigned char> &_Iref, 
	const vpImage<unsigned char> &_Icurrent)
{
  for (unsigned int i = 0; i < matchedReferencePoints.size(); i++){
      vpDisplay::displayCross (_Iref, referenceImagePointsList[matchedReferencePoints[i]], 3, vpColor::red);
      vpDisplay::displayCross (_Icurrent, currentImagePointsList[i], 3, vpColor::green);
  }

}


/*!
  This function displays only the matched points computed in the
  current image. They are displayed in the image Icurrent.
  
  \param _Icurrent : the gray scaled image for the display
*/
void 
vpFernClassifier::display(const vpImage<unsigned char> &_Icurrent)
{    
  for (unsigned int i = 0; i < matchedReferencePoints.size(); i++){
      vpDisplay::displayCross (_Icurrent, currentImagePointsList[i], 3, vpColor::green);
  }
}

/*             IO METHODS            */

/*!
  \brief load the Fern classifier
  
  Load and initialize the Fern classifier and load the 3D and 2D keypoints. It 
  can take up to sevral seconds.
  
  \param _dataFile : The name of the data filename (very large text file). It can have any file extension.
*/
void 
vpFernClassifier::load(const std::string& _dataFile, const std::string& /*_objectName*/)
{
  std::cout << " > Load data for the planar object detector..." << std::endl;
  
  /* part of code from OpenCV planarObjectDetector */
  cv::FileStorage fs(_dataFile, cv::FileStorage::READ);
  cv::FileNode node = fs.getFirstTopLevelNode();
    
  cv::FileNodeIterator it = node["model-roi"].begin(), it_end;
  it >> modelROI.x >> modelROI.y >> modelROI.width >> modelROI.height;
  
  ldetector.read(node["detector"]);
  fernClassifier.read(node["fern-classifier"]);
  cv::read(node["model-points"], modelPoints);
  
  cv::LDetector d(radius, threshold, nbOctave, nbView, patchSize, dist);
  ldetector = d;
  hasLearn = true;
}


/*!
  \brief record the Ferns classifier in the text file

  \param _objectName : The name of the object to store in the data file
  \param _dataFile : The name of the data filename (very large text file). It can have any file extension.
*/
void
vpFernClassifier::record(const std::string& _objectName, const std::string& _dataFile )
{
  /* part of code from OpenCV planarObjectDetector */
  cv::FileStorage fs(_dataFile, cv::FileStorage::WRITE);
  
  cv::WriteStructContext ws(fs, _objectName, CV_NODE_MAP);
  
  {
    cv::WriteStructContext wsroi(fs, "model-roi", CV_NODE_SEQ + CV_NODE_FLOW);
    cv::write(fs, modelROI_Ref.x);
    cv::write(fs, modelROI_Ref.y);
    cv::write(fs, modelROI_Ref.width);
    cv::write(fs, modelROI_Ref.height);
  }
  
  ldetector.write(fs, "detector");
  cv::write(fs, "model-points", modelPoints);
  fernClassifier.write(fs, "fern-classifier");
}



/*!
  Set the current image. This method allows to convert a image from the visp 
  format to the OpenCV one. 
  
  \param _I : the input image.
*/
void 
vpFernClassifier::setImage(const vpImage<unsigned char>& _I)
{  
  if(curIplImg != NULL){
    cvResetImageROI(curIplImg);
    cvReleaseImage(&curIplImg);
    curIplImg = NULL;
  }
  
  vpImageConvert::convert(_I, curIplImg);
  if(curIplImg == NULL){
    std::cout << "!> conversion failed" << std::endl;
    throw vpException(vpException::notInitialized , "conversion failed");
  }
}

#endif

