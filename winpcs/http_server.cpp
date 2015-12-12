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
	http_server.cpp for services of http server.
*/

#include "http_server.h"

void http_server::start(process_manager& pm, boost::system::error_code &ec)
{
    if (impl_.get() != nullptr) {
        return;
    }

    impl_.reset(new cinatra::Cinatra<>);

    impl_->route("/status/pid/:pid", [this, &pm](cinatra::Request& /* req */, cinatra::Response& res, int pid)
    {
        auto status = pm.status(pid);

        std::ostringstream ss;
        {
            cereal::JSONOutputArchive ar(ss);
            ar(cereal::make_nvp("status", status));
        }

        res.end(ss.str());
        return;
    });

    impl_->route("/start", [](cinatra::Request& /* req */, cinatra::Response& res)
    {
        res.end("{\"result\":0}");
        return;
    });

    impl_->route("/stop", [](cinatra::Request& /* req */, cinatra::Response& res)
    {
        res.end("{\"result\":0}");
        return;
    });

    impl_->route("/restart", [](cinatra::Request& /* req */, cinatra::Response& res)
    {
        res.end("{\"result\":0}");
        return;
    });

    impl_->listen("http");

    thread_.reset(new boost::thread([this]()
    {
        this->impl_->run();
    }));
}

void http_server::stop()
{
    if (impl_.get() == nullptr) {
        return;
    }

    impl_->stop();
    impl_.reset();
}
