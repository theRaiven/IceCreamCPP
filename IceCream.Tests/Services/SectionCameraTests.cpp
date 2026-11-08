#include "../pch.h"
#include "../../IceCream.App/Views/Graphics/OrbitCamera/SectionCamera.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using IceCream::App::Views::Graphics::SectionCamera;

namespace IceCream::Tests::Views
{
    TEST_CLASS(SectionCameraTests)
    {
    public:
        TEST_METHOD(FitCentersOnDistantContentInsteadOfTheOrigin)
        {
            SectionCamera camera;
            camera.SetViewport(900,600);
            camera.SetContentBounds({999,1001,499,500});
            auto bounds = camera.Bounds();
            Assert::AreEqual(1000.0, camera.CenterX());
            Assert::AreEqual(499.5, camera.CenterY());
            Assert::IsTrue(bounds.MinX < 999 && bounds.MaxX > 1001);
            Assert::IsTrue(bounds.MinY < 499 && bounds.MaxY > 500);
            Assert::IsTrue(bounds.MinX > 990);
            Assert::AreEqual(1.5,(bounds.MaxX-bounds.MinX)/(bounds.MaxY-bounds.MinY),1e-10);
        }

        TEST_METHOD(ZoomKeepsTheWorldPointUnderTheCursor)
        {
            SectionCamera camera;
            camera.SetViewport(1200,800);
            camera.SetContentBounds({-10,10,-5,5});
            auto before = camera.WorldAt(225,610);
            double span = camera.SpanY();
            camera.ZoomAt(3,225,610);
            auto after = camera.WorldAt(225,610);
            Assert::AreEqual(before.first,after.first,1e-10);
            Assert::AreEqual(before.second,after.second,1e-10);
            Assert::IsTrue(camera.SpanY() < span);
            camera.ZoomAt(-3,225,610);
            Assert::AreEqual(span,camera.SpanY(),1e-10);
        }

        TEST_METHOD(PanningFollowsTheMouseInBothDirections)
        {
            SectionCamera camera;
            camera.SetViewport(800,600);
            auto before = camera.WorldAt(400,300);
            camera.PanPixels(120,50);
            auto after = camera.WorldAt(520,350);
            Assert::AreEqual(before.first,after.first,1e-10);
            Assert::AreEqual(before.second,after.second,1e-10);
            Assert::IsFalse(camera.AutoFit());
        }

        TEST_METHOD(SectionChangesDoNotUndoManualNavigation)
        {
            SectionCamera camera;
            camera.SetViewport(900,600);
            camera.PanPixels(100,-50);
            camera.ZoomAt(2,300,200);
            double x = camera.CenterX(), y = camera.CenterY(), span = camera.SpanY();
            camera.SetContentBounds({500,600,-100,-50});
            Assert::AreEqual(x,camera.CenterX());
            Assert::AreEqual(y,camera.CenterY());
            Assert::AreEqual(span,camera.SpanY());
            camera.ResetView();
            Assert::IsTrue(camera.AutoFit());
            Assert::AreEqual(550.0,camera.CenterX());
            Assert::AreEqual(-75.0,camera.CenterY());
        }

        TEST_METHOD(ResizePreservesManualCenterAndSquareUnits)
        {
            SectionCamera camera;
            camera.SetViewport(800,600);
            camera.PanPixels(100,100);
            double x = camera.CenterX(), y = camera.CenterY(), span = camera.SpanY();
            camera.SetViewport(1200,700);
            auto bounds = camera.Bounds();
            Assert::AreEqual(x,camera.CenterX());
            Assert::AreEqual(y,camera.CenterY());
            Assert::AreEqual(span,camera.SpanY());
            Assert::AreEqual((bounds.MaxX-bounds.MinX)/1200,(bounds.MaxY-bounds.MinY)/700,1e-10);
        }

        TEST_METHOD(DeepZoomAtLargeCoordinatesStaysFiniteAndAnchored)
        {
            SectionCamera camera;
            camera.SetViewport(1200,800);
            camera.SetContentBounds({1e9-1,1e9+1,-1e9-1,-1e9+1});
            auto anchor = camera.WorldAt(132,451);
            for (int i = 0; i < 300; ++i) camera.ZoomAt(1,132,451);
            auto after = camera.WorldAt(132,451);
            auto bounds = camera.Bounds();
            Assert::AreEqual(anchor.first,after.first,1e-5);
            Assert::AreEqual(anchor.second,after.second,1e-5);
            Assert::IsTrue(std::isfinite(camera.SpanY()) && bounds.MaxX > bounds.MinX && bounds.MaxY > bounds.MinY);
        }

        TEST_METHOD(ExtremeZoomAndPanDoNotCollapseTheProjection)
        {
            SectionCamera camera;
            camera.SetViewport(800,600);
            for (int i = 0; i < 20; ++i) camera.ZoomAt(100000,400,300);
            camera.PanPixels(1e30,-1e30);
            auto bounds = camera.Bounds();
            Assert::IsTrue(bounds.MaxX > bounds.MinX && bounds.MaxY > bounds.MinY);
            for (int i = 0; i < 20; ++i) camera.ZoomAt(-100000,400,300);
            Assert::IsTrue(std::isfinite(camera.SpanY()) && camera.SpanY() <= 1e9);
        }

        TEST_METHOD(PointAndInvalidBoundsCannotPoisonTheCamera)
        {
            SectionCamera camera;
            camera.SetViewport(0,0);
            camera.SetContentBounds({20,20,30,30});
            auto bounds = camera.Bounds();
            Assert::IsTrue(bounds.MaxX > bounds.MinX && bounds.MaxY > bounds.MinY);
            camera.SetContentBounds({1,0,0,1});
            camera.SetContentBounds({std::numeric_limits<double>::quiet_NaN(),0,0,1});
            Assert::AreEqual(20.0,camera.CenterX());
            Assert::AreEqual(30.0,camera.CenterY());
        }
    };
}
