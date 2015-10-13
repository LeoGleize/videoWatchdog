/*
 * iSearch.h
 *
 *  Created on: Oct 12, 2015
 *      Author: fcaldas
 */

#ifndef ISEARCH_H_
#define ISEARCH_H_

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std;

namespace imageSearchObjects{

	struct __imData{
		string filename;
		cv::Mat image;
//		bool hasMask;
//		cv::Mat mask;
	};

	struct objectOccurence{
		int id;
		cv::Rect position;
	};
}

class imSearch {

public:

	imSearch();

	virtual ~imSearch();

	bool hasFinished();

	bool doSearch(cv::Mat &img);

	string getNameOf(int id);

	vector<imageSearchObjects::objectOccurence> getOccurrences();
private:
	std::mutex lockOnWrite;
	std::mutex lockOnCall;
	vector<imageSearchObjects::__imData> images;
	bool finished = false;
	string path = "./patterns";
	vector<imageSearchObjects::objectOccurence> occurrenceList;
	cv::Mat toSearchImg;

	void doSearchOnImage();
};

#endif /* ISEARCH_H_ */
