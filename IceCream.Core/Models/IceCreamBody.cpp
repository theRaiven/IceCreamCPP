#include "IceCreamBody.h"

namespace IceCream::Core::Models
{
	IceCreamBody::IceCreamBody(IceCreamParameters parameters, const SurfaceMeshBuilder& meshBuilder, int baseSegments, int capSegments, int radialSegments)
		: parameters_(parameters),
		// Профиль рожка
		// Профиль мороженного
		profile_(std::make_shared<IceCreamProfile>(std::make_shared<PowerProfile>(parameters.BaseHeight, parameters.JoinRadius, parameters.BaseExponent), parameters.CapHeight)),
		mesh_(meshBuilder.Build(static_cast<const IceCreamProfile&>(*profile_), baseSegments, capSegments, radialSegments))
	{
	}
    IceCreamBody::IceCreamBody(IceCreamParameters parameters, std::shared_ptr<const Profiles::IRadialProfile> profile, const SurfaceMeshBuilder& builder)
        : parameters_(parameters), profile_(profile ? std::move(profile) : throw std::invalid_argument("Profile cannot be null")), mesh_(builder.Build(*profile_)) {}

    IceCreamBody::IceCreamBody(IceCreamParameters p,TriangleMesh mesh):parameters_(p),mesh_(std::move(mesh)) {}
    double IceCreamBody::MinZ() const {
        double z=mesh_.Vertices().front().Z;
        for(auto p:mesh_.Vertices()) z=std::min(z,p.Z);
        return z;
    }
    double IceCreamBody::MaxZ() const {
        double z=mesh_.Vertices().front().Z;
        for(auto p:mesh_.Vertices()) z=std::max(z,p.Z);
        return z;
    }

}
