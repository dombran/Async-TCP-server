/*
 * stdafx.h
 *
 *  Created on: Feb 24, 2021
 *      Author: dombran
 */

#ifndef STDAFX_H_
#define STDAFX_H_

#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>


#include <thread>
#include <set>
#include <map>
#include <iostream>
#include <deque>
#include <mutex>

#include <fstream>
#include <iostream>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <vector>
#include <algorithm>
#include <chrono>
#include <list>
#include <time.h>

#include <gsm.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <dirent.h>

#include <boost/current_function.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/crc.hpp>
#include <boost/signals2.hpp>

#include <boost/shared_array.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/date_parsing.hpp>

#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/date_parsing.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/parse_format_base.hpp>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind/bind.hpp>

#include <boost/filesystem.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>

#include <boost/convert.hpp>
#include <boost/convert/stream.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/system/linux_error.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#endif /* STDAFX_H_ */
