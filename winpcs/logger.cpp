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
	logger.cpp for log file
*/
#include "logger.h"

src::severity_logger< severity_level >& logger::log_filter()
{
    static src::severity_logger< severity_level > slg;
    return slg;
}

logger& logger::init()
{
    typedef sinks::debug_output_backend backend_t;
    auto pDebugBackend = boost::make_shared< backend_t >();

    auto pDebugSink = boost::make_shared< sinks::synchronous_sink< backend_t > >(pDebugBackend);
    pDebugSink->set_filter(expr::attr<severity_level>("Severity") >= trace);
    pDebugSink->set_formatter(
        expr::stream
        << "[" << expr::attr<UINT>("RecordID")
        << "][" << expr::format_date_time(_timestamp, "%Y-%m-%d %H:%M:%S.%f")
        << "][" << _severity
        << "]" << expr::message
        << std::endl);
    logging::core::get()->add_sink(pDebugSink);

    sink_ = logging::add_file_log(
        keywords::open_mode = std::ios::app, // append write
        keywords::file_name = file_name_ + "%Y-%m-%d.log",
        keywords::rotation_size = rotation_size_,
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
        keywords::min_free_space = min_free_space_,

        keywords::format = (
            expr::stream
            << "[" << expr::attr<UINT>("RecordID")
            << "][" << expr::format_date_time(_timestamp, "%Y-%m-%d %H:%M:%S.%f")
            << "][" << _severity
            << "]" << expr::message)
        );

    sink_->locked_backend()->auto_flush(auto_flush_);

    logging::add_common_attributes();

    attrs::counter<UINT> RecordID(1);
    logging::core::get()->add_global_attribute("RecordID", RecordID);

    this->set_filter_trace();

    return *this;
}


logger& logger::start()
{
    logging::core::get()->set_logging_enabled(true);
    return *this;
}

logger& logger::stop()
{
    logging::core::get()->set_logging_enabled(false);
    return *this;
}


logger& logger::set_filter_trace()
{
    set_filter<trace>();
    return *this;
}

logger& logger::set_filter_warning()
{
    set_filter<warning>();
    return *this;
}

logger& logger::set_filter_error()
{
    set_filter<error>();
    return *this;
}

template <int level>
void logger::set_filter()
{
    sink_->set_filter(expr::attr<severity_level>("Severity") >= level);
}


logger& logger::set_logfile(const std::string& log_file)
{
    file_name_ = log_file;
    return *this;
}

logger& logger::set_min_free_space(const size_t size)
{
    min_free_space_ = size * 1024 * 1024;
    return *this;
}

logger& logger::set_rotation_size(const size_t size)
{
    rotation_size_ = size * 1024 * 1024;
    return *this;
}
