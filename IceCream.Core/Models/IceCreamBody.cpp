#include "IceCreamBody.h"

namespace IceCream::Core::Models
{
	IceCreamBody::IceCreamBody(IceCreamParameters parameters, const SurfaceMeshBuilder& meshBuilder, int baseSegments, int capSegments, int radialSegments)
		: parameters_(parameters),
		// Профиль рожка
		// Профиль мороженного
		profile_(std::make_shared<PowerProfile>(parameters.BaseHeight, parameters.JoinRadius, parameters.BaseExponent), parameters.CapHeight),
		mesh_(meshBuilder.Build(profile_, baseSegments, capSegments, radialSegments))
	{
	}
}
