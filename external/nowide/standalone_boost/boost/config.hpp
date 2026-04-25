//
// Standalone minimal <boost/config.hpp> shim for Boost.Nowide
// Provides the macros nowide needs without requiring full Boost.
//

#if(defined(_WIN32) || defined(__WIN32__) || defined(WIN32)) && !defined(__CYGWIN__)
#define BOOST_WINDOWS
#endif

#ifdef _MSC_VER
#define BOOST_MSVC _MSC_VER
#endif

#ifdef __GNUC__
#define BOOST_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#endif

#ifndef BOOST_SYMBOL_VISIBLE
#define BOOST_SYMBOL_VISIBLE
#endif

#ifdef BOOST_WINDOWS
#define BOOST_SYMBOL_EXPORT __declspec(dllexport)
#define BOOST_SYMBOL_IMPORT __declspec(dllimport)
#elif defined(__CYGWIN__) && defined(__GNUC__) && (__GNUC__ >= 4)
#define BOOST_SYMBOL_EXPORT __attribute__((__dllexport__))
#define BOOST_SYMBOL_IMPORT __attribute__((__dllimport__))
#else
#define BOOST_SYMBOL_EXPORT BOOST_SYMBOL_VISIBLE
#define BOOST_SYMBOL_IMPORT
#endif

#if defined __GNUC__ && __GNUC__ >= 7
#define BOOST_FALLTHROUGH __attribute__((fallthrough))
#elif defined __clang__ && __has_cpp_attribute(clang::fallthrough)
#define BOOST_FALLTHROUGH [[clang::fallthrough]]
#elif defined _MSC_VER
#define BOOST_FALLTHROUGH __fallthrough
#else
#define BOOST_FALLTHROUGH
#endif

#if defined __GNUC__
#define BOOST_LIKELY(x) __builtin_expect(x, 1)
#define BOOST_UNLIKELY(x) __builtin_expect(x, 0)
#else
#if !defined(BOOST_LIKELY)
#define BOOST_LIKELY(x) x
#endif
#if !defined(BOOST_UNLIKELY)
#define BOOST_UNLIKELY(x) x
#endif
#endif

// C++11 features are assumed available (required by nowide)
#define BOOST_NO_CXX11_CHAR16_T 0
#define BOOST_NO_CXX11_CHAR32_T 0
