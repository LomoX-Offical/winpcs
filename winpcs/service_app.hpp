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
	service_app.hpp for running as a service program.
*/

#pragma once
#include "config.hpp"

#include <boost/application/auto_handler.hpp>
#include <boost/filesystem.hpp>

#include "timer.hpp"
#include "process_manager.hpp"
#include "parse_config.hpp"


class service_app
{
public:

	service_app(boost::application::context& context)
		: context_(context)
	{
        boost::application::handler<>::callback termination_callback
			= boost::bind(&service_app::stop, this);

		context_.insert<boost::application::termination_handler>(
            boost::application::csbl::make_shared<boost::application::termination_handler_default_behaviour>(termination_callback));
	}

	int operator()()
	{
		WRITE_LOG(trace) << "server running";

		ns::shared_ptr<boost::application::args> cmd_args
			= context_.find<boost::application::args>();


		// define our simple installation schema options
		po::options_description install("service options");
		install.add_options()
			("config", po::value<std::string>()->default_value("winpcs.conf"), "");

		po::variables_map vm;
		po::store(po::parse_command_line_allow_unregistered(cmd_args->argc(), cmd_args->argv(), install), vm);
		boost::system::error_code ec;

		ns::shared_ptr<boost::application::path> application_path
			= context_.find<boost::application::path>();

		boost::filesystem::current_path(application_path->executable_path());
		boost::filesystem::path config_file(vm["config"].as<std::string>());
		if (config_file.is_relative()) 
		{
			config_file = boost::filesystem::current_path();
			config_file += boost::filesystem::path::preferred_separator;
			config_file += vm["config"].as<std::string>();
		}

		WRITE_LOG(trace) << "config file:" << config_file;

		config_.reset(new parse_config(context_, config_file, ec));
		if (ec)
		{
			WRITE_LOG(error) << "config file load failed! >> " << ec.message();
			return 1;
		}


		timer_.start();

		psmgr_.start(config_->get_processes(), timer_, ec);

//         http_.start(ec);


		WRITE_LOG(trace) << "server wait for termination request..";

		context_.find<boost::application::wait_for_termination_request>()->wait();

//         http_.stop();

		psmgr_.stop();

		timer_.stop();

		WRITE_LOG(trace) << "server exiting..";

		return 0;
	}

	bool stop()
	{
		timer_.stop();

		WRITE_LOG(trace) << "server stopping";
		return true;
	}

	bool pause()
	{
		WRITE_LOG(trace) << "server pausing";
		return false;
	}

	bool resume()
	{
		WRITE_LOG(trace) << "server resuming";
		return false;
	}

private:

    boost::application::context                  &context_;
	timer_generator                        timer_;
	ns::shared_ptr<parse_config>           config_;
	process_manager                        psmgr_;
//     http_server                            http_;
	boost::shared_ptr<boost::thread>       http_thread_;
};
