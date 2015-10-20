#ifndef CAMERADECKLINK_H
#define CAMERADECKLINK_H

#include "./cardSDK/DeckLinkAPI.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <mutex>
#include "cardexceptions.h"
#include <exception>
#define FIX_RESOLUTION

enum FRAME_STATE{DECKLINK_VIDEO_OK, DECKLINK_NO_VIDEO_INPUT};

class DeckLinkCaptureDelegate: public IDeckLinkInputCallback {
public:
	DeckLinkCaptureDelegate();
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

	void convertFrameToOpenCV(void* frameBytes, IplImage * m_RGB);

	pthread_mutex_t *sleepMutex;
	pthread_cond_t *sleepCond;
	//        bool stopped;
	//        bool converting;
	void* frameBytes;
	long frameCount;
	BMDTimecodeFormat g_timecodeFormat;

private:
	ULONG m_refCount;
	FRAME_STATE frameState;
	pthread_mutex_t m_mutex;
	IplImage* lastImage;

	int height, width;

};

class CameraDecklink {
public:
	CameraDecklink();

	void initializeCamera(IDeckLink * _deckLink);

	IplImage * captureLastFrame();
	cv::Mat captureLastCvMat();

private:
	IDeckLink *deckLink;
	IDeckLinkIterator *deckLinkIterator;

	DeckLinkCaptureDelegate *delegate;
	std::mutex mutexCallOnce;
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
