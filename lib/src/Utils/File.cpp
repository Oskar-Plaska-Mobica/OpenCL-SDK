// OpenCL SDK includes
#include <CL/Utils/File.hpp>

// STL includes
#include <fstream>
#include <iterator>
#include <algorithm>
#include <iostream>

std::string cl::util::read_text_file(const char* const filename,
    cl_int* const error)
{
    std::ifstream in(filename);
    if (in.good())
    {
        try
        {
            std::string red((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());
            if (in.good() && in.eof())
            {
                if (error != nullptr) *error = CL_SUCCESS;
                return red;
            }
            else
            {
                detail::errHandler(CL_UTIL_FILE_OPERATION_ERROR, error,
                                   "File read error!");
                return std::string();
            }
        } catch (std::bad_alloc& ex)
        {
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::string();
        }
    }
    else
    {
        detail::errHandler(CL_INVALID_VALUE, error, "No file!");
        return std::string();
    }
}

std::vector<unsigned char> cl::util::read_binary_file(const char* const filename,
                                            cl_int* const error)
{
    std::ifstream in(filename, std::ios::binary);
    if (in.good())
    {
        try
        {
            std::vector<unsigned char> buffer(
                std::istreambuf_iterator<char>(in), {});
            return buffer;
        } catch (std::bad_alloc& ex)
        {
            detail::errHandler(CL_OUT_OF_RESOURCES, error, "Bad allocation!");
            return std::vector<unsigned char>();
        }
    }
    else
    {
        detail::errHandler(CL_INVALID_VALUE, error, "No file!");
        return std::vector<unsigned char>();
    }
}


cl::Program::Binaries cl::util::read_binary_files(const std::vector<cl::Device>& devices,
                            const char* const program_file_name,
                            cl_int* const error)
{
    cl::Program::Binaries binaries(0);
    size_t num_devices = devices.size();
    std::cout << num_devices << "\n" << std::endl;
    for (const auto& device : devices)
    {
        string device_name = device.getInfo<CL_DEVICE_NAME>();
        string binary_name = string(program_file_name) + "-" + device_name + ".bin";

        std::cout << binary_name << "\n" << std::endl;
        binaries.push_back(cl::util::read_binary_file(binary_name.c_str(), error));
        if (*error == CL_INVALID_VALUE) 
            return binaries;
    }
    *error = CL_SUCCESS;
    return binaries;
}