/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Geometry/PhyreGeometry.h>
#include <Rendering/PhyreRendering.h>
#include <Sprite/PhyreSprite.h>
#include <Sprite/PhyreSprite.h>

#include "PhyreCommonGameSprite.h"

using namespace Phyre;
using namespace PSprite;
using namespace PRendering;
using namespace PGeometry;

namespace Phyre
{
	namespace PCommon
	{
		PHYRE_BIND_START(PGameSprite)
			PHYRE_ADD_MEMBER_FLAGS(DataMember, Phyre::PE_CLASS_DATA_MEMBER_HIDDEN)
		PHYRE_BIND_END

		float PGameSprite::s_height = 720.0f;
		float PGameSprite::s_width = 1280.0f;

		// Description:
		// Sets the screen width and height.
		//
		// Arguments:
		// width : The screen width.
		// height: The screen height.
		void PGameSprite::SetScreenWidthHeight(float width, float height)
		{
			s_height = height;
			s_width = width;
		}

		// Description:
		// Sets the velocity of the sprite.
		//
		// Arguments:
		// v0 : The velocity of the sprite in x direction.
		// v1 : The velocity of the sprite in y direction.
		void PGameSprite::setVelocity(float v0, float v1)
		{
			m_velocity[0] = v0;
			m_velocity[1] = v1;
		}

		// Description:
		// Animates the texture of the sprite.
		//
		// Arguments:
		// elapsedTime :	The elapsed time between frames.
		//
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR :     The operation failed because an error occurred while animating the sprite.
		// PE_RESULT_NO_ERROR :          The sprite was animated successfully.
		PResult PGameSprite::animate(float elapsedTime)
		{
			m_time += elapsedTime;

			// Advance the texture for the animation.
			if (m_animInfo && m_active && m_time >= m_timeInterval)
			{
				m_currentAnimationFrameIndex++;
				if (m_currentAnimationFrameIndex >= m_animInfo->m_subTextureIDs.getCount())
					m_currentAnimationFrameIndex = 0;

				// Now look up the texture/sprite information for this frame.
				PUInt32 spriteIndex = m_animInfo->m_subTextureIDs[m_currentAnimationFrameIndex];
				PTextureAtlasInfo *atlas=m_animInfo->m_textureAtlasInfo;
				if (!atlas)
					return PE_RESULT_UNKNOWN_ERROR;
				PSubTextureInfo &spriteInfo = atlas->m_subTextures[spriteIndex];

				// Check if we need to flip the texture animation direction.
				if (!m_flip)
				{
					PHYRE_TRY(m_spriteCollection->setSpriteTextureOriginNormalized(m_id, spriteInfo.m_uOrigin, spriteInfo.m_vOrigin));
					PHYRE_TRY(m_spriteCollection->setSpriteTextureSizeNormalized(m_id, spriteInfo.m_textureSizeX, spriteInfo.m_textureSizeY));
				}
				else
				{
					PHYRE_TRY(m_spriteCollection->setSpriteTextureOriginNormalized(m_id, spriteInfo.m_uOrigin + spriteInfo.m_textureSizeX, spriteInfo.m_vOrigin));
					PHYRE_TRY(m_spriteCollection->setSpriteTextureSizeNormalized(m_id, -spriteInfo.m_textureSizeX, spriteInfo.m_textureSizeY));
				}
				m_time = 0.0f;
			}
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Adds the sprite to the sprite collection.
		//
		// Arguments:
		// collection :		The pointer to the sprite collection.
		// animInfos :		The array of PSpriteAnimationInfo.
		// index :			The animation index to select.
		// x :				The x position of the sprite.
		// y :				The y position of the sprite.
		// sizeX :			The size in the x direction of the sprite.
		// sizeY :			The size in the y direction of the sprite.
		//
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR :     The operation failed because an error occurred while adding the sprite to the PSpriteCollection.
		// PE_RESULT_NO_ERROR :          The sprite was added to the PSpriteCollection successfully.
		PResult PGameSprite::addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PArray<Phyre::PSprite::PSpriteAnimationInfo *> &animInfos, Phyre::PUInt32 index, float x, float y, float sizeX, float sizeY)
		{
			// Select an animation
			m_animInfo = animInfos[index];
			if (!m_animInfo)
				return PE_RESULT_UNKNOWN_ERROR;

			return addToCollection(collection, *m_animInfo, index, x, y, sizeX, sizeY);
		}

