#include "cameradecklink.h"

CameraDecklink::CameraDecklink()
{
}

void CameraDecklink::initializeCamera(IDeckLink *_deckLink){

    static int	g_videoModeIndex = 8;
    static int	g_audioChannels = 2;
    static int	g_audioSampleDepth = 16;

    pixelFormat = bmdFormat8BitYUV;

    if (_deckLink->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)
        return;

    delegate = new DeckLinkCaptureDelegate();
    pthread_mutex_init(delegate->sleepMutex, NULL);
    pthread_cond_init(delegate->sleepCond, NULL);
    deckLinkInput->SetCallback(delegate);

    // Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output
    result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
    if (result != S_OK)
    {
        fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
        return;
    }

    displayModeCount = 0;
    while (displayModeIterator->Next(&displayMode) == S_OK)
    {
        if (g_videoModeIndex == displayModeCount)
        {
            BMDDisplayModeSupport result;
            const char *displayModeName;

            foundDisplayMode = true;
            displayMode->GetName(&displayModeName);
            selectedDisplayMode = displayMode->GetDisplayMode();

            deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);

            if (result == bmdDisplayModeNotSupported)
            {
                fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
                return;
            }

            if (inputFlags & bmdVideoInputDualStream3D)
            {
                if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D))
                {
                    fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
                    return;
                }
            }

            break;
        }
        displayModeCount++;
        displayMode->Release();
    }

    if (!foundDisplayMode)
    {
        fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
        return;
    }

    result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
    if(result != S_OK)
    {
        fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
        return;
    }

    result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
    if(result != S_OK)
    {
        return;
    }

    result = deckLinkInput->StartStreams();
    if(result != S_OK)
    {
        return;
    }
}

IplImage* CameraDecklink::captureLastFrame(){
    //pthread_mutex_lock(delegate->sleepMutex);
    pthread_cond_wait(delegate->sleepCond, delegate->sleepMutex);
    //pthread_mutex_unlock(delegate->sleepMutex);
    fprintf(stderr, "Stopping Capture\n");

    return delegate->getLastImage();
}

cv::Mat CameraDecklink::captureLastCvMat(){
    IplImage* img = captureLastFrame();
    cv::Mat mat =  cv::cvarrToMat(img); //free???
    return mat;
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate() : m_refCount(0)
{
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

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
    pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
    pthread_mutex_lock(&m_mutex);
    m_refCount++;
    pthread_mutex_unlock(&m_mutex);

    return (ULONG)m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
    pthread_mutex_lock(&m_mutex);
    m_refCount--;
    pthread_mutex_unlock(&m_mutex);

    if (m_refCount == 0)
    {
        delete this;
        return 0;
    }

    return (ULONG)m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioFrame)
{
    IDeckLinkVideoFrame*	                rightEyeFrame = NULL;
    IDeckLinkVideoFrame3DExtensions*        threeDExtensions = NULL;
    unsigned char * pData;
    void*					audioFrameBytes;

    // Handle Video Frame
    if(videoFrame)// && !stopped)
    {
        // If 3D mode is enabled we retreive the 3D extensions interface which gives.
        // us access to the right eye frame by calling GetFrameForRightEye() .
        if ( (videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **) &threeDExtensions) != S_OK) ||
             (threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK))
        {
            rightEyeFrame = NULL;
        }

        if (threeDExtensions)
            threeDExtensions->Release();

        if (videoFrame->GetFlags() & bmdFrameHasNoInputSource)
        {
            fprintf(stderr, "Frame received (#%lu) - No input signal detected\n", frameCount);
        }
        else
        {
            const char *timecodeString = NULL;
            if (g_timecodeFormat != 0)
            {
                IDeckLinkTimecode *timecode;
                if (videoFrame->GetTimecode(g_timecodeFormat, &timecode) == S_OK)
                {
                    timecode->GetString(&timecodeString);
                }
            }

            fprintf(stderr, "Frame received (#%lu) [%s] - %s - Size: %li bytes\n",
                    frameCount,
                    timecodeString != NULL ? timecodeString : "No timecode",
                    rightEyeFrame != NULL ? "Valid Frame (3D left/right)" : "Valid Frame",
                    videoFrame->GetRowBytes() * videoFrame->GetHeight());

            if (timecodeString)
                free((void*)timecodeString);


            videoFrame->GetBytes(&frameBytes);
            //            converting = true;
            //            lastImage = cvCreateImage(cvSize(1920, 1080), IPL_DEPTH_8U, 3);
            //            converting = false;
            //            CameraDecklink::convertFrameToOpenCV(frameBytes, lastImage);

            //                cvShowImage("Hola", lastImage);
            //                cvWaitKey(27);
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

IplImage* DeckLinkCaptureDelegate::getLastImage(){
    lastImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);
    convertFrameToOpenCV(frameBytes, lastImage);
    return lastImage;
}

void DeckLinkCaptureDelegate::convertFrameToOpenCV(void* frameBytes, IplImage * m_RGB){
    if(!m_RGB)  m_RGB = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

    unsigned char* pData = (unsigned char *) frameBytes;

    for(int i = 0, j=0; i < width * height * 3; i+=6, j+=4)
    {
        unsigned char u = pData[j];
        unsigned char y = pData[j+1];
        unsigned char v = pData[j+2];

        //fprintf(stderr, "%d\n", v);
        m_RGB->imageData[i+2] = 1.0*y + 8 + 1.402*(v-128);               // r
        m_RGB->imageData[i+1] = 1.0*y - 0.34413*(u-128) - 0.71414*(v-128);   // g
        m_RGB->imageData[i] = 1.0*y + 1.772*(u-128) + 0;                            // b

        y = pData[j+3];
        m_RGB->imageData[i+5] = 1.0*y + 8 + 1.402*(v-128);               // r
        m_RGB->imageData[i+4] = 1.0*y - 0.34413*(u-128) - 0.71414*(v-128);   // g
        m_RGB->imageData[i+3] = 1.0*y + 1.772*(u-128) + 0;
    }
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode *mode, BMDDetectedVideoInputFormatFlags)
{
    return S_OK;
}
