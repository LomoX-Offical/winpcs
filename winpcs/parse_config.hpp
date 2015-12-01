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
	parse_config.hpp for parsing the config file.
*/

#pragma once

#include "config.hpp"

extern "C" {
#include "libjsonnet/libjsonnet.h"
}

#pragma comment(lib, "libjsonnet.lib")


struct process
{
	bool autostart;
	std::string command;
	std::string directory;
	std::string environment;
	std::vector<unsigned int> exitcodes;
	std::string name;
	unsigned int numprocs;
	unsigned int numprocs_start;
	std::string user;

};

class parse_config : noncopyable
{
public:
	parse_config(application::context &context, boost::filesystem::path file, boost::system::error_code &ec)
		: context_(context)
	{
		std::string json_str(_parse_jsonnet(file, ec));
		if (ec)
		{
			return;
		}

	}

	~parse_config()
	{

	}

private:

	std::string _parse_jsonnet(boost::filesystem::path& file, boost::system::error_code &ec)
	{
		int error;
		std::string ret;

		boost::shared_ptr<JsonnetVm> vm(jsonnet_make(), boost::bind(jsonnet_destroy, _1));
		boost::shared_array<char> output(
			jsonnet_evaluate_file(vm.get(), file.string().c_str(), &error), boost::bind(jsonnet_realloc, vm.get(), _1, 0));

		if (error != 0) {
			ec = boost::system::error_code(error,
				application_category(std::string(output.get())));
			return ret;
		}

		ret = output.get();
		return ret;
	}
	application::context    &context_;
	boost::filesystem::path config_filename_;
};