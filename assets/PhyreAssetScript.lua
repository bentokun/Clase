-- SCE CONFIDENTIAL
-- PhyreEngine(TM) Package 3.7.0.0
-- Copyright (C) 2013 Sony Computer Entertainment Inc.
-- All Rights Reserved.

function ProcessAnimationSet(animSet)

  local tol = GetParameter('animTolerance')

  if(tol ~= nil) then
    local clips = animSet.m_animationClips
    for key, value in pairs(clips)
    do
      ret = value:Optimize(tol)
    end
  end
end

function ProcessTextureRemoveMips2D(texture)
  local maxTextureSize = GetParameter('maxTextureSize')
  if(maxTextureSize ~= nil) then
     ret = texture:RemoveMips2D(maxTextureSize)
  end
end

function ProcessTextureRemoveMipsCubeMap(texture)
  local maxTextureSize = GetParameter('maxTextureSize')
  if(maxTextureSize ~= nil) then
     ret = texture:RemoveMipsCubeMap(maxTextureSize)
  end
end

function ProcessTexture(texture)
  local targetFormat = GetParameter('textureTargetFormat')
  if(targetFormat ~= nil) then
    ret = texture:setTargetFormatName(targetFormat)
  end
end

function ProcessSampler(sampler)
  local aniso = GetParameter('aniso')
  if(aniso ~= nil) then
    ret = sampler:setMaxAnisotropy(aniso)
  end
end

function ProcessCluster(cluster, platform)

  if (Phyre.PPhysicsWorld ~= nil) then
  	cluster:deleteInstancesOfType(Phyre.PPhysicsWorld)
	cluster:CollapseRigidBodyTargetNodeHierarchies()
  end

  local assetList = { GetClusterObjectsOfType(cluster, 'PAssetReference') }

  local anim_set_name = Phyre.PAnimationSet
  for key,value in pairs(assetList)
  do
    local name = value:getAssetType()
    if name:isTypeOf(anim_set_name) then
      ProcessAnimationSet(value:getAsset())
    end
  end

  -- Set target texture formats from params
  local texture2DList = { GetClusterObjectsOfType(cluster, 'PTexture2D') }
  for key,value in pairs(texture2DList)
  do
    ProcessTexture(value)
    ProcessTextureRemoveMips2D(value)
  end

  local texture3DList = { GetClusterObjectsOfType(cluster, 'PTexture3D') }
  for key,value in pairs(texture3DList)
  do
    ProcessTexture(value)
  end

  local textureCubeMapList = { GetClusterObjectsOfType(cluster, 'PTextureCubeMap') }
  for key,value in pairs(textureCubeMapList)
  do
    ProcessTexture(value)
    ProcessTextureRemoveMipsCubeMap(value)
  end

   -- Set target texture formats from params
  local samplerStateList = { GetClusterObjectsOfType(cluster, 'PSamplerState') }
  for key,value in pairs(samplerStateList)
  do
    ProcessSampler(value)
  end

  cluster:RemoveUnusedVertexData()

  return 0

end
