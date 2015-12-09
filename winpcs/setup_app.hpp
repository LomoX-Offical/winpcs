/*
  Copyright (c) 2015, Ben Cheung (zqzjz1982@gmail.com)
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of winpcs nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES AND SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
	setup_app.hpp for running as a service setup program.
*/

#pragma once

#include "config.hpp"
#include "service_setup.hpp"

class setup_app
{
public:

	setup_app(boost::application::context& context)
		: context_(context)
	{
	}

	int operator()()
	{
		ns::shared_ptr<boost::application::path> path
			= context_.find<boost::application::path>();

		ns::shared_ptr<boost::application::args> cmd_args
			= context_.find<boost::application::args>();

		// define our simple installation schema options
		po::options_description options("service options");
		options.add_options()
			("help", "produce a help message")
			(",i", "install service")
			(",c", "check service")
			(",u", "unistall service")
			(",s", "start service")
			(",e", "stop service")
			("config", po::value<std::string>()->default_value("winpcs.conf"), "config file (optional, installation only)")
			("name", po::value<std::string>()->default_value("winpcs"), "service name (optional, installation only)")
			("user", po::value<std::string>()->default_value(""), "user logon (optional, installation only)")
			("pass", po::value<std::string>()->default_value(""), "user password (optional, installation only)")
			("display", po::value<std::string>()->default_value("processes control system for windows"), "service display name (optional, installation only)")
			("description", po::value<std::string>()->default_value("service for winpcs"), "service description (optional, installation only)")
			("start", po::value<std::string>()->default_value("auto"), "service start mode (optional, installation only, auto or manaul, default is auto)")
			("depends", po::value<std::string>()->default_value(""), "other services of this service depended, multiple services using '\\' separate  (optional, installation only)")
			;

		po::variables_map vm;
		po::store(po::parse_command_line(cmd_args->argc(), cmd_args->argv(), options), vm);
		boost::system::error_code ec;

		if (vm.count("help"))
		{
			std::cout << options << std::endl;
			return true;
		}

		if (vm.count("-i"))
		{
			std::string config = vm["config"].as<std::string>();

			std::string service_options = std::string(" -b ");

			if (config.size()) 
			{
				service_options += std::string(" --config=") + config;
			}

			install_windows_service(
				setup_arg(vm["name"].as<std::string>()),
				setup_arg(vm["display"].as<std::string>()),
				setup_arg(vm["description"].as<std::string>()),
				setup_arg(path->executable_path_name()),
				setup_arg(vm["config"].as<std::string>()),
				setup_arg(vm["user"].as<std::string>()),
				setup_arg(vm["pass"].as<std::string>()),
				setup_arg(vm["start"].as<std::string>()),
				setup_arg(vm["depends"].as<std::string>()),
				setup_arg(service_options)).install(ec);

			std::cout << ec.message() << std::endl;

			return true;
		}

		if (vm.count("-c"))
		{
			bool exist =
				check_windows_service(
					setup_arg(vm["name"].as<std::string>())).exist(ec);

			if (ec)
				std::cout << ec.message() << std::endl;
			else
			{
				if (exist)
					std::cout
					<< "The service "
					<< vm["name"].as<std::string>()
					<< " is installed!"
					<< std::endl;
				else
					std::cout
					<< "The service "
					<< vm["name"].as<std::string>()
					<< " is NOT installed!"
					<< std::endl;
			}

			return true;
		}

		if (vm.count("-s"))
		{
			bool start =
				start_windows_service(
					setup_arg(vm["name"].as<std::string>())).start(ec);

			if (ec)
				std::cout << ec.message() << std::endl;
			else
				std::cout << "start " << vm["name"].as<std::string>() << " success !"  << std::endl;

			return true;
		}

		if (vm.count("-e"))
		{
			bool stop =
				stop_windows_service(
					setup_arg(vm["name"].as<std::string>())).stop(ec);

			if (ec)
				std::cout << ec.message() << std::endl;
			else
				std::cout << "stop " << vm["name"].as<std::string>() << " success !" << std::endl;

			return true;
		}

		if (vm.count("-u"))
		{
			uninstall_windows_service(
				setup_arg(vm["name"].as<std::string>()),
				setup_arg(path->executable_full_name())).uninstall(ec);

			std::cout << ec.message() << std::endl;

			return true;
		}

		std::cout << options << std::endl;
		return true;
	}

private:
    boost::application::context& context_;

};