		// Description:
		// Adds the sprite to the sprite collection. A specific PSpriteAnimationInfo is used to animate the texture of the sprite.
		//
		// Arguments:
		// collection :		The pointer to the sprite collection.
		// animInfo:		The PSpriteAnimationInfo to animate the texture of the sprite with.
		// index :			The animation index to select.
		// x :				The x position of the sprite.
		// y :				The y position of the sprite.
		// sizeX :			The size in x direction of the sprite.
		// sizeY :			The size in y direction of the sprite.
		//
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR :     The operation failed because an error occurred while adding the sprite to the PSpriteCollection.
		// PE_RESULT_NO_ERROR :          The sprite was added to the PSpriteCollection successfully.
		PResult PGameSprite::addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PSprite::PSpriteAnimationInfo &animInfo, PUInt32 index, float x, float y, float sizeX,float sizeY)
		{
			// Select  frame of the animation
			m_currentAnimationFrameIndex = index;

			// Now look up the texture/sprite information for this frame.
			PUInt32 spriteIndex = animInfo.m_subTextureIDs[m_currentAnimationFrameIndex];
			PTextureAtlasInfo *atlas=animInfo.m_textureAtlasInfo;
			if (!atlas)
				return PE_RESULT_UNKNOWN_ERROR;
			PSubTextureInfo &spriteInfo = atlas->m_subTextures[spriteIndex];

			return addToCollection(collection, spriteInfo, x, y, sizeX, sizeY);
		}

		// Description:
		// Adds the sprite to the sprite collection.
		//
		// Arguments:
		// collection :		The pointer to the sprite collection.
		// spriteInfo:		The PSubTextureInfo for the sprite.
		// x :				The x position of the sprite.
		// y :				The y position of the sprite.
		// sizeX :			The size in x direction of the sprite.
		// sizeY :			The size in y direction of the sprite.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR :     The operation failed because an error occurred while adding the sprite to the PSpriteCollection.
		// PE_RESULT_NO_ERROR :          The sprite was added to the PSpriteCollection successfully.
		PResult PGameSprite::addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PSprite::PSubTextureInfo &spriteInfo, float x, float y, float sizeX,float sizeY)
		{
			m_spriteCollection = collection;

			if (m_spriteCollection)
			{
				PHYRE_TRY(m_spriteCollection->addSprite(m_id));
				PHYRE_TRY(m_spriteCollection->setPosition(m_id, x, y));
				PHYRE_TRY(m_spriteCollection->setSpriteSize(m_id, sizeX, sizeY));
				PHYRE_TRY(m_spriteCollection->setSpriteTextureOriginNormalized(m_id, spriteInfo.m_uOrigin, spriteInfo.m_vOrigin));
				PHYRE_TRY(m_spriteCollection->setSpriteTextureSizeNormalized(m_id, spriteInfo.m_textureSizeX, spriteInfo.m_textureSizeY));

				// Define some depth order.
				PHYRE_TRY(m_spriteCollection->setDepth(m_id, (float) (m_spriteCollection->getCurrentSpriteCount() - (float)m_spriteCollection->getMaxSpriteCount()) / m_spriteCollection->getMaxSpriteCount()));

				m_active = true;
				m_time = 0.0f;

				return PE_RESULT_NO_ERROR;
			}
			return PE_RESULT_UNKNOWN_ERROR;
		}

		// Description:
		// The constructor for PGameSprite class.
		PGameSprite::PGameSprite()
		{
			m_flip = false;
			m_active = false;
			m_spriteCollection = NULL;
			m_animInfo = NULL;
			m_time = 0.0f;
			m_id = 0;
			m_currentAnimationFrameIndex = 0;
			m_velocity[0] = m_velocity[1] = 0.0f;
			m_timeInterval = 0.04f;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace
