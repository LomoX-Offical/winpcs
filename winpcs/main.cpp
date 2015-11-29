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
	main.cpp for setup the service
*/


#include "config.hpp"

#include "setup_app.hpp"
#include "service_app.hpp"

int main(int argc, char *argv[])
{
	try
	{
		application::context app_context;
		app_context.insert<args>(
			ns::make_shared<application::args>(argc, argv));

		app_context.insert<application::path>(
			boost::make_shared<application::path>());


		ns::shared_ptr<application::path> path = app_context.find<application::path>();
		logger::instance().set_logfile(path->executable_path_name().string());
		logger::instance().init();

		po::variables_map vm;
		po::options_description desc;

		desc.add_options()
			(",b", "run on backend service")
			;
		po::store(po::parse_command_line_allow_unregistered(argc, argv, desc), vm);

		if (vm.count("-b"))
		{
			service_app app(app_context);
			return application::launch<application::server>(app, app_context);
		}

		setup_app app(app_context);
		return application::launch<application::common>(app, app_context);
	}
	catch (boost::system::system_error& se)
	{
		std::cerr << se.what() << std::endl;
		WRITE_LOG(error) << se.what();
		return 1;
	}
	catch (std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		WRITE_LOG(error) << e.what();
		return 1;
	}
	catch (...)
	{
		std::cerr << "Unknown error." << std::endl;
		WRITE_LOG(error) << "Unknown error.";
		return 1;
	}

	return 0;
}