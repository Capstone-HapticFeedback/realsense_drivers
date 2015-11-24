/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2014-2015 Intel Corporation. All Rights Reserved.

*******************************************************************************/

#pragma once

#include <r200_driver/DSAPIUtil.h>

#include <algorithm>
#include <cstdlib>
#include <cstdint>  // For uint8_t, uint16_t, ...
#include <cstdarg>  // For va_list, va_start, ...
#include <cstdio>   // For vsnprintf
#include <ctime>    // For time_t, gmtime, strftime
#include <string>   // For std::string
#include <iostream> // For ostream, cerr
#include <iomanip>
#include <GL/freeglut.h> // For basic input/output
#include <map>

#ifdef _WIN32
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

// DSAPI functions which can fail will return false.
// When this happens, DSAPI::getLastErrorStatus() will return an error code, and DSAPI::getLastErrorDescription() will return a human-readable detail string about what went wrong.
// Note: g_dsapi DSAPI* is hardwired
#define DS_CHECK_ERRORS(s)                                                                           \
    do                                                                                               \
    {                                                                                                \
        if (!s)                                                                                      \
        {                                                                                            \
            std::cerr << "\nDSAPI call failed at " << __FILE__ << ":" << __LINE__ << "!";            \
            std::cerr << "\n  status:  " << DSStatusString(g_dsapi->getLastErrorStatus());           \
            std::cerr << "\n  details: " << g_dsapi->getLastErrorDescription() << '\n' << std::endl; \
            std::cerr << "\n hit return to exit " << '\n' << std::endl;                              \
            getchar();                                                                               \
            exit(EXIT_FAILURE);                                                                      \
        }                                                                                            \
    } while (false);


// Simple utility class which computes a frame rate
class Timer
{
    int lastTime, frameCounter;
    float timeCounter, framesPerSecond;

public:
    Timer()
        : lastTime()
        , frameCounter()
        , timeCounter()
        , framesPerSecond()
    {
    }

    float GetFramesPerSecond() const { return framesPerSecond; }

    void OnFrame()
    {
        ++frameCounter;
        int time = glutGet(GLUT_ELAPSED_TIME);
        timeCounter += (time - lastTime) * 0.001f;
        lastTime = time;
        if (timeCounter >= 0.5f)
        {
            framesPerSecond = frameCounter / timeCounter;
            frameCounter = 0;
            timeCounter = 0;
        }
    }
};

// Produce an RGB image from a depth image by computing a cumulative histogram of depth values and using it to map each pixel between a near color and a far color
inline void ConvertDepthToRGBUsingHistogram(const uint16_t depthImage[], int width, int height, const uint8_t nearColor[3], const uint8_t farColor[3], uint8_t rgbImage[])
{
    // Produce a cumulative histogram of depth values
    int histogram[256 * 256] = {1};
    for (int i = 0; i < width * height; ++i)
    {
        if (auto d = depthImage[i]) ++histogram[d];
    }
    for (int i = 1; i < 256 * 256; i++)
    {
        histogram[i] += histogram[i - 1];
    }

    // Remap the cumulative histogram to the range 0..256
    for (int i = 1; i < 256 * 256; i++)
    {
        histogram[i] = (histogram[i] << 8) / histogram[256 * 256 - 1];
    }

    // Produce RGB image by using the histogram to interpolate between two colors
    auto rgb = rgbImage;
    for (int i = 0; i < width * height; i++)
    {
        if (uint16_t d = depthImage[i]) // For valid depth values (depth > 0)
        {
            auto t = histogram[d]; // Use the histogram entry (in the range of 0..256) to interpolate between nearColor and farColor
            *rgb++ = ((256 - t) * nearColor[0] + t * farColor[0]) >> 8;
            *rgb++ = ((256 - t) * nearColor[1] + t * farColor[1]) >> 8;
            *rgb++ = ((256 - t) * nearColor[2] + t * farColor[2]) >> 8;
        }
        else // Use black pixels for invalid values (depth == 0)
        {
            *rgb++ = 0;
            *rgb++ = 0;
            *rgb++ = 0;
        }
    }
}

// Print an non-rectified image's intrinsic properties to stdout
std::ostream & operator<<(std::ostream & out, const DSCalibIntrinsicsNonRectified & i)
{
    out << "\n  Image dimensions: " << i.w << ", " << i.h;
    out << "\n  Focal lengths: " << i.fx << ", " << i.fy;
    out << "\n  Principal point: " << i.px << ", " << i.py;
    return out << "\n  Distortion coefficients: " << i.k[0] << ", " << i.k[1] << ", " << i.k[2] << ", " << i.k[3] << ", " << i.k[4];
}

// Print a rectified image's intrinsic properties to stdout
std::ostream & operator<<(std::ostream & out, const DSCalibIntrinsicsRectified & i)
{
    float hFOV = 0.0f;
    float vFOV = 0.0f;
    DSFieldOfViewsFromIntrinsicsRect(i, hFOV, vFOV);

    out << "\n  Image dimensions: " << i.rw << ", " << i.rh;
    out << "\n  Focal lengths: " << i.rfx << ", " << i.rfy;
    out << "\n  Principal point: " << i.rpx << ", " << i.rpy;
    return out << "\n  Field of view: " << hFOV << ", " << vFOV;
}

// Format a DSAPI timestamp in a human-readable fashion
std::string GetHumanTime(double secondsSinceEpoch)
{
    time_t time = (time_t)secondsSinceEpoch;
    char buffer[80];
    struct tm * pTime = gmtime(&time);
    if (pTime)
    {
        size_t i = strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", pTime);
        snprintf(buffer + i, std::max<int>((int)0, (int)(sizeof(buffer) - i)), ".%02d UTC", static_cast<int>(fmod(secondsSinceEpoch, 1.0) * 100));
        return buffer;
    }
    else
        return "";
}

