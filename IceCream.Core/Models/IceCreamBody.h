#pragma once

#include "IceCreamParameters.h"

#include "../Geometry/Meshes/TriangleMesh.h"

#include "../Profiles/Basic/IRadialProfile.h"
#include "../Profiles/Basic/PowerProfile.h"
#include "../Profiles/Basic/SphericalCapProfile.h"
#include "../Profiles/Composite/TwoPartProfile.h"

#include "../Services/Meshing/SurfaceMeshBuilder.h"

#include <memory>
#include <stdexcept>

namespace IceCream::Core::Models
{
	using Profiles::TwoPartProfile;
	using Profiles::SphericalCapProfile;
	using Profiles::PowerProfile;
	using Geometry::TriangleMesh;
	using Services::SurfaceMeshBuilder;


	class IceCreamBody
	{
	private:
		IceCreamParameters parameters_;
		std::shared_ptr<const Profiles::IRadialProfile> profile_;
		TriangleMesh mesh_;

	public:
		const IceCreamParameters& Parameters() const
		{
			return parameters_;
		}

		const Profiles::IRadialProfile& Profile() const
		{
			if (!profile_) throw std::logic_error("Это тело не является радиальным");
            return *profile_;
		}

		const TriangleMesh& Mesh() const
		{
			return mesh_;
		}

        double MinZ() const;
        double MaxZ() const;
        IceCreamBody(IceCreamParameters parameters, TriangleMesh mesh);
		IceCreamBody(IceCreamParameters parameters, std::shared_ptr<const Profiles::IRadialProfile> profile, const SurfaceMeshBuilder& builder);
		IceCreamBody(IceCreamParameters parameters, const SurfaceMeshBuilder& meshBuilder, int baseSegments = 40, int capSegments = 40, int radialSegments = 64);
	};
}
