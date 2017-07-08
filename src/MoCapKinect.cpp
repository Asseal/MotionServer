#include "MocapKinect.h"

#ifdef USE_KINECT

#include "Logging.h"
#undef   LOG_CLASS
#define  LOG_CLASS "MoCapKinect"

#include "VectorMath.h"

#include <algorithm>
#include <iterator>
#include <string>

struct SKELETON_POSITIONS {
	char* PositionName;
	NUI_SKELETON_POSITION_INDEX index;
	float x;
	float y;
	float z;
};

const SKELETON_POSITIONS SKELETON_POSTIONS_PARAM[]{
	{ "HEAD", NUI_SKELETON_POSITION_HEAD },{ "SHOULDER_CENTER",NUI_SKELETON_POSITION_SHOULDER_CENTER },
	{ "SHOULDER_RIGHT", NUI_SKELETON_POSITION_SHOULDER_RIGHT },{ "ELBOW_RIGHT", NUI_SKELETON_POSITION_ELBOW_RIGHT },
	{ "WRIST_RIGHT", NUI_SKELETON_POSITION_WRIST_RIGHT },{ "HAND_RIGHT", NUI_SKELETON_POSITION_HAND_RIGHT },
	{ "SHOULDER_LEFT", NUI_SKELETON_POSITION_SHOULDER_LEFT },{ "ELBOW_LEFT", NUI_SKELETON_POSITION_ELBOW_LEFT },
	{ "WRIST_LEFT",NUI_SKELETON_POSITION_WRIST_LEFT },{ "HAND_LEFT", NUI_SKELETON_POSITION_HAND_LEFT },
	{ "SPINE", NUI_SKELETON_POSITION_SPINE },{ "HIP_CENTER", NUI_SKELETON_POSITION_HIP_CENTER },
	{ "HIP_RIGHT", NUI_SKELETON_POSITION_HIP_RIGHT },{ "HIP_LEFT", NUI_SKELETON_POSITION_HIP_LEFT },
	{ "KNEE_RIGHT", NUI_SKELETON_POSITION_KNEE_RIGHT },{ "KNEE_LEFT", NUI_SKELETON_POSITION_KNEE_LEFT },
	{ "ANKLE_RIGHT", NUI_SKELETON_POSITION_ANKLE_RIGHT },{ "ANKLE_LEFT", NUI_SKELETON_POSITION_ANKLE_LEFT },
	{ "FOOT_RIGHT", NUI_SKELETON_POSITION_FOOT_RIGHT },{ "FOOT_LEFT",NUI_SKELETON_POSITION_FOOT_LEFT },
};

const int SKELETON_POSITION_INDEX_COUNT = sizeof(SKELETON_POSTIONS_PARAM) / 2;

MoCapKinect::MoCapKinect() :
	initialised(false),
	isPlaying(true)
{
	this->strKinectAddress = strKinectAddress;
}

bool MoCapKinect::initialise() {
	initialised = true;

	LOG_INFO("Kinect SDK version v 1.8");

	HRESULT result;
	result = findKinectSensor();


	if (FAILED(result)) {
		LOG_INFO("Cannot find connected kinect.");
		initialised = false;
		return initialised;
	}

	LOG_INFO("Connected to Kinect server ");

	result = pNuiSensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_SKELETON);

	if (FAILED(result)) {
		LOG_INFO("Cannot open kinect.");
		deinitialise();
		return initialised;
	}

	kinectHandel = CreateEventW(NULL, TRUE, FALSE, NULL);
	result = pNuiSensor->NuiSkeletonTrackingEnable(kinectHandel, NUI_SKELETON_TRACKING_FLAG_ENABLE_SEATED_SUPPORT);

	if (FAILED(result)) {
		LOG_INFO("Cannot enable skeleton tracking.");
		deinitialise();
		return initialised;
	}

	LOG_INFO("Kinect Initialized.");
	return initialised;
}

bool MoCapKinect::isActive()
{
	return initialised;
}


float MoCapKinect::getUpdateRate()
{
	return 60;
}

bool  MoCapKinect::getFrameData(MoCapData& refData) {
	// update marker data
	sMarkerSetData& msData = refData.frame.MocapData[0];

	if (WAIT_OBJECT_0 == WaitForSingleObject(kinectHandel, 0)) {
		NUI_SKELETON_FRAME skeletonFrame = { 0 };

		if (SUCCEEDED(pNuiSensor->NuiSkeletonGetNextFrame(0, &skeletonFrame)))
		{
			SkeletonFrameReady(&skeletonFrame, &msData);
		}
	}

	Sleep(1000/120);

	return true;
}

