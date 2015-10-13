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

void imSearch::doSearchOnImage(){
	cout<<"on doSearchOnImage()"<<endl;
	cout<<this<<endl;
	//convert image to RGB
	cv::Mat im_gray , result;
	cv::cvtColor(toSearchImg,im_gray,CV_RGB2GRAY);
	vector<imageSearchObjects::objectOccurence> ocList;
	//go through the list of templates to be matched
	for(int i = 0; i < this->images.size(); i++){
		//create template heat map
		cv::matchTemplate(im_gray, this->images[i].image, result ,CV_TM_SQDIFF_NORMED);

		double minVal, maxVal;
		cv::Point minLoc, maxLoc;
		cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
		cout << "MaxVal found = " << maxVal << " and MinVal = " << minVal << endl;
		objectOccurence myOcc;
		myOcc.id = i;
		myOcc.position.x = minLoc.x;
		myOcc.position.y = minLoc.y;
		myOcc.position.width = images[i].image.size().width;
		myOcc.position.height = images[i].image.size().height;
		if(minVal < 0.55)
			ocList.push_back(myOcc);
	}

	//write occurence list
	this->lockOnWrite.lock();
	this->occurrenceList = ocList;
	this->lockOnWrite.unlock();

	//release mutex
	this->lockOnCall.unlock();
}

