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
#include <omp.h>
#include "ServerInstance/ServerInstance.h"

using namespace std;


int main(void) {
	RestServer::ServerInstance myServer;
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

    	cv::imshow("camera1", img);

    	if (cvWaitKey(5) >= 0)
    		break;
    }

    myServer.stop();
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
