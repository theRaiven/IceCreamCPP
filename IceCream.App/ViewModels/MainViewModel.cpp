#include "MainViewModel.h"

#include "../../IceCream.Core/Services/SurfaceMeshBuilder.h"
using  namespace IceCream::Core::Services;
using  namespace IceCream::Core::Models;

namespace IceCream::App::ViewModels
{
    MainViewModel::MainViewModel()
        : body_(
            IceCreamParameters
            {
                3.0,  // BaseHeight — высота основания.
                1.0,  // JoinRadius — радиус соединения.
                0.5,  // BaseExponent — показатель степени основания.
                1.2   // CapHeight — высота шапки.
            },
            SurfaceMeshBuilder()) { }
}