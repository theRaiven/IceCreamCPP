#include "MainWindow.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <algorithm>
#include <iostream>

namespace IceCream::App::Views
{
	int MainWindow::Run(int argc, char** argv)
	{
		instance_ = this;

		glutInit(&argc, argv);

		// Совместимый профиль: шейдеры 3.3 для геометрии и GLUT-текст на панели.
		glutInitContextVersion(3, 3);
		glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
		glutInitWindowSize(windowWidth_, windowHeight_);
		glutCreateWindow("IceCream");
		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

		glewExperimental = GL_TRUE;
		const GLenum glewResult = glewInit();

		if (glewResult != GLEW_OK)
		{
			std::cerr << "GLEW error: " << glewGetErrorString(glewResult) << '\n';
			glutDestroyWindow(glutGetWindow());

			return 1;
		}

		glEnable(GL_DEPTH_TEST);

		parametersPanel_.Initialize(viewModel_, [this]
		                            {
			                            ReloadMeshIfChanged();
			                            glutPostRedisplay();
		                            });
		sceneView_.Initialize();
		sectionView_.Initialize();
		ReloadMesh();
		UpdateLayout();

		glutDisplayFunc(Display);
		glutReshapeFunc(Reshape);
		glutMouseFunc(Mouse);
		glutMotionFunc(MouseMotion);
		glutMouseWheelFunc(MouseWheel);
		glutKeyboardFunc(Keyboard);
		glutSpecialFunc(Special);
		glutCloseFunc(Close);

		glutMainLoop();

		return 0;
	}

	void MainWindow::UpdateLayout()
	{
		constexpr int PanelWidth = 400;
		const int availableWidth = std::max(windowWidth_ - PanelWidth, 2);
		const int splitX = availableWidth * 58 / 100;
		sceneView_.SetViewport(PanelWidth, 0, splitX, windowHeight_);
		sectionView_.SetViewport(PanelWidth + splitX, 0, availableWidth - splitX, windowHeight_);
		parametersPanel_.SetViewport(0, 0, PanelWidth, windowHeight_);
	}

	void MainWindow::ReloadMesh()
	{
		sceneView_.UploadMesh(viewModel_.Body().Mesh());
		uploadedRevision_ = viewModel_.GeometryRevision();
		std::cout << "Vertices: " << viewModel_.Body().Mesh().Vertices().size()
		          << "  Triangles: " << viewModel_.Body().Mesh().TriangleCount()
		          << "  Volume: " << viewModel_.Volume() << '\n';
	}

	void MainWindow::ReloadMeshIfChanged()
	{
		if (viewModel_.GeometryRevision() != uploadedRevision_)
		{
			ReloadMesh();
		}
	}

