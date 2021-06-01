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

Author:...... .Jordan Hueckstaedt
Contact:.......SpazzMonkeys@gmail.com
Website:.......RubberGuppy.com
Last Modified:.7/6/2010
Version:.......0.1

Description:
	A polygon deformer which averages verticies on the selected mesh.
	
Todo:
	I may not do any of these, but I have these in mind so far:
	Make threaded.
	A volume preservation scheme which acts like Blender's Shrinkwrap + 
	Smooth modifier.
*/

#include "smoothMesh.h"
#include <maya/MFnPlugin.h>

//Smooth Node member attributes
MString smoothMesh::name("smoothMesh");
MTypeId smoothMesh::id(0x91919191);			//Not obtained from Autodesk.  Warning Will Robinson, Warning!
MObject smoothMesh::laplace;
MObject smoothMesh::iterations;
MObject smoothMesh::operation;
MObject smoothMesh::volume;
MObject smoothMesh::offset;

//Smooth Node
MStatus smoothMesh::deform( MDataBlock& data, MItGeometry& geomIter, const MMatrix &localToWorldMatrix, unsigned int mIndex)
{
    MStatus status = MS::kUnknownParameter;
	//get attributes
	int operationV = data.inputValue(operation, &status ).asShort();
	int iterationsV = data.inputValue(iterations, &status ).asInt();
	float laplaceV = data.inputValue(laplace, &status ).asFloat();
	float volumeV = data.inputValue(volume, &status ).asFloat();
	float envelopeV = data.inputValue(envelope, &status ).asFloat();
	float offsetV = data.inputValue(offset, &status).asFloat();

	//Get points using MItGeometry this time
	MPointArray allPoints;
	geomIter.allPositions(allPoints);

	//Efficiency...
	if (envelopeV * laplaceV > 0) {
		
		//declare values for later
		int index;

		//Copy all points to averages.
		MPointArray avgPoints;
		avgPoints.copy(allPoints);	
		
		//Set dirty bit if something drastic happened
		if (allPoints.length() != nearVerts.size()) {
			updateNeighbors = 1;
		}
		
		//If we have new mesh connected recompute the nearby verts of every vert.
		if (updateNeighbors){
			// get the input geometry and mIndex for iterators and points
			MArrayDataHandle inputArray = data.inputArrayValue(input);
			inputArray.jumpToElement(mIndex);
			MDataHandle inputData = inputArray.inputValue();
			MDataHandle inputGeomDataH = inputData.child(inputGeom);
			MObject inputMesh = inputGeomDataH.asMesh();
			
			MItMeshVertex vertIter(inputMesh);	//Might add option to test onBoundary later

			//clear array-like objects
			nearVerts.clear();
			
			//declare values before loop to be more efficient
			unsigned int x;
			MIntArray nearV;
			
			//Find nearby verts of every vert
			for ( ; !vertIter.isDone(); vertIter.next() ) {
				index = vertIter.index();
				vertIter.getConnectedVertices(nearV);
				for(x = 0; x < nearV.length(); x++) {
					if(nearVerts.size() < index + 1){
						std::set<int> tmpSet;
						nearVerts.push_back(tmpSet);
					}
					nearVerts.at(index).insert(nearV[x]);
				}

				//nearVerts is a divisor later.  I've had this happen already.  It wasn't pretty.
				if (nearVerts.at(index).size() <=0){
					JHSTATUSERROR( "Divide by zero.  Vertex has no neighbors");
					return MS::kFailure;
				}
			}
			updateNeighbors = 0;
		}
		
		//Declare stuff for iterator loop
		MFloatArray weights;
		std::set <int>::iterator nearVertsIter;

		//Get weights
		geomIter.reset();
		for (; !geomIter.isDone(); geomIter.next()) {
			index = geomIter.index();
			weights.append(weightValue(data, mIndex, index) * envelopeV);
			if (weights[index] > 1)
				weights.set(1.0, index);
		}
		
		//Define Taubin stuff.
		int step;
		double factors[2];
		factors[0] = laplaceV;
		factors[1] = 1 / (volumeV - (1/laplaceV));
		
		//Taubin needs twice the iterations.
		if (factors[1] != 0 && operationV == 1)
			iterationsV *= 2;
		
		//Calculate the deformation
		for (int it = 0; it < iterationsV; it ++) {
			if (factors[1] != 0  && operationV == 1)
				step = it % 2;
			else
				step = 0;
			
			geomIter.reset();
			for ( ; !geomIter.isDone(); geomIter.next() ) {
				index = geomIter.index();
				
				//Skip deformation calculation if we don't need to.
				if (weights[index] > 0.01){							
					MVector avgPoint(0,0,0);	
					for ( nearVertsIter = nearVerts.at(index).begin() ; nearVertsIter != nearVerts.at(index).end(); nearVertsIter++)
						avgPoint += allPoints[*nearVertsIter];
					avgPoint /= nearVerts.at(index).size();
					avgPoint -= allPoints[index];
					avgPoint = allPoints[index] + (avgPoint * factors[step] * weights[index]);
					avgPoints.set(avgPoint, index);
				}
			}
			//Update original points to new averages.
			allPoints.copy(avgPoints);
		}

		//Apply the smooth deformation
		geomIter.setAllPositions(allPoints);

		//Offset surface first
		if (offsetV != 0) {
			MArrayDataHandle inputArray = data.inputArrayValue(input);
			inputArray.jumpToElement(mIndex);
			MDataHandle inputData = inputArray.inputValue();
			MDataHandle inputGeomDataH = inputData.child(inputGeom);
			MObject inputMesh = inputGeomDataH.asMesh();
			MItMeshVertex vertIter(inputMesh);

			for (; !vertIter.isDone(); vertIter.next()) {
				index = vertIter.index();
				if (weights[index] > 0) {
					MVector normal(0, 0, 0);
					vertIter.getNormal(normal);
					allPoints[index] += normal * offsetV * weights[index];
				}				
			}
			//Apply surface offset
			geomIter.setAllPositions(allPoints);
		}
	}
	
	//This was here from someone else.  Shouldn't this be...earlier in the code?  Like the beginning? 
	//Oh well, maybe always succeeding is a good thing.
	status = MS::kSuccess;
	
	return status;
}

