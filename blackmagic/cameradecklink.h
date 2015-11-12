#ifndef CAMERADECKLINK_H
#define CAMERADECKLINK_H

#include "./cardSDK/DeckLinkAPI.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <mutex>
#include "cardexceptions.h"
#include <exception>
#include <thread>
#include <fstream>
#include <queue>

#define N_AUDIO_BUFFERS_STORE 100
enum FRAME_STATE{DECKLINK_VIDEO_OK, DECKLINK_NO_VIDEO_INPUT};

class DeckLinkCaptureDelegate: public IDeckLinkInputCallback {
public:
	DeckLinkCaptureDelegate(bool isFullHD);
	~DeckLinkCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv) {
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(
			BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*,
			BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(
			IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*);

	IplImage* getLastImage();

	void *getLastAudioBuffer(int *nBytesWritten);
	void convertFrameToOpenCV(void* frameBytes, IplImage * m_RGB);

	pthread_mutex_t *sleepMutex;
	pthread_cond_t *sleepCond;
	//        bool stopped;
	//        bool converting;
	void* frameBytes;
	long frameCount;
	BMDTimecodeFormat g_timecodeFormat;

	void getAudioData(void **pointerToData, int *size);
private:
	std::deque<std::pair<void *, unsigned short> > audioData;
	ULONG m_refCount;
	FRAME_STATE frameState;
	pthread_mutex_t m_mutex;
	pthread_mutex_t m_audio_mutex;
	IplImage* lastImage;
	std::ofstream audioWrite;
	int height, width;

};

class CameraDecklink {
public:
	CameraDecklink(bool isFullHD);

	void initializeCamera(IDeckLink * _deckLink);

	IplImage * captureLastFrame();
	IplImage * captureLastFrameAndAudio(void **ptrToAudio, int *nBytes);
	cv::Mat captureLastCvMatClone();
	cv::Mat captureLastCvMat(IplImage **p);
	cv::Mat captureLastCvMatAndAudio(IplImage **p, void **ptrToAudio, int *nBytesToAudio);
	void getAudioData(void **pointerToData, int *size);
	bool getIsFullHD();
private:
	bool isFullHD;
	IDeckLink *deckLink;
	IDeckLinkIterator *deckLinkIterator;
	DeckLinkCaptureDelegate *delegate;
	std::mutex mutexCallOnce;
	short audioBufferCounter;
	std::mutex threadMutexAcquire;
	IDeckLinkInput *deckLinkInput;
	IDeckLinkDisplayModeIterator *displayModeIterator;
	IDeckLinkDisplayMode *displayMode;
	BMDVideoInputFlags inputFlags;
	BMDDisplayMode selectedDisplayMode;
	BMDPixelFormat pixelFormat;
	int displayModeCount;
	int exitStatus;
	bool foundDisplayMode;
	HRESULT result;

	void bail();

};

#endif // CAMERADECKLINK_H
