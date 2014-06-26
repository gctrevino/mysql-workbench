/* 
 * Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysql_parser_services.h"

#include "base/string_utilities.h"

using namespace parser;

//------------------ ParserContext -----------------------------------------------------------------

ParserContext::ParserContext(GrtCharacterSetsRef charsets, GrtVersionRef version, bool case_sensitive)
{
  _version = version;
  _case_sensitive = case_sensitive;

  std::set<std::string> filtered_charsets;
  for (size_t i = 0; i < charsets->count(); i++)
    filtered_charsets.insert(base::tolower(*charsets[i]->name()));

  // 3 character sets were added in version 5.5.3. Remove them from the list if the current version
  // is lower than that.
  ssize_t server_version;
  if (version.is_valid())
  {
    server_version = version->majorNumber() * 10000;
    if (version->minorNumber() > -1)
      server_version += version->minorNumber() * 100;
    else
      server_version += 500;
    if (version->releaseNumber() > -1)
      server_version += version->releaseNumber();
  }
  else
    server_version = 50501; // Assume some reasonable default (5.5.1).
  if (server_version < 50503)
  {
    filtered_charsets.erase("utf8mb4");
    filtered_charsets.erase("utf16");
    filtered_charsets.erase("utf32");
  }
  
  _recognizer = new MySQLRecognizer((long)server_version, "", filtered_charsets);
}

//--------------------------------------------------------------------------------------------------

ParserContext::~ParserContext()
{
  delete _recognizer;
}

//--------------------------------------------------------------------------------------------------

void ParserContext::use_sql_mode(const std::string &mode)
{
  _sql_mode = mode;
  _recognizer->set_sql_mode(mode);
}

//--------------------------------------------------------------------------------------------------

std::string ParserContext::get_sql_mode()
{
  return _sql_mode;
}

//--------------------------------------------------------------------------------------------------

void ParserContext::use_server_version(GrtVersionRef version)
{
  _version = version;
  
  ssize_t server_version;
  if (version.is_valid())
  {
    server_version = version->majorNumber() * 10000;
    if (version->minorNumber() > -1)
      server_version += version->minorNumber() * 100;
    else
      server_version += 500;
    if (version->releaseNumber() > -1)
      server_version += version->releaseNumber();
  }
  else
    server_version = 50501; // Assume some reasonable default (5.5.1).
  _recognizer->set_server_version((long)server_version);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a collection of errors from the last parser run. The start position is offset by the given
 * value (used to adjust error position in a larger context).
 */
std::vector<ParserErrorEntry> ParserContext::get_errors_with_offset(size_t offset)
{
  std::vector<ParserErrorEntry> errors;

  if (_recognizer->has_errors())
  {
    const std::vector<MySQLParserErrorInfo> error_info = _recognizer->error_info();
    for (std::vector<MySQLParserErrorInfo>::const_iterator error_iterator = error_info.begin();
         error_iterator != error_info.end(); ++error_iterator)
    {
      ParserErrorEntry entry = {error_iterator->message, error_iterator->charOffset + offset,
        error_iterator->line, error_iterator->length};
      errors.push_back(entry);
    }
  }

  return errors;
}

//------------------ MySQLParserServices -----------------------------------------------------------

ParserContext::Ref MySQLParserServices::createParserContext(GrtCharacterSetsRef charsets,
                                                            GrtVersionRef version, bool case_sensitive)
{
  boost::shared_ptr<ParserContext> result(new ParserContext(charsets, version, case_sensitive));

  return result;
}

//--------------------------------------------------------------------------------------------------

MySQLParserServices::Ref MySQLParserServices::get(grt::GRT *grt)
{
  MySQLParserServices::Ref module = dynamic_cast<MySQLParserServices::Ref>(grt->get_module("MySQLParserServices"));
  if (!module)
		throw std::runtime_error("Can't get MySQLParserServices module.");
  return module;
}

//--------------------------------------------------------------------------------------------------
