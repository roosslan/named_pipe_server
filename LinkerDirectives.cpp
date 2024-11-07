
#include "stdafx.h"

//required for the api calls in ntsecapi and secext
#pragma comment(lib, "Secur32")

//required for the NET api calls in lm.h
#pragma comment(lib, "Netapi32")

//required for transaction support
#pragma comment(lib, "KtmW32")

//required for transaction support
#pragma comment(lib, "bcrypt")

//required for directory services support
#pragma comment(lib, "adsiid")
#pragma comment(lib, "activeds")

//required for WMI interface
#pragma comment(lib, "wbemuuid.lib")