#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <CL/cl2.hpp>

#include <chrono>
#include <numeric>
#include <iterator>

#include <vector>       // std::vector
#include <string>
#include <exception>    // std::runtime_error, std::exception
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <random>       // std::default_random_engine, std::uniform_real_distribution
#include <algorithm>    // std::transform
#include <cstdlib>      // EXIT_FAILURE
#include <iomanip>
#include <math.h>
#include <windows.h>

struct rawcolor{ unsigned char r, g, b, a; };
struct rawcolor3{ unsigned char r, g, b; };
struct color   { float         r, g, b, a; };

int main()
{
    try
    {
        // Checking the device info
        cl::CommandQueue queue = cl::CommandQueue::getDefault();
        cl::Device device = queue.getInfo<CL_QUEUE_DEVICE>();
        cl::Context context = queue.getInfo<CL_QUEUE_CONTEXT>();

        // Load program source
        std::ifstream file2{ "C:/Users/haffn/Desktop/MSc-III/GPU-II/Projects/second project/final/rot_res_gam.cl" };

        // if program failed to open
        if (!file2.is_open())
            throw std::runtime_error{ std::string{ "Cannot open kernel source: " } + "rot_res_gam.cl" };

        // Creating the program
        cl::Program program{ std::string{ std::istreambuf_iterator<char>{ file2 }, std::istreambuf_iterator<char>{} } };

        //Building the program
        program.build({ device });

        // Creating the kernel
        cl:: Kernel rotation(program, "rotation");
        cl:: Kernel resize_gamma(program, "resize_gamma");

// ############################################################################
// Declaring some variables

        float angle;
        float gamma_val;
        int chanels = 0;

        int height = 0;
        int width = 0;
        int res_new_width;
        int res_new_height;

// Getting the inputs from the user
        
        std::cout << std::endl <<"Please enter the angle of rotation in degrees! \n";
        std::cin >> angle;

        std::cout << std::endl << "Please enter the new width! \n";
        std::cin >> res_new_width;
        std::cout << std::endl << "Please enter the new height! \n";
        std::cin >> res_new_height;

        std::cout << std::endl <<"Please enter the value of the gamma correction! \n";
        std::cin >> gamma_val;

// Printing the new values on the consol

        std::cout << std::endl << "The degree of rotation is: "  << angle;
        std::cout << std::endl <<"The new sizes are: " << res_new_width << "x" << res_new_height;     
        std::cout << std::endl<< "The value of the gamma correction is: " << gamma_val << std::endl;

// ############################################################################
// Getting the name of the input file:

        static const std::string input_filename = "C:/Users/haffn/Desktop/MSc-III/GPU-II/Projects/second project/nice.jpg";

// Loading the image:

        rawcolor* data0 = reinterpret_cast<rawcolor*>(stbi_load(input_filename.c_str(), &width, &height, &chanels, 4 /* we expect 4 components */));
        if(!data0)
        {
            std::cout << "Error: could not open input file: " << input_filename << "\n";
            return -1;
        }
        else
        {
            std::cout << std::endl << "Image opened successfully." << "\n";
        }

        std::vector<color> input(width*height);

// Transfrming the image according to the channels

        if(chanels == 4)
        {
            std::transform(data0, data0+width*height, input.begin(), [](rawcolor c){ return color{c.r/255.0f, c.g/255.0f, c.b/255.0f, c.a/255.0f}; } );
        }
        else if(chanels == 3)
        {
            std::transform(data0, data0+width*height, input.begin(), [](rawcolor c){ return color{c.r/255.0f, c.g/255.0f, c.b/255.0f, 1.0f}; } );
        }
        stbi_image_free(data0);

// ############################################################################ 
// Declaring some variables

        float pi = 3.1415926535897932f;
        float angle_rad = pi/180.0f*angle;

// Calculating the new image sizes and centers:

        int rot_new_height = (int)std::round(std::abs(height*cos(angle_rad)) + std::abs(width*sin(angle_rad)));
        int rot_new_width = (int)std::round(std::abs(width*cos(angle_rad)) + std::abs(height*sin(angle_rad)));

        size_t width_size = width;
        size_t height_size = height;
        size_t new_width_size = rot_new_width;
        size_t new_height_size = rot_new_height;
        size_t res_width_size = res_new_width;
        size_t res_height_size = res_new_height;

// ############################################################################
// Crating the texture vector

        std::vector<cl::Image2D> textures(3);

// Creating the formats and declaring the image channel types

        cl::ImageFormat Format1;
        cl::ImageFormat Format2;
        cl::ImageFormat Format3;

        Format1.image_channel_data_type = CL_FLOAT;
        Format2.image_channel_data_type = CL_FLOAT;
        Format3.image_channel_data_type = CL_FLOAT;

        Format1.image_channel_order = CL_RGBA;
        Format2.image_channel_order = CL_RGBA;
        Format3.image_channel_order = CL_RGBA;

// Creating the textures with the appropriate data

        std::vector<color> output1(rot_new_width*rot_new_height);
        std::vector<color> output2(res_new_width*res_new_height);

        textures[0] = cl::Image2D(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, Format1, width_size, height_size, 0, input.data(), nullptr);
        textures[1] = cl::Image2D(context, CL_MEM_READ_WRITE, Format2, new_width_size, new_height_size, 0, nullptr, nullptr);
        textures[2] = cl::Image2D(context, CL_MEM_WRITE_ONLY, Format3, res_width_size, res_height_size, 0, nullptr, nullptr);

        cl::Sampler sampler = cl::Sampler(context, CL_FALSE, CL_ADDRESS_CLAMP, CL_FILTER_NEAREST);

        std::array<cl::size_type, 3> origin = {0, 0, 0};
        std::array<cl::size_type, 3> dims1 = {new_width_size, new_height_size, 1};
        std::array<cl::size_type, 3> dims2 = {res_width_size, res_height_size, 1};

// ############################################################################
// Setting the arguments of the rotation kernel

        rotation.setArg(0, textures[0]);
        rotation.setArg(1, textures[1]);
        rotation.setArg(2, sampler);
        rotation.setArg(3, height);
        rotation.setArg(4, width);
        rotation.setArg(5, rot_new_height);
        rotation.setArg(6, rot_new_width);
        rotation.setArg(7, angle_rad);

// Setting the argument of the resize_gamma kernel

        resize_gamma.setArg(0, textures[1]);
        resize_gamma.setArg(1, textures[2]);
        resize_gamma.setArg(2, sampler);
        resize_gamma.setArg(3, rot_new_width);
        resize_gamma.setArg(4, rot_new_height);
        resize_gamma.setArg(5, res_new_width);
        resize_gamma.setArg(6, res_new_height);
        resize_gamma.setArg(7, gamma_val);

// Calling the kernels

        queue.enqueueNDRangeKernel(rotation, cl::NullRange, cl::NDRange(new_width_size, new_height_size));
        cl::finish();
        queue.enqueueReadImage(textures[1], CL_TRUE, origin, dims1, 0, 0, output1.data(), 0, nullptr);

        queue.enqueueNDRangeKernel(resize_gamma, cl::NullRange, cl::NDRange(res_width_size, res_height_size));
        cl::finish();
        queue.enqueueReadImage(textures[2], CL_TRUE, origin, dims2, 0, 0, output2.data(), 0, nullptr);

// ############################################################################
// Creating the output data and saving the image

        std::vector<rawcolor> tmp(res_new_width*res_new_height*4);
        std::transform(output2.cbegin(), output2.cend(), tmp.begin(), 
        [](color c) {return rawcolor{ (unsigned char)(c.r*255.0f),
                                      (unsigned char)(c.g*255.0f),
                                      (unsigned char)(c.b*255.0f),
                                      (unsigned char)(1.0f*255.0f) }; });

        int res = stbi_write_png("result.png", res_new_width, res_new_height, 4, tmp.data(), res_new_width*4);
        if (res == 0)
        {
            std::cout << "Error writing utput to file\n";
        }
        else
        {
            std::cout << "Output written to file\n";
        }        

// ############################################################################
// If kernel failed to build

    }
    catch (cl::BuildError& error) 
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;

        for (const auto& log : error.getBuildLog())
        {
            std::cerr <<
                "\tBuild log for device: " <<
                log.first.getInfo<CL_DEVICE_NAME>() <<
                std::endl << std::endl <<
                log.second <<
                std::endl << std::endl;
        }

        std::exit(error.err());
    }
    catch (cl::Error& error) // If any OpenCL error occurs
    {
        std::cerr << error.what() << "(" << error.err() << ")" << std::endl;
        std::exit(error.err());
    }
    catch (std::exception& error) // If STL/CRT error occurs
    {
        std::cerr << error.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

