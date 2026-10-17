#pragma once

#include "IceCreamParameters.h"
#include "../Profiles/PowerProfile.h"
#include "../Services/SurfaceMeshBuilder.h"

namespace IceCream::Core::Models
{
	using Profiles::IceCreamProfile;
	using Profiles::PowerProfile;
	using Geometry::TriangleMesh;
	using Services::SurfaceMeshBuilder;

	class IceCreamBody
	{
	private:
		IceCreamParameters parameters_;
		IceCreamProfile profile_;
		TriangleMesh mesh_;

	public:
		const IceCreamParameters& Parameters() const
		{
			return parameters_;
		}

		const IceCreamProfile& Profile() const
		{
			return profile_;
		}

		const TriangleMesh& Mesh() const
		{
			return mesh_;
		}

		IceCreamBody(IceCreamParameters parameters, const SurfaceMeshBuilder& meshBuilder, int baseSegments = 40, int capSegments = 40, int radialSegments = 64);
	};
}
