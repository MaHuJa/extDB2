/*
Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#pragma once

#include <boost/crc.hpp>
#include <boost/thread/thread.hpp>

#include <Poco/ClassLibrary.h>
#include <Poco/MD4Engine.h>
#include <Poco/MD5Engine.h>

#include "abstract_protocol.h"


class MISC: public AbstractProtocol
{
	public:
		bool init(AbstractExt *extension, const std::string &database_id, const std::string init_str);
		bool callProtocol(std::string input_str, std::string &result, const int unique_id=-1);

	private:
		Poco::MD5Engine md5;
		std::mutex mutex_md5;

		Poco::MD4Engine md4;
		std::mutex mutex_md4;

	    boost::crc_32_type crc32;
		std::mutex mutex_crc32;

		std::mutex mutex_RandomString;
		std::vector < std::string > uniqueRandomVarNames;

		void getDateTime(std::string &result);
		void getDateTime(int hours, std::string &result);

		void getCrc32(std::string &input_str, std::string &result);
		void getMD4(std::string &input_str, std::string &result);
		void getMD5(std::string &input_str, std::string &result);
		void getBEGUID(std::string &input_str, std::string &result);

		void getRandomString(std::string &input_str, bool uniqueString, std::string &result);
};
