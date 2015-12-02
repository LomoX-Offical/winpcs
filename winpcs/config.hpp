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
	config.hpp for define of macro, type and namespace.
*/

#pragma once

// for shared ptr
#define BOOST_APPLICATION_FEATURE_NS_SELECT_BOOST


#include <boost/smart_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/application.hpp>

#ifdef BOOST_APPLICATION_FEATURE_NS_SELECT_BOOST
namespace ns = boost;
#else
namespace ns = std;
#endif

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace boost { namespace program_options {
	template<class charT>
	basic_parsed_options<charT>
		parse_command_line_allow_unregistered(int argc, const charT* const argv[],
			const options_description& desc,
			int style = 0,
			function1<std::pair<std::string, std::string>,
			const std::string&> ext
			= ext_parser())
	{
		return basic_command_line_parser<charT>(argc, argv).options(desc).allow_unregistered().
			style(style).extra_parser(ext).run();
	}

}}

#include <boost/system/error_code.hpp>
#include "application_category.hpp"

#include "logger.hpp"

#include <boost/thread.hpp>

typedef boost::try_mutex MUTEX;
typedef MUTEX::scoped_lock LOCK;
typedef MUTEX::scoped_try_lock TRY_LOCK;

typedef boost::recursive_try_mutex RECURSIVE_MUTEX;
typedef RECURSIVE_MUTEX::scoped_lock RECURSIVE_LOCK;
typedef RECURSIVE_MUTEX::scoped_try_lock RECURSIVE_TRY_LOCK;

#include <functional>

template <typename F>
struct ScopeExit {
	ScopeExit(F f) : f(f) {}
	~ScopeExit() { f(); }
	F f;
};

template <typename F>
ScopeExit<F> MakeScopeExit(F f) {
	return ScopeExit<F>(f);
};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SCOPE_EXIT(code) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})
