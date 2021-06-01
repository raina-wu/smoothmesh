/*************************************************************************
	Copyright 2010 Jordan Hueckstaedt
**************************************************************************
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// smoothMesh.h
#ifndef SMOOTHMESH_H
#define SMOOTHMESH_H
 
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPointArray.h>
#include <maya/MStatus.h>
#include <maya/MArrayDataHandle.h>	//Not strictly necessary.  I wonder what is?
#include <maya/MFloatVectorArray.h> //This one is!
#include <maya/MFloatArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MItGeometry.h>
#include <vector>
#include <set>

class smoothMesh : public MPxDeformerNode
{
public:
	smoothMesh() {};
	virtual MStatus deform( MDataBlock& data, MItGeometry& itGeo, const MMatrix &localToWorldMatrix, unsigned int mIndex );
	virtual MStatus connectionMade( const MPlug& plug, const MPlug& otherPlug, bool asSrc );
	//	virtual MStatus connectionBroken( const MPlug& plug, const MPlug& otherPlug, bool asSrc );

    static  void*   creator();
    static  MStatus initialize();
 
    static MTypeId      id;
	static MString		name;
	std::vector< std::set<int> > nearVerts;
	bool updateNeighbors;
	static MObject		iterations;
	static MObject		operation;
	static MObject      laplace;
	static MObject      volume;
	static MObject		offset;
};


//Define Macros
inline void mError(const MString &msg, const int &line)
{
	MGlobal::displayError(smoothMesh::name + " Error " + line + ": " + msg);
}

inline void pInfoHere(const int &line)
{
	MString msg = "Got to : ";
	msg += line;
	MGlobal::displayInfo(msg);
}

#define JHSTATUSERROR( msg ) \
	mError( msg, __LINE__);

	
#define JHCHECK_STATUS( stat, msg ) \
	if( !stat ) \
	{ \
	mError( msg, __LINE__); \
	}
	
#define PRINT_HERE( ) \
	pInfoHere( __LINE__);

#endif