// Display a string using GLUT, with a 50% opaque black rectangle behind the text to make it stand out from bright backgrounds
inline void DrawString(int x, int y, const char * format, ...)
{
    // Format output string
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Compute string width in pixels
    int width = 0;
    for (auto s = buffer; *s; ++s)
    {
        width += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *s);
    }

    // Set up a pixel-aligned orthographic coordinate space
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, +1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Draw a 50% opaque black rectangle
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(0, 0, 0, 0.5f);
    glVertex2i(x, y);
    glVertex2i(x + width + 4, y);
    glVertex2i(x + width + 4, y + 16);
    glVertex2i(x, y + 16);
    glEnd();
    glDisable(GL_BLEND);

    // Draw the string using bitmap glyphs
    glColor3f(1, 1, 1);
    glRasterPos2i(x + 2, y + 12);
    for (auto s = buffer; *s; ++s)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
    }

    // Restore GL state to what it was prior to this call
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

class GlutWindow
{

    static std::map<int, bool> window_list;

    int windowId;
    GLuint texture;
    static void OnDisplay()
    {
        glutPostRedisplay();
    }
    static void onClose()
    {
        //        GlutWindow::print_window_list();

        int git = glutGetWindow();
        if (git != 0)
        {
            auto it = GlutWindow::window_list.find(git);
            if (it != GlutWindow::window_list.end())
            {
                GlutWindow::window_list.erase(it);
            }
        }
        //        GlutWindow::print_window_list();
    }

    void Begin()
    {
        glutSetWindow(windowId);
        glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        //		glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushMatrix();
        glPushAttrib(GL_CURRENT_BIT);
        glMatrixMode(GL_MODELVIEW);
    }
    void End()
    {
        glPopAttrib();
        glPopMatrix();
    }

public:
    GlutWindow() {}
    int id() const { return windowId; }
    bool alive()
    {
        if (GlutWindow::window_list.empty()) return false;
        if (GlutWindow::window_list.find((int)windowId) == GlutWindow::window_list.end()) return false;
        return true;
    }

    static const std::map<int, bool> & get_window_list() { return GlutWindow::window_list; }
    static void clear_window_list()
    {
        GlutWindow::window_list.clear();
    }
    static void print_window_list()
    {
        std::map<int, bool>::iterator it;
        for (it = GlutWindow::window_list.begin(); it != GlutWindow::window_list.end(); it++)
            std::cout << (*it).first << " => " << std::boolalpha << (*it).second << '\n';
    }

    ~GlutWindow()
    {
    }

    void Open(std::string name, int width, int height, int startX, int startY,
              void (*keyboardFunc)(unsigned char key, int x, int y),
              void (*mouseFunc)(int button, int state, int x, int y) = nullptr)

    {
        glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
        glutInitWindowSize(width, height);
        glutInitWindowPosition(startX, startY);
        windowId = glutCreateWindow(name.c_str());
        glutDisplayFunc(OnDisplay);
        glutKeyboardFunc(keyboardFunc);
        if (mouseFunc)
            glutMouseFunc(mouseFunc);
        glutCloseFunc(&GlutWindow::onClose);

        glutSetWindow(windowId);
        GlutWindow::window_list[windowId] = true;
    }

    void ClearScreen(float r, float g, float b)
    {
        Begin();
        glClearColor(r, g, b, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        End();
    }

    void DrawImage(GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels, GLfloat multiplier)
    {
        Begin();

        // Upload the image as a texture
        if (!texture) glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glPixelTransferf(GL_RED_SCALE, multiplier);
        glPixelTransferf(GL_GREEN_SCALE, multiplier);
        glPixelTransferf(GL_BLUE_SCALE, multiplier);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, type, pixels);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        // Draw the image via a scaled quad, as large as possible without modifying the aspect ratio of the image
        auto winWidth = glutGet(GLUT_WINDOW_WIDTH), winHeight = glutGet(GLUT_WINDOW_HEIGHT);
        auto scaleX = (GLfloat)winWidth / width, scaleY = (GLfloat)winHeight / height, scale = scaleX < scaleY ? scaleX : scaleY;
        auto vertX = width * scale / winWidth, vertY = height * scale / winHeight;
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glColor3f(multiplier, multiplier, multiplier);
        glTexCoord2f(0, 0);
        glVertex2f(-vertX, +vertY);
        glTexCoord2f(1, 0);
        glVertex2f(+vertX, +vertY);
        glTexCoord2f(1, 1);
        glVertex2f(+vertX, -vertY);
        glTexCoord2f(0, 1);
        glVertex2f(-vertX, -vertY);
        glEnd();

        End();
    }

    void DrawCross(const double x, const double y, const double w, const GLfloat linewidth)
    {

        Begin();
        glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
        glLineWidth(linewidth);
        // render lines for each edge of the box
        glBegin(GL_LINES);
        glVertex3d(x - w, y, 0.0);
        glVertex3d(x + w, y, 0.0);
        glVertex3d(x, y - w, 0.0);
        glVertex3d(x, y + w, 0.0);
        glEnd();
        glFlush();
        End();
    }

    // Position the raster cursor to x,y and call above.
    void DrawString(float x, float y, int font, const std::string & str, GLfloat zoom)
    {
        Begin();
        glColor4ub(255, 255, 255, 128);
        glRasterPos2f(x, y);
        glPixelZoom(zoom, zoom);
        glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const uint8_t *)(str.c_str()));
        glEnd();
        glFlush();
        End();
    }
};
