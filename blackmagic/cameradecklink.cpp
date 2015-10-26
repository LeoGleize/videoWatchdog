#include "cameradecklink.h"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include "cardexceptions.h"
#include <unistd.h>
#include <sys/time.h>

using namespace std;

CameraDecklink::CameraDecklink() {

	deckLinkIterator = CreateDeckLinkIteratorInstance();

	int exitStatus = 1;
	HRESULT result;

	if (!deckLinkIterator) {
		fprintf(stderr,
				"This application requires the DeckLink drivers installed.\n");
		bail();
	}
	/* Connect to the first DeckLink instance */
	result = deckLinkIterator->Next(&deckLink);
	if (result != S_OK) {
		fprintf(stderr, "No DeckLink PCI cards found.\n");
		bail();
	}

	this->initializeCamera(deckLink);

	return;

}


void CameraDecklink::bail(){
	if (deckLink != NULL) {
		deckLink->Release();
		deckLink = NULL;
	}

	if (deckLinkIterator != NULL)
		deckLinkIterator->Release();
	exit(0);
}

void CameraDecklink::initializeCamera(IDeckLink *_deckLink) {

	static int g_videoModeIndex = 8;
	static int g_audioChannels = 2;
	static int g_audioSampleDepth = 16;

	pixelFormat = bmdFormat8BitYUV;

	if (_deckLink->QueryInterface(IID_IDeckLinkInput,
			(void**) &deckLinkInput) != S_OK)
		return;

	delegate = new DeckLinkCaptureDelegate();
	pthread_mutex_init(delegate->sleepMutex, NULL);
	pthread_cond_init(delegate->sleepCond, NULL);
	deckLinkInput->SetCallback(delegate);

	// Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK) {
		fprintf(stderr,
				"Could not obtain the video output display mode iterator - result = %08x\n",
				result);
		return;
	}

	displayModeCount = 0;
	while (displayModeIterator->Next(&displayMode) == S_OK) {
		if (g_videoModeIndex == displayModeCount) {
			BMDDisplayModeSupport result;
			const char *displayModeName;

			foundDisplayMode = true;
			displayMode->GetName(&displayModeName);
			selectedDisplayMode = displayMode->GetDisplayMode();

//            cout << selectedDisplayMode << endl
//                 << "should be " << bmdModeHD1080i50 << endl;
#ifdef FIX_RESOLUTION
			selectedDisplayMode = bmdModeHD1080i50;
#endif
			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode,
					pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

			if (result == bmdDisplayModeNotSupported) {
				fprintf(stderr,
						"The display mode %s is not supported with the selected pixel format\n",
						displayModeName);
				return;
			}

			if (inputFlags & bmdVideoInputDualStream3D) {
				if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D)) {
					fprintf(stderr,
							"The display mode %s is not supported with 3D\n",
							displayModeName);
					return;
				}
			}

			break;
		}
		displayModeCount++;
		displayMode->Release();
	}

	if (!foundDisplayMode) {
		fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
		return;
	}

	result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat,
			inputFlags);

	if (result != S_OK) {
		fprintf(stderr,
				"Failed to enable video input. Is another application using the card?\n");
		return;
	}

	result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz,
			g_audioSampleDepth, g_audioChannels);
	if (result != S_OK) {
		return;
	}

	result = deckLinkInput->StartStreams();
	if (result != S_OK) {
		return;
	}
}

IplImage* CameraDecklink::captureLastFrame() {
	//pthread_mutex_lock(delegate->sleepMutex);
	pthread_cond_wait(delegate->sleepCond, delegate->sleepMutex);
	//pthread_mutex_unlock(delegate->sleepMutex);
//    fprintf(stderr, "Stopping Capture\n");

	return delegate->getLastImage();
}

/**
 * Method to capture video from the driver, this method can only be accessed
 * by one thread at any time due to concurrency constraints on the video card.
 */
