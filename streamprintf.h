// Copyright (c) 2001 Mike Morearty
// Original code and docs: http://www.morearty.com/code/streamprintf
// Date: March 30, 2001
//
// Usage:
//		// printf-style writing to an ostream
//		ostream o;
//		oprintf(o, "%s %d\n", "hello", 3);
//
//		// printf-style writing to a C++ string
//		string s = strprintf("%s %d\n", "hello", 3);
//		wstring ws = wstrprintf(L"%s %d\n", L"hello", 3);
//
//		// printf-style passing of an argument to any function which has
//		// a (const char*) argument, without having to first explicitly
//		// write the string to a local variable
//		void any_function(const char*);
//		any_function( strprintf("%s %d\n", "hello", 3).c_str() );
//
//		// another example:
//		MessageBox( hwnd,
//					strprintf( "error %d", errorcode ).c_str(),
//					NULL, MB_OK );

//-----------------------------------------------------------------------------
// If STREAMPRINTF_STRICT_SIGN is defined, then the sign of the parameter
// must match the sign of the formatting argument.  For example, if this is
// defined, then the following lines would cause runtime assertions:
//		int i = 0;
//		unsigned u = 0;
//		oprintf(cout, "%d", u);
//		oprintf(cout, "%u", i);

// #define STREAMPRINTF_STRICT_SIGN


//-----------------------------------------------------------------------------
// If STREAMPRINTF_STRICT_INTSIZE is defined, then 'int' and 'long' are treated
// as incompatible types.  For example, if this is defined, then the following
// lines would cause runtime assertions:
//		int i = 0;
//		int l = 0;
//		oprintf(cout, "%ld", i);
//		oprintf(cout, "%d", l);

// #define STREAMPRINTF_STRICT_INTSIZE


#include <assert.h>
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>

#include <iostream>
#include <sstream>

#ifndef NDEBUG
	#if _MSC_VER >= 1400
		#define assertmsg(exp, msg) (void)( (exp) || (_wassert(_CRT_WIDE(msg), _CRT_WIDE(__FILE__), __LINE__), 0) )
	#else
		#define assertmsg(exp, msg) (void)( (exp) || (_assert(msg, __FILE__, __LINE__), 0) )
	#endif
	#define PRINTF_TYPE(n) n
#else
	#define assertmsg(exp, msg) assert(exp)
	#define PRINTF_TYPE(n) 0
#endif

template <class CharT>
class Printf
{
public:
	Printf(std::basic_ostream<CharT>& ostm, const CharT* fmt) : _ostm(ostm), _fmt(fmt), _pos(0)
		{ OutputStaticText(); }
	~Printf()
		{ assertmsg( _fmt[_pos] == '\0', "printf: Too few arguments" ); }

	Printf& operator<<(bool n)                 { Do(PRINTF_TYPE(None | Int), n); return *this; }
	Printf& operator<<(short n)                { Do(PRINTF_TYPE(Short| Int), n); return *this; }
	Printf& operator<<(int n)                  { Do(PRINTF_TYPE(None | Int), n); return *this; }
	Printf& operator<<(long n)                 { Do(PRINTF_TYPE(Long | Int), n); return *this; }
	Printf& operator<<(__int64 n)              { Do(PRINTF_TYPE(Int64| Int), n); return *this; }

	Printf& operator<<(unsigned short u)       { Do(PRINTF_TYPE(Short| Unsigned), u); return *this; }
	Printf& operator<<(unsigned int u)         { Do(PRINTF_TYPE(None | Unsigned), u); return *this; }
	Printf& operator<<(unsigned long u)        { Do(PRINTF_TYPE(Long | Unsigned), u); return *this; }
	Printf& operator<<(unsigned __int64 u)     { Do(PRINTF_TYPE(Int64| Unsigned), u); return *this; }

	Printf& operator<<(float f)                { Do(PRINTF_TYPE(None | Float), f); return *this; }
	Printf& operator<<(double f)               { Do(PRINTF_TYPE(None | Float), f); return *this; }
	Printf& operator<<(long double f)          { Do(PRINTF_TYPE(Long | Float), f); return *this; }

	Printf& operator<<(char c)                 { Do(PRINTF_TYPE(Short| Char), c); return *this; }
	Printf& operator<<(unsigned char c)        { Do(PRINTF_TYPE(Short| Char), c); return *this; }

	Printf& operator<<(const char* s)          { Do(PRINTF_TYPE(Short| String), s); return *this; }
	Printf& operator<<(const unsigned char* s) { Do(PRINTF_TYPE(Short| String), s); return *this; }
	Printf& operator<<(const std::string& s)   { Do(PRINTF_TYPE(Short| String), s.c_str()); return *this; }

	Printf& operator<<(const wchar_t* w)       { Do(PRINTF_TYPE(Long | String), w); return *this; }
	Printf& operator<<(const std::wstring& w)  { Do(PRINTF_TYPE(Long | String), w.c_str()); return *this; }

