/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_GAME_SPRITE_H
#define PHYRE_SAMPLES_COMMON_GAME_SPRITE_H

namespace Phyre
{
	namespace PCommon
	{

		// Description:
		// Describes a gameplay related sprite.
		class PGameSprite : public PBase
		{
			PHYRE_BIND_DECLARE_CLASS(PGameSprite, PBase);
		public:
			static float s_width;										// The width of the game world.
			static float s_height;									    // The height of the game world.
			Phyre::PSprite::PSpriteCollection  *m_spriteCollection;		// A pointer to the sprite collection.
			PUInt32 m_id;            									// The sprite ID.
			PUInt32 m_currentAnimationFrameIndex;     					// The ID of the current animation frame.
			Phyre::PSprite::PSpriteAnimationInfo *m_animInfo;			// A pointer to the animation info for this sprite.
			float m_time;												// The local time for the sprite.
			float m_timeInterval;										// The time (in seconds) between the animation frames.

			virtual PResult addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PArray<Phyre::PSprite::PSpriteAnimationInfo *> &animInfos, Phyre::PUInt32 index, float x, float y, float sizeX, float sizeY);
			PResult addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PSprite::PSpriteAnimationInfo &animInfo, Phyre::PUInt32 index, float x, float y, float sizeX, float sizeY);
			PResult addToCollection(Phyre::PSprite::PSpriteCollection *collection, Phyre::PSprite::PSubTextureInfo &spriteInfo, float x, float y, float sizeX, float sizeY);

			virtual PResult animate(float elapsedTime);

			bool m_active;											// A flag that indicates whether the sprite is active.
			bool m_flip;											// A flag that indicates whether to flip the sprite texture.
			float m_velocity[2];									// The velocity vector of the sprite.

			PGameSprite();

			// Description:
			// The destructor for the PGameSprite class.
			virtual ~PGameSprite()
			{
				// Remove dead sprites from the collection.
				if (m_spriteCollection && m_active)
					m_spriteCollection->removeSprite(m_id);

			};

			static void SetScreenWidthHeight(float width, float height);
			void setVelocity(float v0, float v1);
		};
	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_GAME_SPRITE_H
