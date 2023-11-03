#include "player.h"
#include "MFUtility.h"

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }
#define CHECK_HR(hr, msg) if (hr != S_OK) { printf(msg); printf(" Error: %.2X.\n", hr); goto done; }

HRESULT CreateAudioProfileIn(IMFMediaType** ppMediaType)
{
		HRESULT hr = S_OK;

		IMFMediaType* pMediaType = NULL;

		hr = MFCreateMediaType(&pMediaType);

		if (SUCCEEDED(hr))
		{
			hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		}
		if (SUCCEEDED(hr))
		{
			hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
		}
		if (SUCCEEDED(hr))
		{
			hr = pMediaType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
		}
		if (SUCCEEDED(hr))
		{
			hr = pMediaType->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
		}
		if (SUCCEEDED(hr))
		{
			hr = pMediaType->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 1);
		}
		if (SUCCEEDED(hr))
		{
			//hr = pMediaType->SetUINT32(MF_MT_AVG_BITRATE, m_pConfig->bitRate);
		}
		if (SUCCEEDED(hr))
		{
			*ppMediaType = pMediaType;
			(*ppMediaType)->AddRef();
		}

		SafeRelease(&pMediaType);

		return hr;
}

namespace record_windows {
	Player::Player()
	{
	}

	Player::~Player()
	{
        Dispose();
        
	}

    void Player::Dispose() {
        m_ready = false;
        SAFE_RELEASE(m_pSinkSupportedType);
        SAFE_RELEASE(m_pStreamSink);
        SAFE_RELEASE(m_pAudioSinkWriter);
        SAFE_RELEASE(m_pMediaSink);
        SAFE_RELEASE(m_pSinkMediaTypeHandler);
    }

