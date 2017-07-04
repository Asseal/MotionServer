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
};

const SKELETON_POSITIONS SKELETON_POSTIONS_PARAM[]{
	{"HEAD", NUI_SKELETON_POSITION_HEAD}, {"SHOULDER_CENTER",NUI_SKELETON_POSITION_SHOULDER_CENTER}, 
	{"SHOULDER_RIGHT", NUI_SKELETON_POSITION_SHOULDER_RIGHT}, {"ELBOW_RIGHT", NUI_SKELETON_POSITION_ELBOW_RIGHT},
	{"WRIST_RIGHT", NUI_SKELETON_POSITION_WRIST_RIGHT}, {"HAND_RIGHT", NUI_SKELETON_POSITION_HAND_RIGHT},
	{"SHOULDER_LEFT", NUI_SKELETON_POSITION_SHOULDER_LEFT}, {"ELBOW_LEFT", NUI_SKELETON_POSITION_ELBOW_LEFT},
	{"WRIST_LEFT",NUI_SKELETON_POSITION_WRIST_LEFT}, {"HAND_LEFT", NUI_SKELETON_POSITION_HAND_LEFT},
	{"SPINE", NUI_SKELETON_POSITION_SPINE}, {"HIP_CENTER", NUI_SKELETON_POSITION_HIP_CENTER}, 
	{"HIP_RIGHT", NUI_SKELETON_POSITION_HIP_RIGHT}, {"HIP_LEFT", NUI_SKELETON_POSITION_HIP_LEFT}, 
	{"KNEE_RIGHT", NUI_SKELETON_POSITION_KNEE_RIGHT}, {"KNEE_LEFT", NUI_SKELETON_POSITION_KNEE_LEFT}, 
	{"ANKLE_RIGHT", NUI_SKELETON_POSITION_ANKLE_RIGHT}, {"ANKLE_LEFT", NUI_SKELETON_POSITION_ANKLE_LEFT}, 
	{"FOOT_RIGHT", NUI_SKELETON_POSITION_FOOT_RIGHT}, {"FOOT_LEFT",NUI_SKELETON_POSITION_FOOT_LEFT},
};

const int SKELETON_POSITION_INDEX_COUNT = sizeof(SKELETON_POSTIONS_PARAM)/2;

MoCapKinect::MoCapKinect():
	initialised(false),
	isPlaying(true)
{
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
		LOG_INFO ("Cannot open kinect.");
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
	return 0;
}

bool  MoCapKinect::getFrameData(MoCapData& refData) {
	LOG_INFO("FRAME DATA")

		// update marker data
		sMarkerSetData& msData = refData.frame.MocapData[b];
		for (int m = 0; m < msData.nMarkers; m++)
		{
			msData.Markers[m][0] = arrPos[b].x + (rand() * 0.1f / RAND_MAX - 0.05f);
			msData.Markers[m][1] = arrPos[b].y + (rand() * 0.1f / RAND_MAX - 0.05f);
			msData.Markers[m][2] = arrPos[b].z + (rand() * 0.1f / RAND_MAX - 0.05f);
		}

		// update rigid body data
		sRigidBodyData& rbData = refData.frame.RigidBodies[b];
		rbData.ID = b;

		rbData.nMarkers = 0;
		rbData.MeanError = 0;

	return true;
}

bool  MoCapKinect::processCommand(const std::string& strCommand) {
	return false;
}

bool MoCapKinect::isRunning()
{
	return isPlaying;
}

void MoCapKinect::setRunning(bool running)
{
}


bool MoCapKinect::update()
{

	//Not implemented
	if (isPlaying)
	{
	}
		
	return false;
}


bool MoCapKinect::getSceneDescription(MoCapData& refData)
{
	LOG_INFO("Requesting scene description")

	int descrIdx = 0;
	
		// create markerset description and frame
		sMarkerSetDescription* pMarkerDesc = new sMarkerSetDescription();
		sMarkerSetData&        msData = refData.frame.MocapData[0];

		// name of marker set
		strcpy_s(pMarkerDesc->szName, sizeof(pMarkerDesc->szName), "user 1");
		strcpy_s(msData.szName, sizeof(msData.szName), pMarkerDesc->szName);

		// number of markers
		pMarkerDesc->nMarkers = SKELETON_POSITION_INDEX_COUNT;
		msData.nMarkers = SKELETON_POSITION_INDEX_COUNT;

		pMarkerDesc->szMarkerNames = new char*[SKELETON_POSITION_INDEX_COUNT];
		msData.Markers = new MarkerData[SKELETON_POSITION_INDEX_COUNT];
		for (int m = 0; m < SKELETON_POSITION_INDEX_COUNT; m++)
		{
			pMarkerDesc->szMarkerNames[m] = _strdup(SKELETON_POSTIONS_PARAM[m].PositionName);
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
	if (initialised){
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