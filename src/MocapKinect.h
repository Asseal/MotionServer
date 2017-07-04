#pragma once
#include "Config.h"

#ifdef USE_KINECT

#pragma comment(lib, "Kinect10.lib")

#include <Windows.h>
#include "MoCapSystem.h"
#include "NuiApi.h"

class MoCapKinect : public MoCapSystem
{
public:
		MoCapKinect();
		virtual ~MoCapKinect();
	
public:
		virtual bool  initialise();
		virtual bool  isActive();
		virtual float getUpdateRate();
		virtual bool  isRunning();
		virtual void  setRunning(bool running);
		virtual bool  update();
		virtual bool  getSceneDescription(MoCapData& refData);
		virtual bool  getFrameData(MoCapData& refData);
		virtual bool  processCommand(const std::string& strCommand);
		virtual bool  deinitialise();

private:
		HRESULT findKinectSensor();

private:
		bool         initialised;
		bool         isPlaying;
		std::string  strCortexAddress;
		std::string  strLocalAddress;
		INuiSensor *pNuiSensor;
		HANDLE kinectHandel;
};

#endif