//Set my own dirty bit if there's a new mesh.
MStatus smoothMesh::connectionMade( const MPlug& plug, const MPlug& otherPlug, bool asSrc )
{
        if ( plug == inputGeom ) {
			updateNeighbors = 1;
        }
        return MPxDeformerNode::connectionMade( plug, otherPlug, asSrc );
}


//Smooth Node attributes initialize
MStatus smoothMesh::initialize()
{
	MFnEnumAttribute eAttr;
	operation = eAttr.create( "operation", "op", 0 );
	eAttr.addField("Laplace", 0);
	eAttr.addField("Taubin", 1);
	eAttr.setKeyable( true );
	eAttr.setStorable( true );
	
	MFnNumericAttribute numAttr;
	iterations = numAttr.create( "iterations", "it", MFnNumericData::kInt, 1 );
	numAttr.setMin(0);
	numAttr.setKeyable( true );
	numAttr.setStorable( true );
	
	laplace = numAttr.create( "laplace", "lp", MFnNumericData::kFloat, 0.5 );
	numAttr.setMin(0);
	numAttr.setMax(1);
	numAttr.setKeyable( true );
	numAttr.setStorable( true );
	
	volume = numAttr.create( "volume", "vol", MFnNumericData::kFloat, 0.1	);
	numAttr.setMin(0);
	numAttr.setMax(1);
	numAttr.setKeyable( true );
	numAttr.setStorable( true );
	
	offset = numAttr.create("offset", "os", MFnNumericData::kFloat, 0.0);
	numAttr.setKeyable(true);
	numAttr.setStorable(true);

	addAttribute(operation);
	addAttribute(iterations);
	addAttribute(laplace);
	addAttribute(volume);
	addAttribute(offset);
	
	attributeAffects(operation, outputGeom);
	attributeAffects(iterations, outputGeom);
	attributeAffects(laplace, outputGeom);
	attributeAffects(volume, outputGeom);
	attributeAffects(offset, outputGeom);

	MGlobal::executeCommand( "makePaintable -attrType multiFloat -sm deformer smoothMesh weights;" );
	return MS::kSuccess;
}

//Standard node stuff
MStatus initializePlugin( MObject obj )
{
	MStatus status;
	MFnPlugin mplugin( obj, "Jordan Hueckstaedt", "0.1", "Any" );
	status = mplugin.registerNode( smoothMesh::name, smoothMesh::id, smoothMesh::creator, smoothMesh::initialize, MPxNode::kDeformerNode);
	JHCHECK_STATUS( status, "Can not initialize node." );
	return status;
}

MStatus uninitializePlugin ( MObject obj )
{
	MStatus status;
	MFnPlugin mplugin( obj );
	status = mplugin.deregisterNode( smoothMesh::id );
	JHCHECK_STATUS( status, "Can not initialize node." );
	return status;
}

void* smoothMesh::creator()
{
	return new smoothMesh;
}