    void Player::Ready(std::string deviceId) {
        //m_critSec.Lock();

        Dispose();
        HRESULT hr = CreateAudioRendererDevice(deviceId, &m_pMediaSink);
        assert(hr == S_OK);
        if (hr != S_OK) {
            //m_critSec.Unlock();
            return;
        }
        hr = CreateAudioSinkWriter(m_pMediaSink, &m_pAudioSinkWriter, &m_pStreamSink, &m_pSinkMediaTypeHandler, &m_pSinkSupportedType);
        assert(hr == S_OK);
        if (hr != S_OK) {
            m_ready = false;
            //m_critSec.Unlock();
            return;
        }
        m_ready = true;
        //m_critSec.Unlock();
    }

    
	void Player::writeOutputSink(DWORD flags, LONGLONG llTimeStamp, IMFSample* pAudioSample) {
        //m_critSec.Lock();
        if (m_ready == false) {
            //m_critSec.Unlock();
            return;
        }
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			printf("End of stream.\n");
            //m_critSec.Unlock();
			return ;
		}
		if (flags & MF_SOURCE_READERF_STREAMTICK) {
			printf("Stream tick.\n");
			m_pAudioSinkWriter->SendStreamTick(0, llTimeStamp);
		} 
		if (!pAudioSample) {
			printf("Null audio sample.\n");
		}
		else {
			//LONGLONG sampleDuration = 0;
			//pAudioSample->SetSampleTime(llTimeStamp);
			//pAudioSample->GetSampleDuration(&sampleDuration);
			auto hr = m_pAudioSinkWriter->WriteSample(0, pAudioSample);
			assert(hr == S_OK);
		}
        //m_critSec.Unlock();
	}

	std::string Player::GetMediaTypeDescript(IMFMediaType *pMediaType) {
		return GetMediaTypeDescription(pMediaType);
	}

	HRESULT Player::ListOutputDevices(std::vector<PlayDevice>& devices) {
		HRESULT hr = S_OK;

		IMMDeviceEnumerator* pEnumerator = NULL;      // Audio device enumerator.
		IMMDeviceCollection* pCollection = NULL;   // Audio device collection.
		IMMDevice* pEndpoint = NULL;        // Device ID.

		IPropertyStore* pProps = NULL;
		LPWSTR pwszID = NULL;

		// Create the device enumerator.
		hr = CoCreateInstance(
			__uuidof(MMDeviceEnumerator),
			NULL,
			CLSCTX_ALL,
			__uuidof(IMMDeviceEnumerator),
			(void**)&pEnumerator
		);
		EXIT_ON_ERROR(hr)


		// Enumerate the rendering devices.
		hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
		EXIT_ON_ERROR(hr)

		UINT count;
		hr = pCollection->GetCount(&count); 
		EXIT_ON_ERROR(hr)


		for (UINT i = 0; i < count; i++) {

			hr = pCollection->Item(i, &pEndpoint);
			EXIT_ON_ERROR(hr)
			hr = pEndpoint->GetId(&pwszID);
			EXIT_ON_ERROR(hr)
			hr = pEndpoint->OpenPropertyStore(STGM_READ, &pProps);
			EXIT_ON_ERROR(hr)

			PROPVARIANT varName;
			PropVariantInit(&varName);
			hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
			EXIT_ON_ERROR(hr)
			// GetValue succeeds and returns S_OK if PKEY_Device_FriendlyName is not found.
			// In this case vartName.vt is set to VT_EMPTY.      
			if (varName.vt != VT_EMPTY) {
			// Print endpoint friendly name and endpoint ID.
                PlayDevice device;
                device.id = toString(pwszID);
                device.label = toString(varName.pwszVal);
				devices.push_back(device);
			}
			
			CoTaskMemFree(pwszID);
			pwszID = NULL;
			PropVariantClear(&varName);
			SAFE_RELEASE(pProps)
			SAFE_RELEASE(pEndpoint)
		}

	Exit:
		//printf("Error!\n");
		CoTaskMemFree(pwszID);
		SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pCollection)
		SAFE_RELEASE(pEndpoint)
		SAFE_RELEASE(pProps)
		return hr;
	}
	
	HRESULT Player::CreateAudioRendererDevice(std::string deviceId, IMFMediaSink **ppMediaSink) {
		//return GetAudioOutputDevice(0, ppMediaSink);
		
		std::wstring wDeviceId = std::wstring(deviceId.begin(), deviceId.end());
		HRESULT hr = S_OK;
		IMFAttributes* pAttributes = NULL;

		//IMFActivate* pActivate = NULL;          // Activation object.
		//BOOL fSelected = false;
		//DWORD sourceStreamCount = 0, sinkStreamCount = 0, sinkStreamIndex = 0, sinkMediaTypeCount = 0;

		hr = MFCreateAttributes(&pAttributes, 1);
		assert(hr == S_OK);
		hr = pAttributes->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, wDeviceId.c_str());
		assert(hr ==S_OK);
		hr = MFCreateAudioRenderer(pAttributes, ppMediaSink);
		assert(hr==S_OK);
		//hr = MFCreateAudioRendererActivate(&pActivate);
		//assert(hr == S_OK);
		//hr = pActivate->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, deviceId);
		//assert(hr ==S_OK);
		SAFE_RELEASE(pAttributes);
        return hr;
	}

    HRESULT Player::CreateAudioSinkWriter(IMFMediaSink *pMediaSink, IMFSinkWriter **ppAudioSinkWriter, IMFStreamSink **ppStreamSink, IMFMediaTypeHandler **ppSinkMediaTypeHandler, IMFMediaType **ppSinkSupportedType) {

		IMFStreamSink* pStreamSink = NULL;
		IMFMediaTypeHandler* pSinkMediaTypeHandler = NULL;
		IMFMediaType* pSinkSupportedType = NULL;

		HRESULT hr = pMediaSink->GetStreamSinkByIndex(0, &pStreamSink);
		assert(hr == S_OK);

		hr = pStreamSink->GetMediaTypeHandler(&pSinkMediaTypeHandler);
		assert(hr == S_OK);



		// ----- Wire up the source and sink. -----

		// hr = m_pReader->GetCurrentMediaType(0, &m_pMediaType);
		// if (pSinkSupportedType) {
		// 	IMFMediaType* pPreferSinkSupportedType = NULL;
		// 	hr = pSinkMediaTypeHandler->IsMediaTypeSupported(pSinkSupportedType, &pPreferSinkSupportedType);
		// 	if (hr != S_OK && pPreferSinkSupportedType != nullptr) {
		// 		hr = pSinkMediaTypeHandler->IsMediaTypeSupported(pPreferSinkSupportedType, NULL);
		// 	}
		// 	//assert(hr == S_OK);
		// 	if (hr == S_OK) {
		// 		std::cout << "Matching media type found." << std::endl;
		// 		//std::cout << GetMediaTypeDescription(pSinkSupportedType) << std::endl;
		// 		hr = MFCreateSinkWriterFromMediaSink(pMediaSink, NULL, &m_pAudioSinkWriter);
		// 		assert(hr== S_OK);
		// 		hr = m_pAudioSinkWriter->SetInputMediaType(0, pSinkSupportedType, NULL);
		// 		assert(hr == S_OK);
		// 		hr = m_pAudioSinkWriter->BeginWriting();
		// 		assert(hr == S_OK);
		// 		return hr;
		// 	}
		// }

		// Find a media type that the stream sink supports.		
		DWORD sinkMediaTypeCount = 0;
		hr = pSinkMediaTypeHandler->GetMediaTypeCount(&sinkMediaTypeCount);
        assert(hr == S_OK);
        std::cout << "Supported media type count: "<< sinkMediaTypeCount << std::endl;
		for (UINT i = 0; i < sinkMediaTypeCount; i++)
		{
			CHECK_HR(pSinkMediaTypeHandler->GetMediaTypeByIndex(i, &pSinkSupportedType),
				"Error getting media type from sink media type handler.");

			if (pSinkMediaTypeHandler->IsMediaTypeSupported(pSinkSupportedType, NULL) == S_OK)
			{
				std::cout << "Matching media type found." << std::endl;
				std::cout << GetMediaTypeDescription(pSinkSupportedType) << std::endl;
				break;
			}
			else {
				std::cout << "Sink and source media type incompatible." << std::endl;
				std::cout << GetMediaTypeDescription(pSinkSupportedType) << std::endl;
				SAFE_RELEASE(pSinkSupportedType);
			}
		}
		if (pSinkSupportedType) {
			hr = MFCreateSinkWriterFromMediaSink(pMediaSink, NULL, ppAudioSinkWriter);
            assert(hr == S_OK);

            if (hr == S_OK) {
                hr = (*ppAudioSinkWriter)->SetInputMediaType(0, pSinkSupportedType, NULL);
                assert(hr == S_OK);
            }

            if (hr == S_OK) {
                hr = (*ppAudioSinkWriter)->BeginWriting();
                assert(hr == S_OK);

                *ppStreamSink = pStreamSink;
                *ppSinkMediaTypeHandler = pSinkMediaTypeHandler;
                *ppSinkSupportedType = pSinkSupportedType;
            }
			
		}
	done:
		return hr;
    }

};