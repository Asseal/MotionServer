#pragma once
#include "Config.h"

#ifdef USE_KINECT

#pragma comment(lib, "Kinect10.lib")

#include <Windows.h>
#include "MoCapSystem.h"
#include "NuiApi.h"

#include "VectorMath.h"

#include <vector>

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
		bool initialiseKinect();
		void SkeletonFrameReady(NUI_SKELETON_FRAME *pSkeletonframe, sMarkerSetData* msData);
		void GetSkeleton(const NUI_SKELETON_DATA &skeleton, sMarkerSetData* msData);

private:
		bool         initialised;
		bool         isPlaying;
		std::string  strKinectAddress;
		std::string  strLocalAddress;
		INuiSensor *pNuiSensor;
		HANDLE kinectHandel;

		int                     iFrame;
		float                   fTime;
		std::vector<Vector3D>   arrPos;
		std::vector<Quaternion> arrRot;

		bool                    trackingUnreliable;
		std::vector<int>        arrTrackingLostCounter;

};

#endif
