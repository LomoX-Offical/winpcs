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

#include <cereal/cereal.hpp>

#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/base_class.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/map.hpp>

#include <cereal/archives/json.hpp>

extern "C" {
#include "libjsonnet/libjsonnet.h"
}

#pragma comment(lib, "libjsonnet.lib")


#define CEREAL_AR_NVP_DEFAULT(ar, name, value) \
	try { \
		ar(CEREAL_NVP(name));\
	}\
	catch (...) {\
		name = value;\
	}



struct process_config
{
	bool autostart;
	unsigned int autostart_delay_second;
	std::string command;
	std::string process_name;
	std::string directory;
	std::string environment;
	std::string stopsignal;
	std::vector<unsigned int> exitcodes;
	std::string name;
	unsigned int numprocs;
	unsigned int numprocs_start;
	
	std::string user;

	template<class Archive>
	void load(Archive & ar)
	{
		ar(CEREAL_NVP(name), CEREAL_NVP(command));

		CEREAL_AR_NVP_DEFAULT(ar, autostart, false);
		CEREAL_AR_NVP_DEFAULT(ar, process_name, "");
		CEREAL_AR_NVP_DEFAULT(ar, stopsignal, "%kill_tree%");
		CEREAL_AR_NVP_DEFAULT(ar, autostart_delay_second, 0);
		CEREAL_AR_NVP_DEFAULT(ar, exitcodes, std::vector<unsigned int>());
		CEREAL_AR_NVP_DEFAULT(ar, directory, "");
		CEREAL_AR_NVP_DEFAULT(ar, environment, "");
		CEREAL_AR_NVP_DEFAULT(ar, numprocs, 1);
		CEREAL_AR_NVP_DEFAULT(ar, numprocs_start, 0);
		CEREAL_AR_NVP_DEFAULT(ar, user, "system");
	}
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
			
		std::stringstream ss(json_str);
		cereal::JSONInputArchive ar(ss);
		ar(cereal::make_nvp("processes", processes_));

		return;
	}

	~parse_config()
	{

	}

	std::vector<process_config>& get_processes()
	{
		return processes_;
	}

private:

	std::string _parse_jsonnet(boost::filesystem::path& file, boost::system::error_code &ec)
	{
		int error;

		boost::shared_ptr<JsonnetVm> vm(jsonnet_make(), boost::bind(jsonnet_destroy, _1));
		boost::shared_array<char> output(
			jsonnet_evaluate_file(vm.get(), file.string().c_str(), &error), boost::bind(jsonnet_realloc, vm.get(), _1, 0));

		if (error != 0) {
			ec = boost::system::error_code(error,
				application_category(std::string(output.get())));
			return std::string();
		}

		return output.get();
	}

	application::context    &context_;
	boost::filesystem::path config_filename_;

	std::vector<process_config> processes_;

};