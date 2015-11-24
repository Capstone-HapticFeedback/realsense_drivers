/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2014-2015 Intel Corporation. All Rights Reserved.

*******************************************************************************/

#include <r200_driver/DSAPI.h>
#include <r200_driver/DSAPI/DSAPITypes.h>
#include <r200_driver/Common.h>
#include <cassert>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <memory>
#include <map>

#ifndef _WIN32
#include <stdalign.h>
#endif

/// RAII ( Resource Allocation Is Initialization ) wrapper for DSAPI
/// Use  std::shared_ptr<DSAPI> instead of DSAPI* to insure Exception safety and automatic memory management of DSAPI
/// Declared statically here to make sure it is destructed when exit is called.

static std::shared_ptr<DSAPI> g_dsapi;


DSThird * g_third;
DSHardware * g_hardware;
bool g_paused, g_showOverlay = true, g_showImages = true, g_stopped = true;
GlutWindow g_depthWindow, g_leftWindow, g_rightWindow, g_thirdWindow; // Windows where we will display depth, left, right, third images
Timer g_timer, g_thirdTimer;
int g_lastThirdFrame;
float g_exposure, g_gain;
bool g_useAutoExposure = false;
bool g_useAutoWhiteBalance = true;

uint8_t g_zImageRGB[640 * 480 * 3];

float g_minExposure = 0.1f;
float g_maxExposure = 16.3f;
float g_minGain = 1.0f;
float g_maxGain = 64.0f;

void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels);

uint8_t g_leftImage[640 * 480], g_rightImage[640 * 480];

#ifdef _WIN32
__declspec(align(16)) uint8_t g_thirdImage[1920 * 1080 * 4];
#else
uint8_t g_thirdImage alignas(16)[1920 * 1080 * 4];
#endif


void OnIdle()
{
    if (g_stopped)
        return;

    if (!g_paused)
    {
        DS_CHECK_ERRORS(g_dsapi->grab());
    }

    // Display Z image, if it is enabled
    if (g_dsapi->isZEnabled() && g_depthWindow.alive())
    {
        const uint8_t nearColor[] = {255, 0, 0}, farColor[] = {20, 40, 255};
        if (g_showImages) ConvertDepthToRGBUsingHistogram(g_dsapi->getZImage(), g_dsapi->zWidth(), g_dsapi->zHeight(), nearColor, farColor, g_zImageRGB);
        DrawGLImage(g_depthWindow, g_dsapi->zWidth(), g_dsapi->zHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_zImageRGB);
    }

    // Display left/right images, if they are enabled
    if ((g_dsapi->isLeftEnabled() && g_leftWindow.alive()) || (g_dsapi->isRightEnabled() && g_rightWindow.alive()))
    {
        assert(g_dsapi->getLRPixelFormat() == DS_LUMINANCE8);
        if (g_dsapi->isLeftEnabled()) DrawGLImage(g_leftWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_showImages ? g_dsapi->getLImage() : nullptr);
        if (g_dsapi->isRightEnabled()) DrawGLImage(g_rightWindow, g_dsapi->lrWidth(), g_dsapi->lrHeight(), GL_LUMINANCE, GL_UNSIGNED_BYTE, g_showImages ? g_dsapi->getRImage() : nullptr);
    }

    // Display third image, if it is enabled
    if (g_third && g_third->isThirdEnabled() && g_thirdWindow.alive() && g_third->getThirdFrameNumber() != g_lastThirdFrame)
    {
        assert(g_third->getThirdPixelFormat() == DS_RGB8);
        DrawGLImage(g_thirdWindow, g_third->thirdWidth(), g_third->thirdHeight(), GL_RGB, GL_UNSIGNED_BYTE, g_showImages ? g_third->getThirdImage() : nullptr);
        g_lastThirdFrame = g_third->getThirdFrameNumber();

        g_thirdTimer.OnFrame();
    }

    g_timer.OnFrame();
}

void PrintControls()
{
    std::cout << "\nProgram controls:" << std::endl;
    std::cout << "  (space) Pause/unpause" << std::endl;
    std::cout << "  (d) Toggle display of overlay" << std::endl;
    std::cout << "  (i) Toggle display of images" << std::endl;
    std::cout << "  (s) Take snapshot" << std::endl;
    std::cout << "  (a) toggle L/R auto exposure (default is " << g_useAutoExposure << " )" << std::endl;
    std::cout << "  (e/E) Modify exposure" << std::endl;
    std::cout << "  (g/G) Modify gain" << std::endl;
    std::cout << "  (l) Toggle emitter" << std::endl;
    std::cout << "  (q) Quit application" << std::endl;
}

