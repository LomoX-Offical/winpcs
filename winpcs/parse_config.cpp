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
	parse_config.cpp for parsing the config file.
*/

#include "parse_config.h"


extern "C" {
#include "libjsonnet/libjsonnet.h"
}


parse_config::parse_config(boost::filesystem::path file, boost::system::error_code &ec)
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

parse_config::~parse_config()
{

}

std::vector<process_config>& parse_config::get_processes()
{
    return processes_;
}

std::string parse_config::_parse_jsonnet(boost::filesystem::path& file, boost::system::error_code &ec)
{
    int error;

    boost::shared_ptr<JsonnetVm> vm(jsonnet_make(), boost::bind(jsonnet_destroy, _1));
    boost::shared_array<char> output(
        jsonnet_evaluate_file(vm.get(), file.string().c_str(), &error), boost::bind(jsonnet_realloc, vm.get(), _1, 0));

    if (error != 0) {
        ec = boost::system::error_code(error,
            make_app_cat(std::string(output.get())));
        return std::string();
    }

    return output.get();
}
