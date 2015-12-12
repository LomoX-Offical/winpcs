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
	logger.h for log file
*/

#pragma once  

#include "config.hpp"
#include <cassert>  
#include <iostream>  
#include <fstream>  
#include <boost/date_time/posix_time/posix_time_types.hpp>  

#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>  
#include <boost/log/utility/setup/file.hpp>  
#include <boost/log/utility/setup/console.hpp>  
#include <boost/log/utility/setup/common_attributes.hpp>  
#include <boost/log/sources/logger.hpp>  
#include <boost/log/support/date_time.hpp>  

#include <boost/filesystem.hpp>   
#include <boost/log/detail/thread_id.hpp>   
#include <boost/log/sources/global_logger_storage.hpp>  
#include <boost/log/sinks/debug_output_backend.hpp>

namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;


enum severity_level
{
	trace,
	warning,
	error
};

template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
{
	static const char* const str[] =
	{
		"trace",
		"warning",
		"error"
	};
	if (static_cast<std::size_t>(lvl) < (sizeof(str) / sizeof(*str)))
		strm << str[lvl];
	else
		strm << static_cast<int>(lvl);
	return strm;
}

BOOST_LOG_ATTRIBUTE_KEYWORD(_severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(_timestamp, "TimeStamp", boost::posix_time::ptime)



class logger : boost::noncopyable
{

private:
	logger(void)
	{
	}


public:
    static logger& instance()
    {
        static logger ins;
        return ins;
    }

    logger& stop();
    logger& start();
    logger& init();

    logger& set_min_free_space(const size_t size);
    logger& set_rotation_size(const size_t size);
    logger& set_logfile(const std::string& log_file);
    static src::severity_logger< severity_level >& log_filter();


    template <int level>
    void set_filter();
    logger& set_filter_warning();
    logger& set_filter_error();
    logger& set_filter_trace();




protected:
	size_t min_free_space_;
	size_t rotation_size_;
	std::string file_name_;
	bool auto_flush_;
	boost::shared_ptr< sinks::synchronous_sink< sinks::text_file_backend > > sink_;
};

#define WRITE_LOG(level) BOOST_LOG_SEV(logger::log_filter(), level)