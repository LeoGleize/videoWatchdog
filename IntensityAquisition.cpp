//============================================================================
// Name        : IntensityAquisition.cpp
// Author      : Filipe Caldas
// Version     :
// Copyright   : 
// Description : Hello World in C, Ansi-style
//============================================================================

#include <iostream>
#include "blackmagic/cameradecklink.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include "recognition/imSearch.h"
#include <omp.h>

using namespace std;


int main(void) {
	imSearch 		  imageSearch;
    IDeckLink 		  *deckLink;
    IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();

    CameraDecklink    *camera1;
    int				  exitStatus = 1;
    HRESULT			  result;

    if (!deckLinkIterator)
    {
        fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
        goto bail;
    }

    camera1 = new CameraDecklink();


    /* Connect to the first DeckLink instance */
    result = deckLinkIterator->Next(&deckLink);
    if (result != S_OK)
    {
       fprintf(stderr, "No DeckLink PCI cards found.\n");
       goto bail;
    }

    camera1->initializeCamera(deckLink);
    cout << "displaying images!"<<endl;

    while (true) {
    	cv::Mat img = camera1->captureLastCvMat();

    	imageSearch.doSearch(img);
    	vector<imageSearchObjects::objectOccurence> ocList = imageSearch.getOccurrences();
    	for(int i = 0; i < ocList.size(); i++){
    		cv::rectangle(img,ocList[i].position,cv::Scalar(0,250,0),3);
    		cv::putText( img,
    					 imageSearch.getNameOf(ocList[i].id),
    					 cv::Point(ocList[i].position.x,ocList[i].position.y - 5),
    					 CV_FONT_HERSHEY_PLAIN,
    					 1.2,
    					 cv::Scalar(0,250,0));
    	}
    	cv::Mat m2 = img;
    	//cv::Canny(m2, img, 100,150);
    	cv::imshow("camera1", img);

    	if (cvWaitKey(10) >= 0)
    		break;
    }

    return 0;
bail:

    if (deckLink != NULL)
    {
        deckLink->Release();
        deckLink = NULL;
    }

    if (deckLinkIterator != NULL)
        deckLinkIterator->Release();
	return 0;
}