void OnKeyboard(unsigned char key, int x, int y)
{
    switch (std::tolower(key))
    {
    case ' ':
        g_paused = !g_paused;
        break;
    case 'a':
        g_useAutoExposure = !g_useAutoExposure;
        std::cout << "Setting L/R Auto Exposure " << g_useAutoExposure << std::endl;
        g_hardware->setAutoExposure(DS_BOTH_IMAGERS, g_useAutoExposure);
        break;
    case 'd':
        g_showOverlay = !g_showOverlay;
        break;
    case 'i':
        g_showImages = !g_showImages;
        break;
    case 's':
        g_dsapi->takeSnapshot();
        break;
    case 'e':
        if (!g_useAutoExposure && g_hardware)
        {
            g_exposure = std::min(std::max(g_exposure + (key == 'E' ? 0.1f : -0.1f), g_minExposure), g_maxExposure);
            std::cout << "Setting exposure to " << std::fixed << std::setprecision(1) << g_exposure << std::endl;
            if (!g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS))
            {
                std::cerr << "Failed:";
                std::cerr << "\n  status:  " << DSStatusString(g_dsapi->getLastErrorStatus());
                std::cerr << "\n  details: " << g_dsapi->getLastErrorDescription() << std::endl;
            }
        }
        break;
    case 'g':
        if (!g_useAutoExposure && g_hardware)
        {
            g_gain = std::min(std::max(g_gain + (key == 'G' ? 0.1f : -0.1f), g_minGain), g_maxGain);
            std::cout << "Setting gain to " << std::fixed << std::setprecision(1) << g_gain << std::endl;
            if (!g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS))
            {
                std::cerr << "Failed:";
                std::cerr << "\n  status:  " << DSStatusString(g_dsapi->getLastErrorStatus());
                std::cerr << "\n  details: " << g_dsapi->getLastErrorDescription() << std::endl;
            }
        }
        break;
    case 'l':
        if (auto emitter = g_dsapi->accessEmitter())
        {
            bool enabled = false;
            DS_CHECK_ERRORS(emitter->getEmitterStatus(enabled));
            DS_CHECK_ERRORS(emitter->enableEmitter(!enabled));
        }
        break;
    case 'q':
        glutLeaveMainLoop();
        break;
    case 'r':
        if (g_dsapi->startCapture())
            g_stopped = false;
        break;
    case 't':
        if (g_dsapi->stopCapture())
            g_stopped = true;
        break;
    case 'w':
        g_useAutoWhiteBalance = !g_useAutoWhiteBalance;
        std::cout << "Setting Auto White Balance (Third) " << g_useAutoWhiteBalance << std::endl;
        //    g_hardware->setUseAutoWhiteBalance(g_useAutoWhiteBalance, DS_THIRD_IMAGER);
        break;
    }
}

std::map<int, bool> GlutWindow::window_list;


