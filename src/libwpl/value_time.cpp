/*

-------------------------------------------------------------

Copyright (c) MMXIII Atle Solbakken
atle@goliathdns.no

-------------------------------------------------------------

This file is part of P* (P-star).

P* is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

P* is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with P*.  If not, see <http://www.gnu.org/licenses/>.

-------------------------------------------------------------

*/

#include "value_time.h"
#include "operator.h"
#include "value_int.h"
#include "value_string.h"

#include <ctime>
#include <cstring>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

void wpl_value_time::set_weak (wpl_value *value) {
	/*
	   TODO
	   Support setting from string and int
	   */
	wpl_value_time *value_time = dynamic_cast<wpl_value_time*>(value);
	if (value_time == NULL) {
        wpl_value_string * value_string = dynamic_cast<wpl_value_string*>(value);
        if (value_string) {
            try_guess_from_str(value_string->toString());
        } else {
            ostringstream tmp;
            tmp << "Could not set TIME object to value of type " << value->get_type_name();
            throw runtime_error(tmp.str());
        }
	}
	else {
		set_time(value_time->get_time());
		set_format(value_time->get_format());
	}
}

string wpl_value_time::toString() {
	string tmp;
	format_time(NULL, tmp);
	return tmp;
}

int wpl_value_time::do_operator (
		wpl_expression_state *exp_state,
		wpl_value *final_result,
		const wpl_operator_struct *op,
		wpl_value *lhs,
		wpl_value *rhs
) {
	if (op == &OP_ELEMENT) {
		const char *cmd = rhs->toString().c_str();

		/*
		   Commands which return self
		 */
		if (strcmp (cmd, "set_now") == 0) {
			set_now();
			return do_operator_recursive(exp_state, final_result);
		}

		/*
		   Commands which return strings
		 */
		{
			wpl_value_string result("");
			bool ok = true;

			if (strcmp (cmd, "rfc2822") == 0) {
				format_time("%a, %d %b %Y %T %z", result.get_internal_string());
			}
			else if (strcmp (cmd, "iso8601") == 0) {
				format_time("%Y-%m-%dT%H:%M:%S%z", result.get_internal_string());
			}
			else {
				ok = false;
			}

			if (ok) {
				return result.do_operator_recursive(exp_state, final_result);
			}
		}

		/*
		   Commands which return integers
		 */
		{
			time_t t = get_time();
			struct tm time_tm;
			localtime_r(&t, &time_tm);

			wpl_value_int result(0);
			bool ok = true;

			if (strcmp (cmd, "year") == 0) {
				result.set(time_tm.tm_year + 1900);
			}
			else if (
				(strcmp (cmd, "month") == 0) ||
				(strcmp (cmd, "mon") == 0)
			) {
				result.set(time_tm.tm_mon + 1);
			}
			else if (strcmp (cmd, "day") == 0) {
				result.set(time_tm.tm_mday);
			}
			else if (strcmp (cmd, "hour") == 0) {
				result.set(time_tm.tm_hour);
			}
			else if (strcmp (cmd, "min") == 0) {
				result.set(time_tm.tm_min);
			}
			else if (strcmp (cmd, "sec") == 0) {
				result.set(time_tm.tm_sec);
			}
			else {
				ok = false;
			}

			if (ok) {
				return result.do_operator_recursive(exp_state, final_result);
			}
		}

		ostringstream tmp;
		tmp << "Member '" << cmd << "' is uknown for TIME objects";
		throw runtime_error(tmp.str());
	}
	else if (op == &OP_CONCAT) {
		wpl_value_string result("");
		string &tmp = result.get_internal_string();

		format_time(NULL, tmp);
		tmp += rhs->toString();

		return result.do_operator_recursive(exp_state, final_result);
	}
	else if (op == &OP_ARRAY_SUBSCRIPTING) {
		wpl_value_string result("");
	
		format_time(rhs->toString().c_str(), result.get_internal_string());
	
		return result.do_operator_recursive(exp_state, final_result);
	}
	else if (op == &OP_ASSIGN) {
		set_weak(rhs);
		return do_operator_recursive(exp_state, final_result);
	}
	else if (op == &OP_FUNCTION_CALL) {
		throw runtime_error("Unexpected function call () after TIME object");
	}

	return WPL_OP_UNKNOWN;
}

static inline
string try_guess_sep(const std::string& s){
    string sep;
    std::for_each (s.begin(),
                   s.end(),
                   [&sep,&s](char c){
                    if (!isdigit(c)){
                        if (sep.empty())
                            sep = c;
                        else if (sep[0] != c)
                            throw runtime_error(s + ": invalid date/time format (try with one type of separator)");
                    }
                   }
    );
    return sep;
}