	Printf& operator<<(const void* v)          { Do(PRINTF_TYPE(None | Pointer), v); return *this; }

protected:
	enum Size { None=1, Short=2, Long=3, Int64=4, sizeMask=0xFF };
	enum Type { Int=0x100, Unsigned=0x200, Float=0x300, Char=0x400, String=0x500,
				WideString=0x600, Pointer=0x700, typeMask=0xFF00 };

	void Do(int sizeAndType, ...);
	void my_vsprintf(CharT* output, size_t width, const CharT* format, va_list vl);
	void OutputStaticText();

	const CharT* _fmt;		// the format string currently being processed
	size_t _pos;			// current position with _fmt
	std::basic_ostream<CharT>& _ostm;	// stream to which we're outputting
};

//-----------------------------------------------------------------------------
// If you want to use wvsprintf() instead of vsprintf(), you can do that by
// changing the two lines below from "vsprintf" and "vswprintf" to "wvsprintfA"
// and "wvsprintfW" respectively.  Note that the C runtime routines and the
// Windows routines aren't 100% identical in behavior, and there are a few
// parts of the code that assume the behavior of vs[w]printf().  For example,
// the code supports the "%I64" format specifier for 64-bit integers, but
// wvsprintf() does not support that.

template <>
inline void Printf<char>::my_vsprintf(char* output, size_t width, const char* format, va_list vl)
{
	#if _MSC_VER >= 1400
		vsprintf_s(output, width, format, vl);
	#else
		vsprintf(output, format, vl);
	#endif
}

template <>
inline void Printf<wchar_t>::my_vsprintf(wchar_t* output, size_t width, const wchar_t* format, va_list vl)
{
	#if _MSC_VER >= 1400
		vswprintf_s(output, width, format, vl);
	#else
		vswprintf(output, format, vl);
	#endif
}

//-----------------------------------------------------------------------------
template <class CharT>
void Printf<CharT>::Do(int sizeAndType, ...)
{
#ifndef NDEBUG
	Size size = (Size) (sizeAndType & sizeMask);
	Type type = (Type) (sizeAndType & typeMask);
#endif
	va_list vl;
	CharT format[30];
	int i = 0;
	CharT* result;
	CharT sizeChar;
	CharT fmtChar;
	int width = 0;
	int precision = 0;

#ifndef NDEBUG
	const char* legalPrintfTypeChars;

	switch (type)
	{
#ifdef STREAMPRINTF_STRICT_SIGN
	case Int:        legalPrintfTypeChars = "dioxX"; break;
	case Unsigned:   legalPrintfTypeChars = "uoxX";  break;
#else
	case Int:
	case Unsigned:   legalPrintfTypeChars = "diuoxX"; break;
#endif
	case Float:      legalPrintfTypeChars = "eEfgG"; break;
	case Char:       legalPrintfTypeChars = "c";     break;
	case String:     legalPrintfTypeChars = "sp";    break;
	case Pointer:    legalPrintfTypeChars = "p";     break;
	default:         assert(false);
	}
#endif

	format[i++] = _fmt[_pos++];
	assertmsg(format[0] == '%', "printf: Too many arguments");

	// flags
	while (strchr("-+0 #", _fmt[_pos]))
		format[i++] = _fmt[_pos++];

	// width
	if (isdigit(_fmt[_pos]))
	{
		while (isdigit(_fmt[_pos]))
		{
			format[i++] = _fmt[_pos++];
			width = (width*10) + (format[i-1] - '0');
		}
	}

	// precision
	if (_fmt[_pos] == '.')
	{
		format[i++] = _fmt[_pos++];
		while (isdigit(_fmt[_pos]))
		{
			format[i++] = _fmt[_pos++];
			precision = (precision*10) + (format[i-1] - '0');
		}
	}

	// size
	if (strchr("hlL", _fmt[_pos]) != NULL)
	{
		sizeChar = format[i++] = _fmt[_pos++];
		if (sizeChar == 'L')
			sizeChar = 'l';
	}
	else if (_fmt[_pos] == 'I' && _fmt[_pos+1] == '6' && _fmt[_pos+2] == '4')
	{
		sizeChar = format[i++] = _fmt[_pos++];
		format[i++] = _fmt[_pos++];
		format[i++] = _fmt[_pos++];
	}
	else
	{
		sizeChar = '\0';
	}

	assertmsg(_fmt[_pos] != '\0', "printf: Invalid format specification");
	fmtChar = format[i++] = _fmt[_pos++];

	if (sizeChar == '\0')
	{
		switch (fmtChar)
		{
		case 'c':
		case 's':
			sizeChar = (sizeof(CharT) == sizeof(char)) ? 'h' : 'l';
			break;
		case 'C':
		case 'S':
			sizeChar = (sizeof(CharT) == sizeof(char)) ? 'l' : 'h';
			break;
		}
	}

	if (fmtChar == 'C')
		fmtChar = 'c';
	else if (fmtChar == 'S')
		fmtChar = 's';

	assertmsg(i < sizeof(format) / sizeof(format[0]), "printf: Format specification is too long");
	format[i] = '\0';

#ifndef NDEBUG
	// Do the type-checking.  Characters and strings are tricky.
	if (type == String && fmtChar == 'p')
	{
		assertmsg(sizeChar == '\0', "printf: Invalid format specification");
	}
	else if (type == Unsigned && size == Short)
	{
		// unsigned short can either mean an unsigned short integer,
		// or a wchar_t
		if (fmtChar == 'c')	// wchar_t is intended, so width must be 'l'
			assertmsg(sizeChar == 'l', "printf: Type mismatch");
		else				// unsigned short is intended, so width must be 'h'
			assertmsg(sizeChar == 'h', "printf: Type mismatch");
	}
	else
	{
		switch (size)
		{
#ifdef STREAMPRINTF_STRICT_INTSIZE
		case None:
			assertmsg(sizeChar == '\0', "printf: Type mismatch");
			break;
		case Long:
			assertmsg(sizeChar == 'l', "printf: Type mismatch");
			break;
#else
		case None:
		case Long:
			assert(sizeof(int) == sizeof(long));
			assertmsg(sizeChar == '\0' || sizeChar == 'l', "printf: Type mismatch");
			break;
#endif
		case Short:
			assertmsg(sizeChar == 'h', "printf: Type mismatch");
			break;
		case Int64:
			assertmsg(sizeChar == 'I', "printf: Type mismatch");
			break;
		}
	}

	if (type == Unsigned && size == Short)
		assertmsg(fmtChar == 'c' || strchr(legalPrintfTypeChars, fmtChar) != NULL, "printf: Type mismatch");
	else
		assertmsg(strchr(legalPrintfTypeChars, fmtChar) != NULL, "printf: Type mismatch");
#endif

	// allocate buffer for result
	if (fmtChar == 's')
	{
		int len;

		va_start(vl, sizeAndType);

		if (sizeChar == 'h')
		{
			char* staticStr = va_arg(vl, char*);
			len = strlen(staticStr);
		}
		else
		{
			assert(sizeChar == 'l');
			wchar_t* staticStr = va_arg(vl, wchar_t*);
			len = wcslen(staticStr);
		}

		if (len > width)
			width = len;

		va_end(vl);
	}

	if (precision > width)
		width = precision;
	width += 30;
	result = (CharT*) alloca(width * sizeof(CharT));

	va_start(vl, sizeAndType);
	my_vsprintf(result, width, format, vl);
	va_end(vl);

	_ostm << result;

	OutputStaticText();
}

