// Stdafx.h: include file for standard system include files, 
// or a lot reference number of times, and not so much change,
// describes the project dedicated include files.

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN // exclude the part that is not being used from Windows header.
#endif


#ifndef WINVER // Windows 95 and Windows NT 4 or later to allow the use of specific features.
#define WINVER 0x0501 // This is Windows 98 and Windows 2000 or a later version for.
#endif

#ifndef _WIN32_WINNT // Windows NT 4 or later to allow the use of specific features.
#define _WIN32_WINNT 0x0501 // this Windows 2000 or a later version for.
#endif

#ifndef _WIN32_WINDOWS // Windows 98 or later to allow the use of specific features.
#define _WIN32_WINDOWS 0x0501 // This is Windows Me or later versions for.
#endif

#ifndef _WIN32_IE // IE 4.0 and later allow the use of specific features.
#define _WIN32_IE 0x0500 // This IE 5.0 or a later version for.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // part is explicitly.

#include <afxwin.h> // core and standard component of MFC
#include <afxext.h> // extension of the MFC

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h> // MFC OLE class
#include <afxodlgs.h> // MFC OLE dialog class
#include <afxdisp.h> // MFC automation class
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h> // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h> // MFC DAO database class
#endif // _AFX_NO_DAO_SUPPORT

#include <afxdtctl.h> // Internet Explorer 4 Common control support of MFC
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h> // MFC of the Windows Common Controls support
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>