bool  MoCapKinect::processCommand(const std::string& strCommand) {
	return true;
}

bool MoCapKinect::isRunning()
{
	return isPlaying;
}

void MoCapKinect::setRunning(bool running)
{
	isPlaying = running;
}


bool MoCapKinect::update()
{
	//update is done by kinect itself, no menu update required	
	signalNewFrame();
	return true;
}

void MoCapKinect::SkeletonFrameReady(NUI_SKELETON_FRAME *pSkeletonFrame, sMarkerSetData* msData)
{
	for (int i = 0; i < NUI_SKELETON_COUNT; i++)
	{
		const NUI_SKELETON_DATA &skeleton = pSkeletonFrame->SkeletonData[i];

		switch (skeleton.eTrackingState)
		{
		case NUI_SKELETON_TRACKED:
			GetSkeleton(skeleton, msData);
			break;
		case NUI_SKELETON_POSITION_ONLY:
			GetSkeleton(skeleton, msData);
			break;
		}
	}
}

void MoCapKinect::GetSkeleton(const NUI_SKELETON_DATA &skeleton, sMarkerSetData *msData)
{
	//sample to show the position
	for (int m = 0; m < msData->nMarkers; m++)
	{
		const Vector4 &point = skeleton.SkeletonPositions[SKELETON_POSTIONS_PARAM[m].index];

		std::cout << "x" << point.x << "y" << point.y << "z" << point.z << "\n" << std::endl;
		msData->Markers[m][0] = point.x;
		msData->Markers[m][1] = point.y;
		msData->Markers[m][2] = point.z;
	}
}

bool MoCapKinect::getSceneDescription(MoCapData& refData)
{
	int descrIdx = 0;

	// create markerset description and frame
	sMarkerSetDescription* pMarkerDesc = new sMarkerSetDescription();
	sMarkerSetData&        msData = refData.frame.MocapData[0];

	// name of marker set
	strcpy_s(pMarkerDesc->szName, sizeof(pMarkerDesc->szName), "user 1");
	strcpy_s(msData.szName, sizeof(msData.szName), pMarkerDesc->szName);

	// number of markers
	pMarkerDesc->nMarkers = 20;
	msData.nMarkers = 20;

	pMarkerDesc->szMarkerNames = new char*[20];
	msData.Markers = new MarkerData[20];


	for (int m = 0; m < 20; m++)
	{
		pMarkerDesc->szMarkerNames[m] = SKELETON_POSTIONS_PARAM[m].PositionName;
	}


	// add to description list
	refData.description.arrDataDescriptions[descrIdx].type = Descriptor_MarkerSet;
	refData.description.arrDataDescriptions[descrIdx].Data.MarkerSetDescription = pMarkerDesc;
	descrIdx++;

	sRigidBodyDescription* pBodyDesc = new sRigidBodyDescription();
	// fill in description structure
	pBodyDesc->ID = 0; // needs to be equal to array index
	pBodyDesc->parentID = -1;
	pBodyDesc->offsetx = 0;
	pBodyDesc->offsety = 0;
	pBodyDesc->offsetz = 0;
	strcpy_s(pBodyDesc->szName, sizeof(pBodyDesc->szName), pMarkerDesc->szName);

	refData.description.arrDataDescriptions[descrIdx].type = Descriptor_RigidBody;
	refData.description.arrDataDescriptions[descrIdx].Data.RigidBodyDescription = pBodyDesc;
	descrIdx++;

	refData.description.nDataDescriptions = descrIdx;

	// pre-fill in frame data
	refData.frame.nMarkerSets = 1;
	refData.frame.nRigidBodies = 1;
	refData.frame.nSkeletons = 0;

	refData.frame.nOtherMarkers = 0;
	refData.frame.OtherMarkers = NULL;

	refData.frame.nLabeledMarkers = 0;

	refData.frame.nForcePlates = 0;

	refData.frame.fLatency = 0.01f; // simulate 10ms
	refData.frame.Timecode = 0;
	refData.frame.TimecodeSubframe = 0;

	return true;
}

bool MoCapKinect::deinitialise() {
	if (initialised) {
		NuiShutdown();
		initialised = false;
	}
	return !initialised;
}

MoCapKinect::~MoCapKinect()
{
	deinitialise();
}

HRESULT	MoCapKinect::findKinectSensor()
{
	return NuiCreateSensorByIndex(0, &pNuiSensor);
}

#endif
