#include <string.h>

#include "Logging.h"
#include "Converter.h"
#include "VectorMath.h"


#define MAX_UNKNOWN_MARKERS 256 // maximum number of unknown markers
#define HANDLE_SKELETON_DATA

float unitScaleFactor      = 1.0f;
bool  handleUnknownMarkers = false;

/**
 * Gets the factor which is used to scale all positions.
 */
float getUnitScaleFactor()
{
	return unitScaleFactor;
}


/**
 * Sets the factor which is used to scale all positions.
 */
void setUnitScaleFactor(float factor)
{
	unitScaleFactor = factor;
}


/**
 * Checks if the converter handles unknown marker data.
 */
bool isHandlingUnknownMarkers()
{
	return handleUnknownMarkers;
}


/**
 * Enables or disables handling of unknown marker data.
 */
void setHandleUnknownMarkers(bool enable)
{
	handleUnknownMarkers = enable;
}


/**
* Converts the scene description from Cortex to NatNet.
*/
void convertCortexDescriptionToNatNet(sBodyDefs& refCortex, sDataDescriptions& refDescr, sFrameOfMocapData& refFrame)
{
	int idxDataBlock = 0;
	int idxMarkerSet = 0;
	int idxRigidBody = 0;
	int idxSkeleton  = 0;

	for (int iBodyIdx = 0; iBodyIdx < refCortex.nBodyDefs; iBodyIdx++)
	{
		sBodyDef& bodyDef = refCortex.BodyDefs[iBodyIdx];

		// create markerset description and markerset data
		sMarkerSetDescription* pMarkerSetDescr = new sMarkerSetDescription;
		sMarkerSetData&        refMarkerSetData = refFrame.MocapData[idxMarkerSet];
		strncpy_s(pMarkerSetDescr->szName, bodyDef.szName, sizeof(pMarkerSetDescr->szName)); // markerset name
		strncpy_s(refMarkerSetData.szName, bodyDef.szName, sizeof(refMarkerSetData.szName));
		int nMarkers = bodyDef.nMarkers;
		pMarkerSetDescr->nMarkers = nMarkers; // number of markers
		refMarkerSetData.nMarkers = nMarkers;
		pMarkerSetDescr->szMarkerNames = new char*[nMarkers]; // array of marker names
		refMarkerSetData.Markers = new MarkerData[nMarkers];  // array of marker data
		for (int mIdx = 0; mIdx < bodyDef.nMarkers; mIdx++)
		{
			pMarkerSetDescr->szMarkerNames[mIdx] = _strdup(bodyDef.szMarkerNames[mIdx]); // marker names
		}
		refDescr.arrDataDescriptions[idxDataBlock].type = Descriptor_MarkerSet;
		refDescr.arrDataDescriptions[idxDataBlock].Data.MarkerSetDescription = pMarkerSetDescr;
		idxDataBlock++;
		idxMarkerSet++;

		sHierarchy& refSkeleton = bodyDef.Hierarchy;
		if (refSkeleton.nSegments == 1)
		{
			sHierarchy& refHierarchy = bodyDef.Hierarchy;

			// one bone skeleton -> treat as rigid body
			// create rigid body description
			sRigidBodyDescription* pRigidBodyDescr  = new sRigidBodyDescription;
			strncpy_s(pRigidBodyDescr->szName, refHierarchy.szSegmentNames[0], sizeof(pRigidBodyDescr->szName)); // rigid body name
			pRigidBodyDescr->ID = iBodyIdx; // rigid body ID = actor ID
			pRigidBodyDescr->parentID = -1; // no parent
			pRigidBodyDescr->offsetx = 0;   // the offset does not exist in Cortex data
			pRigidBodyDescr->offsety = 0;
			pRigidBodyDescr->offsetz = 0;

			// pre-fill rigid body frame data structure
			sRigidBodyData& refRigidBodyData = refFrame.RigidBodies[idxRigidBody];
			refRigidBodyData.ID = iBodyIdx;
			refRigidBodyData.nMarkers    = 0;
			refRigidBodyData.Markers     = NULL;
			refRigidBodyData.MarkerIDs   = NULL;
			refRigidBodyData.MarkerSizes = NULL;
			refRigidBodyData.MeanError   = 0;

			// add to scene description
			refDescr.arrDataDescriptions[idxDataBlock].type = Descriptor_RigidBody;
			refDescr.arrDataDescriptions[idxDataBlock].Data.RigidBodyDescription = pRigidBodyDescr;
			idxDataBlock++;
			idxRigidBody++;
		}
#ifdef HANDLE_SKELETON_DATA
		else if (refSkeleton.nSegments > 0)
		{
			// skeleton data included as well
			// create skeleton description and skeleton data
			sSkeletonDescription* pSkeletonDescr = new sSkeletonDescription;
			sSkeletonData&        refSkeletonData = refFrame.Skeletons[idxSkeleton];		
			strncpy_s(pSkeletonDescr->szName, bodyDef.szName, sizeof(pSkeletonDescr->szName)); // markerset name = skeleton name
			pSkeletonDescr->skeletonID = iBodyIdx; // skeleton ID
			refSkeletonData.skeletonID = iBodyIdx;
			int nSegments = refSkeleton.nSegments;
			pSkeletonDescr->nRigidBodies = nSegments; // number of segments
			refSkeletonData.nRigidBodies = nSegments;
			refSkeletonData.RigidBodyData = new sRigidBodyData[nSegments];  // array of skeleton data
			for (int sIdx = 0; sIdx < nSegments; sIdx++)
			{
				// create skeleton segment description
				sRigidBodyDescription& refRigidBodyDescr = pSkeletonDescr->RigidBodies[sIdx];
				strncpy_s(refRigidBodyDescr.szName, refSkeleton.szSegmentNames[sIdx], sizeof(refRigidBodyDescr.szName)); // segment name
				refRigidBodyDescr.ID       = sIdx; // segment ID
				refRigidBodyDescr.parentID = refSkeleton.iParents[sIdx]; // segment parent
				refRigidBodyDescr.offsetx  = 0;   // the offset does not exist in Cortex data
				refRigidBodyDescr.offsety  = 0;
				refRigidBodyDescr.offsetz  = 0;

				// pre-fill skeleton segment frame data structure
				sRigidBodyData&  refRigidBodyData = refSkeletonData.RigidBodyData[sIdx];
				refRigidBodyData.ID          = sIdx;
				refRigidBodyData.nMarkers    = 0;
				refRigidBodyData.Markers     = NULL;
				refRigidBodyData.MarkerIDs   = NULL;
				refRigidBodyData.MarkerSizes = NULL;
				refRigidBodyData.MeanError   = 0;
			}
			// add to description
			refDescr.arrDataDescriptions[idxDataBlock].type = Descriptor_Skeleton;
			refDescr.arrDataDescriptions[idxDataBlock].Data.SkeletonDescription = pSkeletonDescr;
			idxDataBlock++;
			idxSkeleton++;
		}
#endif
	}
	// store amount of data blocks
	refDescr.nDataDescriptions = idxDataBlock;
	
	// prepare amount of items in frame data
	refFrame.nMarkerSets      = idxMarkerSet;
	refFrame.nOtherMarkers    = 0;
	refFrame.OtherMarkers     = new MarkerData[MAX_UNKNOWN_MARKERS];
	refFrame.nRigidBodies     = idxRigidBody;
	refFrame.nSkeletons       = idxSkeleton;
	refFrame.nLabeledMarkers  = 0;
	refFrame.nForcePlates     = 0;
	refFrame.Timecode         = 0;
	refFrame.TimecodeSubframe = 0;
}


