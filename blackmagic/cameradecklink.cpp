#include "cameradecklink.h"
#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include "cardexceptions.h"
#include <unistd.h>
#include <sys/time.h>

using namespace std;

CameraDecklink::CameraDecklink(bool isFullHD) {
	this->isFullHD = isFullHD;
	deckLinkIterator = CreateDeckLinkIteratorInstance();

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

bool CameraDecklink::getIsFullHD(){
	return this->isFullHD;
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
	static int g_audioSampleDepth = bmdAudioSampleType16bitInteger;

	pixelFormat = bmdFormat8BitYUV;

	if (_deckLink->QueryInterface(IID_IDeckLinkInput,
			(void**) &deckLinkInput) != S_OK)
		return;

	delegate = new DeckLinkCaptureDelegate(isFullHD);
	pthread_mutex_init(delegate->sleepMutex, NULL);
	pthread_cond_init(delegate->sleepCond, NULL);


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

			if(isFullHD){
				std::cout<<"Setting screen resolution to HD1080i50"<<std::endl;
				selectedDisplayMode = bmdModeHD1080i50;
			}else{
				std::cout<<"Setting screen resolution to HD720p50"<<std::endl;
				selectedDisplayMode = bmdModeHD720p50;
			}

			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode,
					pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

			if (result == bmdDisplayModeNotSupported) {
				fprintf(stderr,
						"The display mode %s is not supported with the selected pixel format\n",
						displayModeName);
				return;
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

	result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);

	if (result != S_OK) {
		fprintf(stderr, "Failed to enable audio input\n");
		return;
	}
	deckLinkInput->SetCallback(delegate);
	//start audio and video decoding sync
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

IplImage* CameraDecklink::captureLastFrameAndAudio(void **ptrToAudio, int *nBytes) {
	//pthread_mutex_lock(delegate->sleepMutex);
	pthread_cond_wait(delegate->sleepCond, delegate->sleepMutex);
	//pthread_mutex_unlock(delegate->sleepMutex);
//    fprintf(stderr, "Stopping Capture\n");
	*ptrToAudio = delegate->getLastAudioBuffer(nBytes);
	return delegate->getLastImage();
}

/**
 * Method to capture video from the driver, this method can only be accessed
 * by one thread at any time due to concurrency constraints on the video card.
 */
cv::Mat CameraDecklink::captureLastCvMatClone() {
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

/**
 * Method to capture video from the driver, this method can only be accessed
 * by one thread at any time due to concurrency constraints on the video card.
 * The image returned has to be freed by user and should not be held (is mutable)
 */
cv::Mat CameraDecklink::captureLastCvMat(IplImage **p) {
	mutexCallOnce.lock();
	/*Try to read last image, in case of exception release the mutex
	 * and throw exception to the next level
	 */
	try{
		IplImage* img = captureLastFrame();
		cv::Mat mat = cv::cvarrToMat(img); //free???
//		cvRelease((void**) &img);
		mutexCallOnce.unlock();
		*p = img;
		return mat;
	}catch(const CardException &ex){
		mutexCallOnce.unlock();
		throw ex;
	}
}

cv::Mat CameraDecklink::captureLastCvMatAndAudio(IplImage **p, void **ptrToAudio, int *nBytesToAudio) {
	mutexCallOnce.lock();
	/*Try to read last image, in case of exception release the mutex
	 * and throw exception to the next level
	 */
	try{
		IplImage* img = captureLastFrameAndAudio(ptrToAudio, nBytesToAudio);
		cv::Mat mat = cv::cvarrToMat(img); //free???
		mutexCallOnce.unlock();
		*p = img;
		return mat;
	}catch(const CardException &ex){
		mutexCallOnce.unlock();
		throw ex;
	}
}

void CameraDecklink::getAudioData(void **pointerToData, int *size){
	this->delegate->getAudioData(pointerToData, size);
}


/*
 * Create a capture delegate
 */
DeckLinkCaptureDelegate::DeckLinkCaptureDelegate(bool isFullHD) :
		m_refCount(0) {
	this->frameState = DECKLINK_VIDEO_OK;
	pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_init(&m_audio_mutex, NULL);
	lastImage = 0;
	frameCount = 0;
//    stopped = false;
	sleepMutex = new pthread_mutex_t();
	sleepCond = new pthread_cond_t();

	g_timecodeFormat = 0;

	if(isFullHD){
		height = 1080;
		width = 1920;
	}else{
		height = 720;
		width = 1280;
	}
	audioWrite.open("rawaudio.raw", std::ofstream::binary);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate() {
	pthread_mutex_destroy(&m_mutex);
	pthread_mutex_destroy(&m_audio_mutex);
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
	static int g_audioChannels = 2;
	IDeckLinkVideoFrame* rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions* threeDExtensions = NULL;
	frameState = DECKLINK_VIDEO_OK;

	// Handle Video Frame
	if (videoFrame) // && !stopped)
	{
		// If 3D mode is enabled we retrieve the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ((videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions,
				(void **) &threeDExtensions) != S_OK)
				|| (threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK)) {
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

			if (timecodeString)
				free((void*) timecodeString);

			videoFrame->GetBytes(&frameBytes);

		}

		if (rightEyeFrame)
			rightEyeFrame->Release();

		frameCount++;

		//read audio frame to buffer
		if(audioFrame){
			void* audioBuffer = NULL;
			audioFrame->GetBytes(&audioBuffer); //get pointer to data
			const int audioNBytes = audioFrame->GetSampleFrameCount() * sizeof(int16_t) * g_audioChannels;
			void *pcopy = malloc(audioNBytes);
			memcpy(pcopy, audioBuffer, audioNBytes);
			if(pthread_mutex_trylock(&m_audio_mutex) == 0){
				this->audioData.push_back(std::pair<void*,int>(pcopy, audioNBytes));
				if(audioData.size() > N_AUDIO_BUFFERS_STORE){
					free(this->audioData[0].first);
					this->audioData.pop_front();
				}
				pthread_mutex_unlock(&m_audio_mutex);
			}else{
				free(pcopy);
			}
		}
		pthread_cond_signal(sleepCond);
	}
	return S_OK;
}

void * DeckLinkCaptureDelegate::getLastAudioBuffer(int *nBytesWritten){
	if(this->audioData.size() > 0){
		int nBytes = audioData.back().second;
		void *data = malloc(nBytes);
		memcpy(data, audioData.back().first, nBytes);
		*nBytesWritten = nBytes;
		return data;
	}
	return NULL;
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

/**
 * IntensityPro cards get video Frames in an YUV format but OpenCV frames are in an
 * BGR format, the folowing routine does the conversion
 */
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
		m_RGB->imageData[i + 5] = cv::saturate_cast<uchar>(y+1.402*(v-128));          		   // r
		m_RGB->imageData[i + 4] = cv::saturate_cast<uchar>(y-0.344*(u-128)-0.71414*(v-128));   // g
		m_RGB->imageData[i + 3] = cv::saturate_cast<uchar>(y+1.772*(u-128));                   // b
	}

}

/**
 * getAudioData receives a pointer to a void pointer and a pointer to an integer value
 * 				it'll then allocate memory for a buffer containing audio caught in the last
 * 				N_AUDIO_BUFFERS_STORE frames. This buffer needs to be freed by user and has
 * 				size bytes.
 * 				Audio is stored in raw format with two channels interleaved and each sample has
 * 				the size of 2 bytes.
  */
void DeckLinkCaptureDelegate::getAudioData(void **pointerToData, int *size){
	pthread_mutex_lock(&m_audio_mutex);
	unsigned int dataSz = 0;

	for(unsigned int i = 0; i < this->audioData.size(); i++)
		dataSz += audioData[i].second;
	*pointerToData = malloc(dataSz);
	*size = dataSz;
	unsigned int j = 0;
	char * ptrcpy = (char *) * pointerToData;
	for(unsigned int i = 0; i < this->audioData.size(); i++){
		memcpy(ptrcpy+j,this->audioData[i].first, this->audioData[i].second);
		j += audioData[i].second;
	}
	pthread_mutex_unlock(&m_audio_mutex);
	return;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(
		BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode,
		BMDDetectedVideoInputFormatFlags) {
	return S_OK;
}