//-----------------------------------------------------------------------------
template <class CharT>
void Printf<CharT>::OutputStaticText()
{
	CharT c[2] = { 0, 0 };

	while (_fmt[_pos] != '\0')
	{
		if (_fmt[_pos] == '%')
		{
			assertmsg(_fmt[_pos+1] != '\0', "printf: Invalid format specification");
			if (_fmt[_pos+1] == '%')	// in a printf format string, "%%" outputs "%"
			{
				// must output "%", not '%', so that it works properly for wchar_t
				_ostm << "%";
				++_pos;
			}
			else
			{
				break;
			}
		}
		else
		{
			// must output "c", not 'c', so that it works properly for wchar_t
			c[0] = _fmt[_pos];
			_ostm << c;
		}
		++_pos;
	}
}



//-----------------------------------------------------------------------------
template <class CharT>
inline void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt)
{
	Printf<CharT> p(ostm, fmt);
}

template <class CharT, class A1>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1)
{
	Printf<CharT> p(ostm, fmt);
	p << a1;
}

template <class CharT, class A1, class A2>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2;
}

template <class CharT, class A1, class A2, class A3>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3;
}

template <class CharT, class A1, class A2, class A3, class A4>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5, class A6>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5 << a6;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5, class A6, class A7>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5 << a6 << a7;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9;
}

template <class CharT, class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
void oprintf(std::basic_ostream<CharT>& ostm, const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10)
{
	Printf<CharT> p(ostm, fmt);
	p << a1 << a2 << a3 << a4 << a5 << a6 << a7 << a8 << a9 << a10;
}



//-----------------------------------------------------------------------------
template <class CharT>
class strprintfT: public std::basic_string<CharT>
{
	typedef std::basic_string<CharT> Base;

public:
	strprintfT(const CharT* fmt)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt);
		*(Base*)this = o.str();
	}

	template <class A1>
	strprintfT(const CharT* fmt, A1 a1)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1);
		*(Base*)this = o.str();
	}

	template <class A1, class A2>
	strprintfT(const CharT* fmt, A1 a1, A2 a2)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5, class A6>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5, a6);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5, a6, a7);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		*(Base*)this = o.str();
	}

	template <class A1, class A2, class A3, class A4, class A5, class A6, class A7, class A8, class A9, class A10>
	strprintfT(const CharT* fmt, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6, A7 a7, A8 a8, A9 a9, A10 a10)
	{
		std::basic_ostringstream<CharT> o;
		oprintf(o, fmt, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
		*(Base*)this = o.str();
	}

	operator const CharT* () { return c_str(); }
};

typedef strprintfT<char> strprintf;
typedef strprintfT<wchar_t> wstrprintf;
