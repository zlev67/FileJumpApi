#pragma once

#ifdef FILEJUMP_EXPORTS
#define FILEJUMP_API __declspec(dllexport)
#else
#define FILEJUMP_API __declspec(dllimport)
#endif