	void MainWindow::Display()
	{
		glClearColor(0.10f, 0.11f, 0.14f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		instance_->sceneView_.Render(instance_->viewModel_);
		instance_->sectionView_.Render(instance_->viewModel_);
		instance_->parametersPanel_.Render(instance_->viewModel_);

		glutSwapBuffers();
	}

	void MainWindow::Reshape(int width, int height)
	{
		if (width < 1200 || height < 800)
		{
			glutReshapeWindow(std::max(width, 1200), std::max(height, 800));

			return;
		}

		instance_->windowWidth_ = std::max(width, 1);
		instance_->windowHeight_ = std::max(height, 1);
		instance_->UpdateLayout();
		glutPostRedisplay();
	}

	void MainWindow::Mouse(int button, int state, int x, int y)
	{
		if (button == GLUT_RIGHT_BUTTON)
		{
			if (state == GLUT_DOWN && instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
			{
				instance_->sceneView_.OnPanDown(x, y);
			}
			else if (state == GLUT_UP)
			{
				instance_->sceneView_.OnPanUp();
			}

			glutPostRedisplay();

			return;
		}

		if (button != GLUT_LEFT_BUTTON)
		{
			return;
		}

		const bool panelHandled = instance_->parametersPanel_.Mouse(state == GLUT_DOWN, x, y,
		                                                            instance_->windowHeight_, instance_->viewModel_);
		instance_->ReloadMeshIfChanged();
		glutPostRedisplay();

		if (panelHandled)
		{
			instance_->sceneView_.OnMouseUp();

			return;
		}

		if (state == GLUT_DOWN && instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
		{
			instance_->sceneView_.OnMouseDown(x, y);
		}
		else
		{
			instance_->sceneView_.OnMouseUp();
		}
	}

	void MainWindow::MouseMotion(int x, int y)
	{
		if (!instance_->parametersPanel_.Motion(x, instance_->viewModel_))
		{
			instance_->sceneView_.OnMouseMove(x, y);
		}

		glutPostRedisplay();
	}

	void MainWindow::MouseWheel(int, int direction, int x, int y)
	{
		if (instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
		{
			instance_->sceneView_.OnWheel(direction);
			glutPostRedisplay();
		}
	}

	void MainWindow::Keyboard(unsigned char key, int, int)
	{
		auto& model = instance_->viewModel_;

		if (instance_->parametersPanel_.Keyboard(key, model))
		{
			instance_->ReloadMeshIfChanged();
			glutPostRedisplay();

			return;
		}

		const auto& parameters = model.Parameters();

		if (model.IsHeightField() && key != ' ' && key != 'c' && key != 'C')
		{
			return;
		}

		constexpr double Step = 0.1;

		try
		{
			switch (key)
			{
			case 'q':
			case 'Q':
				model.SetBaseHeight(parameters.BaseHeight + Step);
				instance_->ReloadMesh();
				break;
			case 'a':
			case 'A':
				model.SetBaseHeight(parameters.BaseHeight - Step);
				instance_->ReloadMesh();
				break;
			case 'w':
			case 'W':
				model.SetJoinRadius(parameters.JoinRadius + Step);
				instance_->ReloadMesh();
				break;
			case 's':
			case 'S':
				model.SetJoinRadius(parameters.JoinRadius - Step);
				instance_->ReloadMesh();
				break;
			case 'e':
			case 'E':
				model.SetBaseExponent(parameters.BaseExponent + Step);
				instance_->ReloadMesh();
				break;
			case 'd':
			case 'D':
				model.SetBaseExponent(parameters.BaseExponent - Step);
				instance_->ReloadMesh();
				break;
			case 'r':
			case 'R':
				model.SetCapHeight(parameters.CapHeight + Step);
				instance_->ReloadMesh();
				break;
			case 'f':
			case 'F':
				model.SetCapHeight(parameters.CapHeight - Step);
				instance_->ReloadMesh();
				break;
			case 'c':
			case 'C':
				instance_->sceneView_.ResetCamera();
				break;
			case ' ':
				model.ResetDefaults();
				instance_->sceneView_.ResetCamera();
				instance_->ReloadMesh();
				break;
			default:
				return;
			}
		}
		catch (const std::exception& error)
		{
			std::cerr << error.what() << '\n';
		}

		glutPostRedisplay();
	}

	void MainWindow::Special(int key, int, int)
	{
		auto& model = instance_->viewModel_;

		if (instance_->parametersPanel_.Editing())
		{
			return;
		}

		constexpr double OffsetStep = 0.05;
		constexpr double AngleStep = 5.0;

		switch (key)
		{
		case GLUT_KEY_LEFT:
			model.SetSectionPosition(2, std::max(-20.0, model.SectionPosition().Z - OffsetStep));
			break;
		case GLUT_KEY_RIGHT:
			model.SetSectionPosition(2, std::min(20.0, model.SectionPosition().Z + OffsetStep));
			break;
		case GLUT_KEY_UP:
			model.SetSectionRotation(1, model.SectionRotation().Y + AngleStep);
			break;
		case GLUT_KEY_DOWN:
			model.SetSectionRotation(1, model.SectionRotation().Y - AngleStep);
			break;
		default:
			return;
		}

		glutPostRedisplay();
	}

	void MainWindow::Close()
	{
		instance_->parametersPanel_.Destroy();
		instance_->sceneView_.Destroy();
		instance_->sectionView_.Destroy();
	}
}