/**
 * Converts frame data from Cortex to NatNet.
 */

void _convertCortexMarkerToNatNet(tMarkerData& refCortex, MarkerData& refNatNet)
{
	if (refCortex[0] < XEMPTY)
	{
		refNatNet[0] = refCortex[0] * unitScaleFactor; // X
		refNatNet[1] = refCortex[1] * unitScaleFactor; // Y 
		refNatNet[2] = refCortex[2] * unitScaleFactor; // Z
	}
	else
	{
		// marker has vanished -> set position to exactly 0
		refNatNet[0] = 0;
		refNatNet[1] = 0;
		refNatNet[2] = 0;
	}
}


void _convertCortexMarkerSetToNatNet(sBodyData& refCortex, sMarkerSetData& refNatNet)
{
	if (refCortex.nMarkers != refNatNet.nMarkers)
	{
		LOG_ERROR("Mismatch in marker count");
		return;
	}

	for (int mIdx = 0; mIdx < refCortex.nMarkers; mIdx++)
	{
		_convertCortexMarkerToNatNet(refCortex.Markers[mIdx], refNatNet.Markers[mIdx]);
	}
}


void _convertCortexSegmentToNatNet(double refCortex[], sRigidBodyData& refNatNet)
{
	Vector3D   pos;
	Quaternion rot;

	if (refCortex[0] < XEMPTY) // check for valid data
	{
		pos.x = (float)refCortex[0] * unitScaleFactor;
		pos.y = (float)refCortex[1] * unitScaleFactor;
		pos.z = (float)refCortex[2] * unitScaleFactor;

		// convert Euler rotation from Cortex (default: ZYX) to Quaternion
		Quaternion rotX(1, 0, 0, (float)RADIANS(refCortex[3])); // rotX
		Quaternion rotY(0, 1, 0, (float)RADIANS(refCortex[4])); // rotY
		Quaternion rotZ(0, 0, 1, (float)RADIANS(refCortex[5])); // rotZ
		rot.mult(rotZ).mult(rotY).mult(rotX);
	}
	else
	{
		// segment data not available -> use origin and neutral pose
	}

	refNatNet.x = pos.x;
	refNatNet.y = pos.y;
	refNatNet.z = pos.z;

	refNatNet.qx = rot.x;
	refNatNet.qy = rot.y;
	refNatNet.qz = rot.z;
	refNatNet.qw = rot.w;
}