static inline
void replace_month_name(string& fmt){
    static const std::string long_names[] = {"january", "february", "march", "april",
                                           "may", "june", "july","august",
                                           "september","october","november","december"};
    for (int i=0 ; i<sizeof(long_names)/sizeof(long_names[0]) ; ++i){
        const string num = boost::lexical_cast<string>(i+1);
        boost::ireplace_all(fmt, long_names[i], num);
        boost::ireplace_all(fmt, long_names[i].substr(0,3), num);
    }
}

static inline
bool is_leap_year(const int year){
    if (year%400 == 0)
        return true;
    else if (year%100 == 0)
        return false;
    else if (year%4 == 0)
        return true;
    return false;
}

static inline
bool is_valid_date(const int day, const int month, const int year){
    if (month<1 || month>12 || day<1 || day>31)
        return false;
    else if (month == 2)
        return day<(is_leap_year(year) ? 30 : 29);
    else if (month<=7)
        return day<(month%2 ? 32:31);
    else
        return day<(month%2 ? 31:32);
}

static inline
std::string try_guess_hour(std::string& input, int * h_out, int * m_out, int * s_out){
    static const boost::regex rx("(\\d{1,2}):(\\d{1,2})(:(\\d{1,2}))?",boost::regex::extended);
    boost::smatch match;
    if (boost::regex_search(input,match,rx)){
        try {
            int h = boost::lexical_cast<int>(match[1]);
            int m = boost::lexical_cast<int>(match[2]);
            int s = 0;
            if (match.size()>4 && (match[4].first != match[4].second)){
                s = boost::lexical_cast<int>(match[4]);
            }
            if (h>-1 && h<24 && m>-1 && m<60 && s>-1 && s<60){
                *h_out = h;
                *m_out = m;
                *s_out = s;
                input = boost::regex_replace(input,rx,string());
                boost::trim(input);
                return "%T";
            }
        } catch (const boost::bad_lexical_cast& exc){
            throw runtime_error(exc.what());
        }
    }
    return std::string();
}

static inline
std::string try_guess_date(std::string& input, int * y_out, int * m_out, int * d_out){
    static const boost::regex rx("(\\d{1,4})([^\\d]+)(\\d{1,4})([^\\d]+)(\\d{1,4})",boost::regex::extended);
    boost::smatch match;
    if (boost::regex_search(input,match,rx)){
        if (match.size() > 5){
            if (match[2] != match[4])
                throw runtime_error("Invalid date/time format: try with one type of separator");
            const string sep = match[2];
            try{
                const int v1 = boost::lexical_cast<int>(match[1]);
                const int v2 = boost::lexical_cast<int>(match[3]);
                const int v3 = boost::lexical_cast<int>(match[5]);
                if (is_valid_date(v3,v2,v1)){
                    *y_out = v1;
                    *m_out = v2;
                    *d_out = v3;
                    return "%Y"+sep+"%m"+sep+"%d";
                } else if (is_valid_date(v1,v2,v3)){
                    *y_out = v3;
                    *m_out = v2;
                    *d_out = v1;
                    return "%d"+sep+"%m"+sep+"%Y";
                } else {
                    throw runtime_error("Invalid date/time format");
                }
            } catch (const boost::bad_lexical_cast& exc){
                throw runtime_error(exc.what());
            }
        }
    }
    return std::string();
}

void wpl_value_time::try_guess_from_str(const string &fmt_){
    string fmt = fmt_;
    int year=1900;
    int month=1;
    int day=1;
    int hour=0;
    int minute=0;
    int second = 0;
    std::string guessed_fmt;
    std::string guessed_hour_fmt = try_guess_hour(fmt,&hour,&minute,&second);
    if (fmt.empty() == false) {
        replace_month_name(fmt);
        guessed_fmt = try_guess_date(fmt,&year,&month,&day);
    }
    if (!guessed_hour_fmt.empty()){
        if (hour<0 || minute<0 || second<0)
            throw runtime_error("Invalid date/time format");
        if (guessed_fmt.empty())
            guessed_fmt = guessed_hour_fmt;
        else
            guessed_fmt += " " + guessed_hour_fmt;
    }
    struct tm time_tmp;
    memset(&time_tmp,0,sizeof(time_tmp));
    time_tmp.tm_year = year-1900;
    time_tmp.tm_mon = month-1;
    time_tmp.tm_mday = day;
    time_tmp.tm_hour = hour;
    time_tmp.tm_min = minute;
    time_tmp.tm_sec = second;
    this->set_format(guessed_fmt);
    this->set_time(mktime(&time_tmp));
}