cv::Mat CameraDecklink::captureLastCvMat() {
	mutexCallOnce.lock();
	/*Try to read last image, in case of exception release the mutex
	 * and throw exception to the next level
	 */
	try{
		IplImage* img = captureLastFrame();
		cv::Mat mat = cv::cvarrToMat(img).clone(); //free???
		cvRelease((void**) &img);
		mutexCallOnce.unlock();
		return mat;
	}catch(const CardException &ex){
		mutexCallOnce.unlock();
		throw ex;
	}
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() :
		m_refCount(0) {
	this->frameState = DECKLINK_VIDEO_OK;
	pthread_mutex_init(&m_mutex, NULL);

	lastImage = 0;
	frameCount = 0;
//    stopped = false;
	sleepMutex = new pthread_mutex_t();
	sleepCond = new pthread_cond_t();

	g_timecodeFormat = 0;

	height = 1080;
	width = 1920;
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate() {
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void) {
	pthread_mutex_lock(&m_mutex);
	m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG) m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void) {
	pthread_mutex_lock(&m_mutex);
	m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG) m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(
		IDeckLinkVideoInputFrame* videoFrame,
		IDeckLinkAudioInputPacket* audioFrame) {
	IDeckLinkVideoFrame* rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions* threeDExtensions = NULL;
	frameState = DECKLINK_VIDEO_OK;
	void* audioFrameBytes;
	// Handle Video Frame
	if (videoFrame) // && !stopped)
	{
		// If 3D mode is enabled we retreive the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ((videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions,
				(void **) &threeDExtensions) != S_OK)
				|| (threeDExtensions->GetFrameForRightEye(&rightEyeFrame)
						!= S_OK)) {
			rightEyeFrame = NULL;
		}

		if (threeDExtensions)
			threeDExtensions->Release();

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource) {
			fprintf(stderr,
					"Frame received (#%lu) - No input signal detected\n",
					frameCount);
			frameState = DECKLINK_NO_VIDEO_INPUT;
		} else {
			const char *timecodeString = NULL;

			if (g_timecodeFormat != 0) {
				IDeckLinkTimecode *timecode;
				if (videoFrame->GetTimecode(g_timecodeFormat, &timecode) == S_OK) {
					timecode->GetString(&timecodeString);
				}
			}
//			printf("#%lu frame received: [%s] - Pointer = 0x%x - size = %d px\n",frameCount,
//					timecodeString != NULL ? timecodeString : "No timecode", videoFrame, videoFrame->GetHeight() * videoFrame->GetWidth());
			if (timecodeString)
				free((void*) timecodeString);

			videoFrame->GetBytes(&frameBytes);

		}

		if (rightEyeFrame)
			rightEyeFrame->Release();

		frameCount++;

		pthread_cond_signal(sleepCond); // kill after one frame
		//stopped = true;
		//        if (g_maxFrames > 0 && frameCount >= g_maxFrames)
		//        {
		//            stopped = true;
		//            pthread_cond_signal(&sleepCond);
		//        }
	}
	return S_OK;
}

IplImage* DeckLinkCaptureDelegate::getLastImage() {
	if(frameState == DECKLINK_NO_VIDEO_INPUT){
		throw CardException("No video input", NO_INPUT_EXCEPTION);
	}else{
		lastImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
		convertFrameToOpenCV(frameBytes, lastImage);
		return lastImage;
	}
}

//TOFIX: clamping of values once they are out of range! (will it solve color distortions?)
void DeckLinkCaptureDelegate::convertFrameToOpenCV(void* frameBytes,
		IplImage * m_RGB) {
	if (!m_RGB)
		m_RGB = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

	unsigned char *pData = (unsigned char *) frameBytes;

	for (int i = 0, j = 0; i < width * height * 3; i += 6, j += 4) {
		unsigned char u = pData[j];
		unsigned char y = pData[j + 1];
		unsigned char v = pData[j + 2];

		//fprintf(stderr, "%d\n", v);
		m_RGB->imageData[i + 2] = cv::saturate_cast<uchar>(y+1.402*(v-128));          		   // r
		m_RGB->imageData[i + 1] = cv::saturate_cast<uchar>(y-0.344*(u-128)-0.71414*(v-128));   // g
		m_RGB->imageData[i] = cv::saturate_cast<uchar>(y+1.772*(u-128));                       // b

		y = pData[j + 3];
		m_RGB->imageData[i + 5] = cv::saturate_cast<uchar>(y+1.402*(v-128));          			// r
		m_RGB->imageData[i + 4] = cv::saturate_cast<uchar>(y-0.344*(u-128)-0.71414*(v-128));    // g
		m_RGB->imageData[i + 3] = cv::saturate_cast<uchar>(y+1.772*(u-128));                    // b
	}

}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(
		BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode,
		BMDDetectedVideoInputFormatFlags) {
	return S_OK;
}
