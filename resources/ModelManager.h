//
//  ModelManager.h
//  MiRay/resources
//
//  Created by Damir Sagidullin on 03.04.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//
#pragma once

#include "Model.h"

namespace mr
{

class ImageManager;

class ModelManager : public IResourceManager
{
	ImageManager	*m_pImageManager;
	FbxManager		*m_pSdkManager;
	FbxImporter		*m_pImporter;

	void CollectMeshes(Model & model, FbxNode * pFbxNode, FbxAnimLayer * pFbxAnimLayer, FbxTime & time);
	void AddMesh(Model & model, FbxNode * pFbxNode, FbxAMatrix & pGlobalPosition,
				 const FbxVector4 * pControlPoints, const FbxVector4 * pNormalArray);

	void ComputeDualQuaternionDeformation(FbxAMatrix& pGlobalPosition, FbxMesh* pFbxMesh,
										  FbxTime & pTime, FbxVector4* pVertexArray, FbxPose* pPose);
	void ComputeSkinDeformation(FbxAMatrix & pGlobalPosition, FbxMesh * pFbxMesh, FbxTime & pTime,
								FbxVector4 * pVertexArray, FbxPose * pPose);
	void AddAnimatedMesh(Model & model, FbxNode * pFbxNode, FbxAnimLayer * pFbxAnimLayer,
						 FbxTime & pTime, FbxAMatrix & pGlobalPosition, FbxPose * pPose);
	void ComputeLinearDeformation(FbxAMatrix& pGlobalPosition, FbxMesh* pFbxMesh, FbxTime & pTime,
								  FbxVector4* pVertexArray, FbxPose* pPose);
	void ComputeClusterDeformation(FbxAMatrix & pGlobalPosition, FbxMesh * pFbxMesh, FbxCluster * pCluster,
								   FbxAMatrix & pVertexTransformMatrix, FbxTime & pTime, FbxPose * pPose);

public:
	ModelManager(ImageManager * pImageManager);
	~ModelManager();

	Model * LoadModel(const char * strFilename, pugi::xml_node node);
};

}