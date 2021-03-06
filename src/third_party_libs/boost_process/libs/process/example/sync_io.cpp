// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/process.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <string>

using namespace boost::process;
using namespace boost::process::initializers;
using namespace boost::iostreams;

int main()
{
//[sync_io
    boost::process::pipe p = create_pipe();

    file_descriptor_sink sink(p.sink, close_handle);
    execute(
        run_exe("test.exe"),
        bind_stdout(sink)
    );

    file_descriptor_source source(p.source, close_handle);
    stream<file_descriptor_source> is(source);
    std::string s;
    std::getline(is, s);
//]
}