int main(int argc, char * argv[])
{
    g_dsapi = std::shared_ptr<DSAPI>(DSCreate(DS_DS4_PLATFORM), DSDestroy);
    DS_CHECK_ERRORS(g_dsapi->openDevice());

    g_dsapi->setLoggingLevelAndFile(DS_LOG_ERROR, "DSSimpleCaptureGL.log");

    g_third = g_dsapi->accessThird();
    g_hardware = g_dsapi->accessHardware();

    DS_CHECK_ERRORS(g_dsapi->probeConfiguration());

    std::cout << "Firmware version: " << g_dsapi->getFirmwareVersionString() << std::endl;
    std::cout << "Software version: " << g_dsapi->getSoftwareVersionString() << std::endl;
    uint32_t serialNo = 0;
    DS_CHECK_ERRORS(g_dsapi->getCameraSerialNumber(serialNo));
    std::cout << "Camera serial no: " << serialNo << std::endl;

    // Check calibration data
    if (!g_dsapi->isCalibrationValid())
    {
        std::cout << "Calibration data on camera is invalid" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Configure core Z-from-stereo capabilities
    DS_CHECK_ERRORS(g_dsapi->enableZ(true));
    DS_CHECK_ERRORS(g_dsapi->enableLeft(false));
    DS_CHECK_ERRORS(g_dsapi->enableRight(false));
    DS_CHECK_ERRORS(g_dsapi->setLRZResolutionMode(true, 480, 360, 60, DS_LUMINANCE8)); // Valid resolutions: 628x468, 480x360
    DS_CHECK_ERRORS(g_dsapi->enableLRCrop(true));

    // Configure third camera
    if (g_third)
    {
        DS_CHECK_ERRORS(g_third->enableThird(true));
        DS_CHECK_ERRORS(g_third->setThirdResolutionMode(true, 640, 480, 30, DS_RGB8)); // Valid resolutions: 1920x1080, 640x480
    }

    // Change exposure and gain values
    //g_exposure = 16.3f;
    //g_gain = 2.0f;
    //DS_CHECK_ERRORS(g_hardware->setImagerExposure(g_exposure, DS_BOTH_IMAGERS));
    //DS_CHECK_ERRORS(g_hardware->setImagerGain(g_gain, DS_BOTH_IMAGERS));

    g_hardware->setAutoExposure(DS_BOTH_IMAGERS, g_useAutoExposure);

    //float exposure, gain;
    //DS_CHECK_ERRORS(g_hardware->getImagerExposure(exposure, DS_BOTH_IMAGERS));
    //DS_CHECK_ERRORS(g_hardware->getImagerGain(gain, DS_BOTH_IMAGERS));
    //std::cout << "(Before startCapture()) Initial exposure: " << exposure << ", initial gain: " << gain << std::endl;

    // Begin capturing images
    DS_CHECK_ERRORS(g_dsapi->startCapture());
    g_stopped = false;

    float defaultExposure = 0.0f;
    float exposureDelta = 0.0f;
    g_hardware->getImagerMinMaxExposure(g_minExposure, g_maxExposure, defaultExposure, exposureDelta, DS_BOTH_IMAGERS);
    std::cout << "Allowed exposure range " << g_minExposure << " to " << g_maxExposure << std::endl;

    float defaultGain = 0.0f;
    float gainDelta = 0.0f;
    g_hardware->getImagerMinMaxGain(g_minGain, g_maxGain, defaultGain, gainDelta, DS_BOTH_IMAGERS);
    std::cout << "Allowed gain     range " << g_minGain << " to " << g_maxGain << std::endl;

    if (!g_useAutoExposure)
    {
        DS_CHECK_ERRORS(g_hardware->getImagerExposure(g_exposure, DS_BOTH_IMAGERS));
        DS_CHECK_ERRORS(g_hardware->getImagerGain(g_gain, DS_BOTH_IMAGERS));
        std::cout << "(After startCapture()) Initial exposure: " << g_exposure << ", initial gain: " << g_gain << std::endl;
    }

    // Open some GLUT windows to display the incoming images
    glutInit(&argc, argv);
    glutIdleFunc(OnIdle);

    if (g_dsapi->isZEnabled())
    {
        std::ostringstream title;
        title << "Z: " << g_dsapi->zWidth() << "x" << g_dsapi->zHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_depthWindow.Open(title.str(), 300 * g_dsapi->zWidth() / g_dsapi->zHeight(), 300, 60, 60, OnKeyboard);
    }

    if (g_dsapi->isLeftEnabled())
    {
        std::ostringstream title;
        title << "Left (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_leftWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 60, 420, OnKeyboard);
    }

    if (g_dsapi->isRightEnabled())
    {
        std::ostringstream title;
        title << "Right (" << DSPixelFormatString(g_dsapi->getLRPixelFormat()) << "): " << g_dsapi->lrWidth() << "x" << g_dsapi->lrHeight() << "@" << g_dsapi->getLRZFramerate() << " FPS";
        g_rightWindow.Open(title.str(), 300 * g_dsapi->lrWidth() / g_dsapi->lrHeight(), 300, 520, 420, OnKeyboard);
    }

    if (g_third && g_third->isThirdEnabled())
    {
        std::ostringstream title;
        title << "Third (" << DSPixelFormatString(g_third->getThirdPixelFormat()) << "): " << g_third->thirdWidth() << "x" << g_third->thirdHeight() << "@" << g_third->getThirdFramerate() << " FPS";
        g_thirdWindow.Open(title.str(), 300 * g_third->thirdWidth() / g_third->thirdHeight(), 300, 520, 60, OnKeyboard);
    }

    // Turn control over to GLUT
    PrintControls();
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    glutMainLoop();

    return 0;
}


void DrawGLImage(GlutWindow & window, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    window.ClearScreen(0.1f, 0.1f, 0.15f);

    if (g_showImages)
    {
        window.DrawImage(width, height, format, type, pixels, 1);
    }

    if (g_showOverlay)
    {
        DrawString(10, 10, "FPS: %0.1f", (&window == &g_thirdWindow ? g_thirdTimer : g_timer).GetFramesPerSecond());
        DrawString(10, 28, "Frame #: %d", &window == &g_thirdWindow ? g_third->getThirdFrameNumber() : g_dsapi->getFrameNumber());
        DrawString(10, 46, "Frame time: %s", GetHumanTime(&window == &g_thirdWindow ? g_third->getThirdFrameTime() : g_dsapi->getFrameTime()).c_str());
    }

    glutSwapBuffers();
}
