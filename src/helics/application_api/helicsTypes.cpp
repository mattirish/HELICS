/*

Copyright (C) 2017, Battelle Memorial Institute
All rights reserved.

This software was co-developed by Pacific Northwest National Laboratory, operated by the Battelle Memorial
Institute; the National Renewable Energy Laboratory, operated by the Alliance for Sustainable Energy, LLC; and the
Lawrence Livermore National Laboratory, operated by Lawrence Livermore National Security, LLC.

*/

#include "helicsTypes.hpp"
#include <map>
#include <regex>
#include <boost/lexical_cast.hpp>
#include "application_api/ValueConverter.hpp"

namespace helics
{
static const std::string doubleString ("double");
static const std::string intString ("int64");
static const std::string stringString ("string");
static const std::string complexString ("complex");
static const std::string doubleVecString ("double_vector");
static const std::string complexVecString ("complex_vector");
static const std::string nullString ("");

const std::string &typeNameStringRef (helicsType_t type)
{
    switch (type)
    {
    case helicsType_t::helicsDouble:
        return doubleString;
    case helicsType_t::helicsInt:
        return intString;
    case helicsType_t::helicsString:
        return stringString;
    case helicsType_t::helicsComplex:
        return complexString;
    case helicsType_t::helicsVector:
        return doubleVecString;
    case helicsType_t::helicsComplexVector:
        return complexVecString;
    default:
        return nullString;
    }
}

std::string helicsComplexString (double real, double imag)
{
    std::stringstream ss;
    ss << real;
    if (imag != 0.0)
    {
        if (imag >= 0.0)
        {
            ss << '+' << imag;
        }
        else
        {
            ss << imag;
        }
        ss << 'j';
    }
    return ss.str ();
}

std::string helicsComplexString (std::complex<double> val)
{
    return helicsComplexString (val.real (), val.imag ());
}

static const std::map<std::string, helicsType_t> typeMap{
  {"double", helicsType_t::helicsDouble},
  {"string", helicsType_t::helicsString},
  {"float", helicsType_t::helicsDouble},
  {"vector", helicsType_t::helicsVector},
  {"double_vector", helicsType_t::helicsVector},
  {"complex", helicsType_t::helicsComplex},
  {"int", helicsType_t::helicsInt},
  {"int64", helicsType_t::helicsInt},
  {"complex_vector", helicsType_t::helicsComplexVector},
};

helicsType_t getTypeFromString (const std::string &typeName)
{
    auto res = typeMap.find (typeName);
    if (res == typeMap.end ())
    {
        return helicsType_t::helicsInvalid;
    }
    return res->second;
}

// regular expression to handle complex numbers of various formats
const std::regex creg (
  R"(([+-]?(\d+(\.\d+)?|\.\d+)([eE][+-]?\d+)?)\s*([+-]\s*(\d+(\.\d+)?|\.\d+)([eE][+-]?\d+)?)[ji]*)");

std::complex<double> helicsGetComplex (const std::string &val)
{
    if (val.empty ())
    {
        return std::complex<double> (-1e49, -1e49);
    }
    std::smatch m;
    double re = 0.0;
    double im = 0.0;
    std::regex_search (val, m, creg);
    try
    {
        if (m.size () == 9)
        {
            re = boost::lexical_cast<double> (m[1]);

            im = boost::lexical_cast<double> (m[6]);

            if (*m[5].first == '-')
            {
                im = -im;
            }
        }
        else
        {
            if ((val.back () == 'j') || (val.back () == 'i'))
            {
                im = boost::lexical_cast<double> (val.substr (0, val.size () - 1));
            }
            else
            {
                re = boost::lexical_cast<double> (val);
            }
        }
    }
    catch (const boost::bad_lexical_cast &)
    {
        im = 0.0;
    }
    return {re, im};
}

std::string helicsVectorString (const std::vector<double> &val)
{
    std::string vString ("v");
    vString.append (std::to_string (val.size ()));
    vString.push_back ('[');
    for (const auto &v : val)
    {
        vString.append (std::to_string (v));
        vString.push_back (';');
        vString.push_back (' ');
    }
    if (vString.size () > 2)
    {
        vString.pop_back ();
        vString.pop_back ();
    }
    vString.push_back (']');
    return vString;
}

std::string helicsComplexVectorString (const std::vector<std::complex<double>> &val)
{
    std::string vString ("c");
    vString.append (std::to_string (val.size ()));
    vString.push_back ('[');
    for (const auto &v : val)
    {
        vString.append (helicsComplexString (v.real (), v.imag ()));
        vString.push_back (';');
        vString.push_back (' ');
    }
    if (vString.size () > 2)
    {
        vString.pop_back ();
        vString.pop_back ();
    }
    vString.push_back (']');
    return vString;
}

std::vector<double> helicsGetVector (const std::string &val)
{
    std::vector<double> V;
    helicsGetVector (val, V);
    return V;
}

std::vector<std::complex<double>> helicsGetComplexVector (const std::string &val)
{
    std::vector<std::complex<double>> V;
    helicsGetComplexVector (val, V);
    return V;
}

static auto readSize (const std::string &val)
{
    auto fb = val.find_first_of ('[');
    auto size = std::stoull (val.substr (1, fb - 1));
    return size;
}

void helicsGetVector (const std::string &val, std::vector<double> &data)
{
    if (val.empty ())
    {
        data.resize (0);
    }
    if (val.front () == 'v')
    {
        auto sz = readSize (val);
        data.reserve (sz);
        data.resize (0);
        auto fb = val.find_first_of ('[');
        for (decltype (sz) ii = 0; ii < sz; ++ii)
        {
            auto nc = val.find_first_of (",]", fb + 1);
            try
            {
                auto V = boost::lexical_cast<double> (val.data () + fb + 1, nc - fb - 1);
                data.push_back (V);
            }
            catch (const boost::bad_lexical_cast &)
            {
                data.push_back (-1e49);
            }
            fb = nc;
        }
    }
    else if (val.front () == 'c')
    {
        auto sz = readSize (val);
        data.reserve (sz * 2);
        data.resize (0);
        auto fb = val.find_first_of ('[');
        for (decltype (sz) ii = 0; ii < sz; ++ii)
        {
            auto nc = val.find_first_of (",]", fb + 1);
            auto V = helicsGetComplex (val.substr (fb + 1, nc - fb - 1));
            data.push_back (V.real ());
            data.push_back (V.imag ());
            fb = nc;
        }
    }
    else
    {
        auto V = helicsGetComplex (val);
        if (V.imag () == 0)
        {
            data.resize (1);
            data[0] = V.real ();
        }
        else
        {
            data.resize (2);
            data[0] = V.real ();
            data[1] = V.imag ();
        }
    }
    return;
}

void helicsGetComplexVector (const std::string &val, std::vector<std::complex<double>> &data)
{
    if (val.empty ())
    {
        data.resize (0);
    }
    if (val.front () == 'v')
    {
        auto sz = readSize (val);
        data.reserve (sz / 2);
        data.resize (0);
        auto fb = val.find_first_of ('[');
        for (decltype (sz) ii = 0; ii < sz - 1; ii += 2)
        {
            auto nc = val.find_first_of (",]", fb + 1);
            auto nc2 = val.find_first_of (",]", nc + 1);
            try
            {
                auto V1 = boost::lexical_cast<double> (val.data () + fb + 1, nc - fb - 1);
                auto V2 = boost::lexical_cast<double> (val.data () + nc + 1, nc2 - nc - 1);
                data.emplace_back (V1, V2);
            }
            catch (const boost::bad_lexical_cast &)
            {
                data.push_back (-1e49);
            }
            fb = nc;
        }
    }
    else if (val.front () == 'c')
    {
        auto sz = readSize (val);
        data.reserve (sz);
        data.resize (0);
        auto fb = val.find_first_of ('[');
        for (decltype (sz) ii = 0; ii < sz; ++ii)
        {
            auto nc = val.find_first_of (",]", fb + 1);
            auto V = helicsGetComplex (val.substr (fb + 1, nc - fb - 1));
            data.push_back (V);
            fb = nc;
        }
    }
    else
    {
        auto V = helicsGetComplex (val);
        if (V.imag () == 0)
        {
            data.resize (1);
            data[0] = V.real ();
        }
        else
        {
            data.resize (2);
            data[0] = V.real ();
            data[1] = V.imag ();
        }
    }
    return;
}

data_block typeConvert(helicsType_t type, double val)
{
	switch (type)
	{
		case helicsType_t::helicsDouble:
		default:
			return ValueConverter<double>::convert(val);
		case helicsType_t::helicsInt:
			return ValueConverter<int64_t>::convert(static_cast<int64_t>(val));
		case helicsType_t::helicsComplex:
			return ValueConverter<std::complex<double>>::convert(std::complex<double>(val, 0.0));
		case helicsType_t::helicsString:
			return std::to_string(val);
		case helicsType_t::helicsComplexVector:
		{
			std::complex<double> v2(val, 0.0);
			return ValueConverter<std::complex<double>>::convert(&v2, 1);
		}
		case helicsType_t::helicsVector:
			return ValueConverter<double>::convert(&val, 1);

	}
}
data_block typeConvert(helicsType_t type, int64_t val)
{
	switch (type)
	{
	case helicsType_t::helicsDouble:
		return ValueConverter<double>::convert(static_cast<double>(val));
	case helicsType_t::helicsInt:
	default:
		return ValueConverter<int64_t>::convert(val);
	case helicsType_t::helicsComplex:
		return ValueConverter<std::complex<double>>::convert(std::complex<double>(val, 0.0));
	case helicsType_t::helicsString:
		return std::to_string(val);
	case helicsType_t::helicsComplexVector:
	{
		std::complex<double> v2(static_cast<double>(val), 0.0);
		return ValueConverter<std::complex<double>>::convert(&v2, 1);
	}
	case helicsType_t::helicsVector:
	{
		double v2 = static_cast<double>(val);
		return ValueConverter<double>::convert(&v2, 1);
	}
	}
}

data_block typeConvert(helicsType_t type, const std::string &val)
{
	switch (type)
	{
	case helicsType_t::helicsDouble:
		return ValueConverter<double>::convert(boost::lexical_cast<double>(val));
	case helicsType_t::helicsInt:
		return ValueConverter<int64_t>::convert(boost::lexical_cast<int64_t>(val));
	case helicsType_t::helicsComplex:
		return ValueConverter<std::complex<double>>::convert(helicsGetComplex(val));
	case helicsType_t::helicsString:
	default:
		return val;
	case helicsType_t::helicsComplexVector:
		return ValueConverter<std::vector<std::complex<double>>>::convert(helicsGetComplexVector(val));
	case helicsType_t::helicsVector:
		return ValueConverter<std::vector<double>>::convert(helicsGetVector(val));

	}
}
data_block typeConvert(helicsType_t type, const std::vector<double> &val)
{
	switch (type)
	{
	case helicsType_t::helicsDouble:
		if (val.empty())
		{
			return ValueConverter<double>::convert(0.0);
		}
		else
		{
			return ValueConverter<double>::convert(val[0]);
		}
		
	case helicsType_t::helicsInt:
		break;
	case helicsType_t::helicsComplex:
	{
		std::complex<double> V(0.0,0.0);
		if (val.size() >= 2)
		{
			V = std::complex<double>(val[0], val[1]);
		}
		else if (val.size() == 1)
		{
			V = std::complex<double>(val[0],0.0);
		}
		return ValueConverter<std::complex<double>>::convert(V);
	}
	case helicsType_t::helicsString:
		return helicsVectorString(val);
	case helicsType_t::helicsComplexVector:
	{
		std::vector<std::complex<double>> CD;
		CD.reserve(val.size() / 2);
		for (size_t ii = 0; ii < val.size() - 1; ++ii)
		{
			CD.emplace_back(val[ii], val[ii + 1]);
		}
		return ValueConverter <std::vector<std::complex<double>>> ::convert(CD);
	}
		break;
	case helicsType_t::helicsVector:
	default:
		return ValueConverter <std::vector<double>> ::convert(val);

	}
}
data_block typeConvert(helicsType_t type, const std::vector<std::complex<double>> &val)
{
	switch (type)
	{
	case helicsType_t::helicsDouble:
	
	case helicsType_t::helicsInt:
		break;
	case helicsType_t::helicsComplex:
		break;
	case helicsType_t::helicsString:
		break;
	case helicsType_t::helicsComplexVector:
	default:
		return ValueConverter<std::vector<std::complex<double>>>::convert(val);
	case helicsType_t::helicsVector:
		break;

	}
}
data_block typeConvert(helicsType_t type, std::complex<double> &val)
{
	switch (type)
	{
	case helicsType_t::helicsDouble:
	
		
	case helicsType_t::helicsInt:
		break;
	case helicsType_t::helicsComplex:
	default:
		return ValueConverter<std::complex<double>>::convert(val);
	case helicsType_t::helicsString:
		break;
	case helicsType_t::helicsComplexVector:
		return ValueConverter<std::complex<double>>::convert(&val, 1);
	case helicsType_t::helicsVector:
	{
		std::vector<double>V{ val.real(), val.imag() };
		return ValueConverter<std::vector<double>>::convert(V);
	}

	}
}

}  // namespace helics