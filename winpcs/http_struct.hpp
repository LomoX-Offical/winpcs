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
	http_struct.hpp for gen the http response.
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

struct process_status
{
    std::string name;
    std::string process_name;
    std::string command;
    std::string directory;
    std::string environment;
    unsigned long pid;
    unsigned long exit_code;

    template<class Archive>
    void save(Archive & ar) const
    {
        std::string status = "exited";
        if ((pid != 0) && (this->exit_code == STILL_ACTIVE)) 
        {
            status = "running";
        }

        ar(
            CEREAL_NVP(name),
            CEREAL_NVP(process_name),
            CEREAL_NVP(command),
            CEREAL_NVP(directory),
            CEREAL_NVP(environment),
            CEREAL_NVP(pid),
            CEREAL_NVP(exit_code),
            CEREAL_NVP(status)
        );
    }
};
