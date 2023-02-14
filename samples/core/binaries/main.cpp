/*
 * Copyright (c) 2020 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenCL SDK includes
#include <CL/Utils/File.hpp>
#include <CL/SDK/Context.hpp>
#include <CL/SDK/Options.hpp>
#include <CL/SDK/CLI.hpp>
#include <CL/SDK/Random.hpp>

// STL includes
#include <iostream>
#include <valarray>
#include <random>
#include <algorithm>
#include <fstream>
#include <tuple> // std::make_tuple
#include <numeric> // std::accumulate

// TCLAP includes
#include <tclap/CmdLine.h>

// Sample-specific option
struct BinariesOptions
{
    size_t start;
    size_t length;
};

// Add option to CLI parsing SDK utility
template <> auto cl::sdk::parse<BinariesOptions>()
{

    return std::make_tuple(std::make_shared<TCLAP::ValueArg<size_t>>(
                             "s", "start", "Starting number", false, 1,
                                "positive integral"),
                              std::make_shared<TCLAP::ValueArg<size_t>>(
                               "l", "length", "Length of input", false,
                               100000, "positive integral"));
}
template <>
BinariesOptions cl::sdk::comprehend<BinariesOptions>(
    std::shared_ptr<TCLAP::ValueArg<size_t>> start_arg,
    std::shared_ptr<TCLAP::ValueArg<size_t>> length_arg)
{
    return BinariesOptions{ start_arg->getValue(), length_arg->getValue() };
}

int main(int argc, char* argv[]) 
{
	try
	{
        // Parse command-line options
        auto opts =
            cl::sdk::parse_cli<cl::sdk::options::Diagnostic,
                               cl::sdk::options::SingleDevice, BinariesOptions>(
                argc, argv);
        const auto& diag_opts = std::get<0>(opts);
        const auto& dev_opts = std::get<1>(opts);
        const auto& binaries_opts = std::get<2>(opts);

        // Create context
        cl::Context context = cl::sdk::get_context(dev_opts.triplet);

        cl_int error;
        std::vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

        /// Try to read binary
        cl::Program::Binaries binaries =
            cl::util::read_binary_files(devices, "Collatz", &error);

        if (error != CL_SUCCESS)
        { // if binary not present, compile and save
            std::cout << "File not found" << "\n";

            std::string program_cl = cl::util::read_text_file("./Collatz.cl", &error);
            cl::Program program{context, program_cl};

            program.build(devices.at(0));

            binaries = program.getInfo<CL_PROGRAM_BINARIES>(&error);
            cl::util::write_binaries(binaries, devices, "Collatz");

        }

        // if the binary is already present - calculate
        std::cout << "File found or constructed properly!" << "\n";

        /// Create all remaining runtime objects
        cl::CommandQueue queue{ context, devices[0],
                                cl::QueueProperties::Profiling };

        cl::Platform platform{
            devices.at(0).getInfo<CL_DEVICE_PLATFORM>()
        }; // https://github.com/KhronosGroup/OpenCL-CLHPP/issues/150

        if (!diag_opts.quiet)
        {
            std::cout << "Selected platform: "
                      << platform.getInfo<CL_PLATFORM_VENDOR>() << "\n"
                      << "Selected device: " << devices.at(0).getInfo<CL_DEVICE_NAME>()
                      << "\n"
                      << std::endl;
        }

        cl::Program program{ context, devices, binaries };
        program.build(devices[0]);

        auto collatz = cl::KernelFunctor<cl::Buffer>(program, "Collatz");
        const size_t length = binaries_opts.length;
        const size_t start = binaries_opts.start - 1;

        /// Prepare vector of values to extract results
        std::vector<cl_int> v(length);
        
        /// Initialize device-side storage
        cl::Buffer buf{ context, std::begin(v), std::end(v), false };

        /// Run kernel
        if (diag_opts.verbose)
        {
            std::cout << "Executing on device... ";
            std::cout.flush();
        }
        std::vector<cl::Event> pass;
        pass.push_back(collatz(
            cl::EnqueueArgs{ queue, cl::NDRange{ start, length } }, buf));

        cl::WaitForEvents(pass);

        cl::copy(queue, buf, std::begin(v), std::end(v));

        return 0;

	} catch (cl::BuildError& e)
    {
        std::cerr << "OpenCL runtime error: " << e.what() << std::endl;
        for (auto& build_log : e.getBuildLog())
        {
            std::cerr << "\tBuild log for device: "
                      << build_log.first.getInfo<CL_DEVICE_NAME>() << "\n"
                      << std::endl;
            std::cerr << build_log.second << "\n" << std::endl;
        }
        std::exit(e.err());
    } catch (cl::Error& e)
    {
        std::cerr << "OpenCL rutnime error: " << e.what() << std::endl;
        std::exit(e.err());
    } catch (std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return 0;
}