/*
 * iSearch.cpp
 *
 *  Created on: Oct 12, 2015
 *      Author: fcaldas
 */

#include "imSearch.h"
#include <dirent.h>
#include <thread>
#include <opencv2/imgproc/imgproc.hpp>
#include <unistd.h>
#include <omp.h>

using namespace imageSearchObjects;
using namespace std;

imSearch::imSearch() {
	DIR * dir = opendir(this->path.c_str());
	cout << "Opening " << this->path << " and processing its contents"<<endl;
	assert(("Can't open pattern directory!", NULL != dir));

    dirent * entity = readdir(dir);
	while(entity != NULL)
	{
	    string fname = entity->d_name;
	    if(fname.size() > 4 && fname.substr(fname.size()-4,4) == ".png"){
	    	cout << " -  adding "<<this->path + "/"+fname<<" to file list"<<endl;
	    	cv::Mat imgbw = cv::imread(this->path +"/"+ fname , cv::IMREAD_GRAYSCALE);
	    	cout<<"Reading done"<<endl;
	    	//cv::imshow("test", imgbw);
	    	//cv::waitKey(0);
	    	if(imgbw.data == NULL){
	    		cout << " -  INVALID IMAGE "<<fname<<endl;
	    	}else{
	    		__imData dImg;
	    		dImg.filename = fname;
	    		dImg.image = imgbw;
	    		this->images.push_back(dImg);
	    	}
	    }
	    entity = readdir(dir);
	    cout <<entity<<endl;
	}
	FilterTemplates();
	closedir(dir);
}

imSearch::~imSearch() {
	// TODO Auto-generated destructor stub

}

bool imSearch::hasFinished(){
	return this->finished;
}

bool imSearch::doSearch(cv::Mat &img){
	if(this->lockOnCall.try_lock()){
		//launch thread responsible for pattern matching
		this->toSearchImg = img;
		thread tProc(&imSearch::doSearchOnImage, this);
		tProc.detach();
	}
	return false;
}

vector<imageSearchObjects::objectOccurence> imSearch::getOccurrences(){
	vector<imageSearchObjects::objectOccurence> ocList;
	this->lockOnWrite.lock();
	ocList = this->occurrenceList;
	this->lockOnWrite.unlock();
	return ocList;
}

string imSearch::getNameOf(int id){
	return this->images[id].filename;
}

void imSearch::FilterTemplates(){
	for(int i = 0; i < images.size(); i++){
		cv::Mat m = images[i].image;
		cv::Canny(m, images[i].image, 100,100);
    	cv::imshow("test", images[i].image);
    	cv::waitKey(0);
	}
}

void imSearch::doSearchOnImage(){
	mutex accessList;
	cout<<"on doSearchOnImage()"<<endl;
	cout<<this<<endl;
	//convert image to RGB
	cv::Mat im_gray , result, canny;
	cv::cvtColor(toSearchImg,im_gray,CV_RGB2GRAY);
	canny = im_gray;
	cv::Canny(im_gray, canny, 100,100);
	vector<imageSearchObjects::objectOccurence> ocList;
	//go through the list of templates to be matched

#pragma omp parallel for
	for(int i = 0; i < this->images.size(); i++){
		//create template heat map
		cv::matchTemplate(canny, this->images[i].image, result ,CV_TM_CCORR_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

		objectOccurence myOcc;
		myOcc.id = i;
		myOcc.position.x = maxLoc.x;
		myOcc.position.y = maxLoc.y;
		myOcc.position.width = images[i].image.size().width;
		myOcc.position.height = images[i].image.size().height;
		if(maxVal >= .65){
			accessList.lock();
			ocList.push_back(myOcc);
			accessList.unlock();
			cout << "MaxVal found = " << maxVal << " and MinVal = " << minVal << endl;
		}
	}

	//write occurence list
	this->lockOnWrite.lock();
	this->occurrenceList = ocList;
	this->lockOnWrite.unlock();

	//release mutex
	this->lockOnCall.unlock();
}

