#pragma once

#include "../../IceCream.Core/Models/IceCreamBody.h"

namespace IceCream::App::ViewModels
{
    /// <summary>
    /// Хранит текущее тело мороженого и предоставляет его представлению.
    /// </summary>
    class MainViewModel
    {
    private:
        Core::Models::IceCreamBody body_;

    public:
        /// <summary>
        /// Создаёт мороженое с начальными параметрами.
        /// </summary>
        MainViewModel();

        /// <summary>
        /// Возвращает текущее тело мороженого.
        /// </summary>
        const Core::Models::IceCreamBody& Body() const
        {
            return body_;
        }
    };
}