/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_H
#define PHYRE_SAMPLES_COMMON_H

namespace Phyre
{
	// Description:
	// The namespace for Phyre samples common classes and functions.
	namespace PCommon
	{

		// Description:
		// Finds the first object of the templated type in the cluster.
		// Template Arguments:
		// CLASS - The class of object to find.
		// Arguments:
		// cluster - The cluster to search for objects.
		// Returns:
		// A pointer to the first object of the templated type if found otherwise NULL.
		template <class CLASS>
		CLASS *FindFirstInstanceInCluster(PCluster &cluster)
		{
			CLASS *result = NULL;
			PCluster::PObjectIteratorOfType<CLASS> it(cluster);
			if(it)
				result = &*it;

			return result;
		}

		// Description:
		// The PLoadedClusterArray class is a helper class for storing an array of loaded clusters and destroying them at shutdown.
		class PLoadedClusterArray : public PSharray<PCluster *>
		{
		public:

			// Description:
			// Deletes all of the clusters in the array and then frees the memory used to store the array.
			inline void clear()
			{
				PUInt32		count		= getCount();
				PCluster	**clusters	= getArray();
				while(count--)
					delete *clusters++;
				resize(0);
			}
		};

		// Description:
		// Gets a dynamic property value from a component using pointer types.
		// Template Arguments:
		// T - The type of the dynamic property.
		// Arguments:
		// component - The component to get the type from.
		// name - The name of the property.
		// Returns:
		// A pointer to the dynamic property value if it was found; otherwise NULL is returned.
		template<typename T>
		inline T* GetDynamicPropertyValuePtr(const Phyre::PComponent *component, const char *name)
		{
			const Phyre::PClassDescriptor *cd = &component->getComponentType();
			const Phyre::PClassDataMember *nameMember = cd->getMemberByName<Phyre::PClassDataMember>(name);
			if (nameMember)
			{
				const Phyre::PBase *base = component;
				const void *namePtr = nameMember->data(*base);
				return (T *)namePtr;
			}
			else
			{
				return (T *)NULL;
			}
		}

		// Description:
		// Gets a dynamic property from a component using pointer types.
		// Template Arguments:
		// T - The type of the dynamic property.
		// Arguments:
		// component - The component to get the type from.
		// name - The name of the property.
		// Returns
		// A pointer to the dynamic property if it was found; otherwise NULL is returned.
		template<typename T>
		inline T* GetDynamicPropertyPtr(const Phyre::PComponent *component, const char *name)
		{
			const Phyre::PClassDescriptor *cd = &component->getComponentType();
			const Phyre::PClassDataMember *nameMember = cd->getMemberByName<Phyre::PClassDataMember>(name);
			if (nameMember)
			{
				const Phyre::PBase *base = component;
				const void *namePtr = nameMember->data(*base);
				return *((T **)namePtr);
			}
			else
			{
				return (T *)NULL;
			}
		}

		// Description:
		// Checks if a dynamic property exists on a component.
		// Template Arguments:
		// T - The type of the dynamic property.
		// Arguments:
		// component - The component to get the type from.
		// name - The name of the property.
		// Return Value List:
		// true : The dynamic property exists.
		// false : The dynamic property does not exist.
		inline bool DynamicPropertyIsPresent(const Phyre::PComponent *component, const char *name)
		{
			const Phyre::PClassDescriptor *cd = &component->getComponentType();
			const Phyre::PClassDataMember *nameMember = cd->getMemberByName<Phyre::PClassDataMember>(name);
			return (nameMember) ? true : false;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_H
