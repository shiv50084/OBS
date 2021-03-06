
#include "ExtendVideoFrameObserver.hpp"
#include <TCHAR.h>
//#include <timeapi.h>
#include <stdio.h>


static unsigned char imageBuffer[1920 * 1080 * 4] = { 0 };
pthread_mutex_t buffer_mutex;
CExtendVideoFrameObserver::CExtendVideoFrameObserver()
{
    //	m_lpImageBuffer = new BYTE[0x800000];

    m_RenderWidth = 0;
    m_RenderHeight = 0;

    pthread_mutexattr_t attr;
    pthread_mutex_init_value(&buffer_mutex);

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&buffer_mutex, &attr);
}


CExtendVideoFrameObserver::~CExtendVideoFrameObserver()
{
    pthread_mutex_destroy(&buffer_mutex);
}

int CExtendVideoFrameObserver::_PrintObserverVideoFrame(VideoFrame* frame)
{
    unsigned char* srcBuffer = imageBuffer;
    for (int planeNum = 0; planeNum < 3; ++planeNum) {
        int width = (planeNum ? (frame->width + 1) / 2 : frame->width);
        int height;
        switch (frame->type) {
        case FRAME_TYPE_YUV420:
            height = (planeNum ? (frame->height + 1) / 2 : frame->height);
            break;
            //     case FRAME_TYPE_YUV422:
            //         height = frame->height;
            //         break;
        default:
            height = (planeNum ? (frame->height + 1) / 2 : frame->height);
            break;
        }

        agora::media::IVideoFrame::PLANE_TYPE plane_type = static_cast<agora::media::IVideoFrame::PLANE_TYPE>(planeNum);
        unsigned char* plane_buffer = NULL;
        int stride = frame->yStride;
        if (planeNum == 0) {
            plane_buffer = (unsigned char*)frame->yBuffer;
            stride = frame->yStride;
        }

        else if (planeNum == 1) {
            plane_buffer = (unsigned char*)frame->uBuffer;
            stride = frame->uStride;
        }
        else if (planeNum == 2) {
            plane_buffer = (unsigned char*)frame->vBuffer;
            stride = frame->vStride;
        }

        for (int y = 0; y < height; y++) {
            memcpy_s(plane_buffer, width, srcBuffer, width);
            plane_buffer += stride;
            srcBuffer += width;
        }
    }
    return 0;
}

int timeinc = 0;
bool CExtendVideoFrameObserver::onCaptureVideoFrame(VideoFrame& videoFrame)
{
    int size = videoFrame.yStride * videoFrame.height * 3 / 2;
    int bufSize = 0;
    pthread_mutex_lock(&buffer_mutex);
    if (videoFrame.yStride == videoFrame.width) {
        int nUvLen = videoFrame.height * videoFrame.width / 4;
        int nYLen = nUvLen * 4;

        memcpy_s(videoFrame.yBuffer, nYLen, imageBuffer, nYLen);
        memcpy_s(videoFrame.uBuffer, nUvLen, imageBuffer + nYLen, nUvLen);
        memcpy_s(videoFrame.vBuffer, nUvLen, imageBuffer + nYLen + nUvLen, nUvLen);
    }
    else {
        _PrintObserverVideoFrame(&videoFrame);
    }
    pthread_mutex_unlock(&buffer_mutex);
    videoFrame.renderTimeMs = GetTickCount();
    return true;
}

bool CExtendVideoFrameObserver::onRenderVideoFrame(unsigned int uid, VideoFrame& videoFrame)
{
	
	return true;
}

void CExtendVideoFrameObserver::setVideoResolution(int w, int h)
{
	m_resolutionX = w;
	m_resolutionY = h;
}

void CExtendVideoFrameObserver::pushBackVideoFrame(void* buffer, int size)
{
    pthread_mutex_lock(&buffer_mutex);
    memcpy_s(imageBuffer, size, buffer, size);
    pthread_mutex_unlock(&buffer_mutex);
}