/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#include <librealsense/rs.hpp>
#include <r200_camera/examples/example.hpp>
#include <iostream>
#include <algorithm>


std::vector<texture_buffer> buffers;

int main(int argc, char * argv[]) try
{
    rs::context ctx;

    // Enumerate all devices
    std::vector<rs::device *> devices;
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        devices.push_back(ctx.get_device(i));
    }

    // Configure and start our devices
    for(auto dev : devices)
    {
        std::cout << "Starting " << dev->get_name() << "... ";
        dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev->enable_stream(rs::stream::color, rs::preset::best_quality);
        dev->start();
        std::cout << "done." << std::endl;
    }

	// Depth and color
    buffers.resize(ctx.get_device_count() * 2);

    // Open a GLFW window
    glfwInit();
    std::ostringstream ss; ss << "CPP Multi-Camera Example";
    GLFWwindow * win = glfwCreateWindow(1280, 960, ss.str().c_str(), 0, 0);
    glfwMakeContextCurrent(win);

	int windowWidth, windowHeight;
    glfwGetWindowSize(win, &windowWidth, &windowHeight);

	// Does not account for correct aspect ratios
	auto perTextureWidth = windowWidth / devices.size();
	auto perTextureHeight = 480;

    font font;
    if (auto f = find_file(FONT_CONFIG_FILE_PATH.c_str(), 3))
    {
        font = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

    while (!glfwWindowShouldClose(win))
    {
        // Wait for new images
        glfwPollEvents();
        for(auto dev : devices) dev->wait_for_frames();
        
        // Draw the images
        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glfwGetWindowSize(win, &w, &h);
        glPushMatrix();
        glOrtho(0, w, h, 0, -1, +1);
        glPixelZoom(1, -1);
        int i=0, x=0;
        for(auto dev : devices)
        {
            const auto c = dev->get_stream_intrinsics(rs::stream::color), d = dev->get_stream_intrinsics(rs::stream::depth);
            buffers[i++].show(*dev, rs::stream::color, x, 0, perTextureWidth, perTextureHeight, font);
            buffers[i++].show(*dev, rs::stream::depth, x, perTextureHeight, perTextureWidth, perTextureHeight, font);
            x += perTextureWidth;
        }

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
