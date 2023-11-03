#pragma once

#include <windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mferror.h>
#include <shlwapi.h>
#include <Mfreadwrite.h>
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <assert.h>

// utility functions
#include "utils.h"

using namespace flutter;

namespace record_windows
{

    struct PlayDevice {
        std::string id;
        std::string label;
    };

    class Player {
        public:
                HRESULT static ListOutputDevices(std::vector<PlayDevice>& devices);
                std::string static GetMediaTypeDescript(IMFMediaType *pMediaType);
                
        		Player();
		        virtual ~Player();
                void Dispose();

                void Ready(std::string deviceId);
                void writeOutputSink(DWORD flags, LONGLONG llTimeStamp, IMFSample* pAudioSample);

                IMFMediaType *GetMediaType() {
                    return m_pSinkSupportedType;
                }
        private:
                bool m_ready = false;
                
                HRESULT CreateAudioRendererDevice(std::string deviceId, IMFMediaSink **ppMediaSink);
                HRESULT CreateAudioSinkWriter(IMFMediaSink *pMediaSink, IMFSinkWriter **ppAudioSinkWriter, IMFStreamSink **ppStreamSink, IMFMediaTypeHandler **ppSinkMediaTypeHandler, IMFMediaType **ppSinkSupportedType);
                
                CritSec m_critSec;
    
                IMFMediaSink *m_pMediaSink = NULL;
                IMFSinkWriter *m_pAudioSinkWriter = NULL;
                IMFStreamSink* m_pStreamSink = NULL;
		        IMFMediaTypeHandler* m_pSinkMediaTypeHandler = NULL;
                IMFMediaType *m_pSinkSupportedType = NULL;
    };
};