void _convertCortexSegmentsToNatNet(sBodyData& refCortex, sSkeletonData& refNatNet)
{
	if (refCortex.nSegments != refNatNet.nRigidBodies)
	{
		LOG_ERROR("Mismatch in segment count");
		return;
	}

	for (int sIdx = 0; sIdx < refCortex.nSegments; sIdx++)
	{
		_convertCortexSegmentToNatNet(refCortex.Segments[sIdx], refNatNet.RigidBodyData[sIdx]);
	}
}


bool convertCortexFrameToNatNet(sFrameOfData& refCortex, sFrameOfMocapData& refNatNet)
{
	refNatNet.iFrame   = refCortex.iFrame;
	refNatNet.fLatency = refCortex.fDelay;

	if (refCortex.nBodies != refNatNet.nMarkerSets)
	{
		LOG_ERROR("Mismatch in actor count");
		return false;
	}

	// copy marker data per actor
	for (int mIdx = 0; mIdx < refCortex.nBodies; mIdx++)
	{
		_convertCortexMarkerSetToNatNet(refCortex.BodyData[mIdx], refNatNet.MocapData[mIdx]);
	}

	// copy rigid body data
	for (int rIdx = 0; rIdx < refNatNet.nRigidBodies; rIdx++)
	{
		int sourceIdx = refNatNet.RigidBodies[rIdx].ID;
		_convertCortexSegmentToNatNet(refCortex.BodyData[sourceIdx].Segments[0], refNatNet.RigidBodies[rIdx]);
	}

	if (handleUnknownMarkers)
	{
		// copy unidentified marker data
		refNatNet.nOtherMarkers = refCortex.nUnidentifiedMarkers;
		for (int mIdx = 0; mIdx < refNatNet.nOtherMarkers; mIdx++)
		{
			_convertCortexMarkerToNatNet(refCortex.UnidentifiedMarkers[mIdx], refNatNet.OtherMarkers[mIdx]);
		}
	}
	else
	{
		refNatNet.OtherMarkers = 0;
	}

#ifdef HANDLE_SKELETON_DATA
	// copy skeleton data
	for (int sIdx = 0; sIdx < refNatNet.nSkeletons; sIdx++)
	{
		int sourceIdx = refNatNet.Skeletons[sIdx].skeletonID;
		_convertCortexSegmentsToNatNet(refCortex.BodyData[sourceIdx], refNatNet.Skeletons[sIdx]);
	}
#endif

	return true;
}


