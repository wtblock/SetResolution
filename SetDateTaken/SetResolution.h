/////////////////////////////////////////////////////////////////////////////
// Copyright © by W. T. Block, all rights reserved
/////////////////////////////////////////////////////////////////////////////
#pragma once

#include "resource.h"
#include "KeyedCollection.h"
#include <vector>
#include <map>
#include <memory>
#include <gdiplus.h>

// we need to link to the GDI+ library
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;
using namespace std;

////////////////////////////////////////////////////////////////////////////
// this class creates a fast look up of the mime type and class ID as 
// defined by GDI+ for common file extensions
class CExtension
{
	// protected definitions
protected:
	typedef struct tagExtensionLookup
	{
		CString m_csFileExtension;
		CString m_csMimeType;

	} EXTENSION_LOOKUP;

	typedef struct tagClassLookup
	{
		CString m_csMimeType;
		CLSID m_ClassID;

	} CLASS_LOOKUP;

	// protected data
protected:
	// current file extension
	CString m_csFileExtension;

	// current mime type
	CString m_csMimeType;

	// current class ID
	CLSID m_ClassID;

	// cross reference of file extensions to mime types
	CKeyedCollection<CString, CString> m_mapExtensions;

	// cross reference of mime types to class IDs
	CKeyedCollection<CString, CLSID> m_mapMimeTypes;

	// public properties
public:
	// current file extension
	inline CString GetFileExtension()
	{
		return m_csFileExtension;
	}
	// current file extension
	void SetFileExtension( CString value );
	// current file extension
	__declspec( property( get = GetFileExtension, put = SetFileExtension ) )
		CString FileExtension;

	// image extension associated with the current file extension
	inline CString GetMimeType()
	{
		return m_csMimeType;
	}
	// image extension associated with the current file extension
	inline void SetMimeType( CString value )
	{
		m_csMimeType = value;
	}
	// get image extension associated with the current file extension
	__declspec( property( get = GetMimeType, put = SetMimeType ) )
		CString MimeType;

	// class ID associated with the current file extension
	inline CLSID GetClassID()
	{
		return m_ClassID;
	}
	// class ID associated with the current file extension
	inline void SetClassID( CLSID value )
	{
		m_ClassID = value;
	}
	// class ID associated with the current file extension
	__declspec( property( get = GetClassID, put = SetClassID ) )
		CLSID ClassID;

	// public methods
public:

	// protected methods
protected:

	// public virtual methods
public:

	// protected virtual methods
protected:

	// public construction
public:
	CExtension()
	{
		// extension conversion table
		static EXTENSION_LOOKUP ExtensionLookup[] =
		{
			{ _T( ".bmp" ), _T( "image/bmp" ) },
			{ _T( ".dib" ), _T( "image/bmp" ) },
			{ _T( ".rle" ), _T( "image/bmp" ) },
			{ _T( ".gif" ), _T( "image/gif" ) },
			{ _T( ".jpeg" ), _T( "image/jpeg" ) },
			{ _T( ".jpg" ), _T( "image/jpeg" ) },
			{ _T( ".jpe" ), _T( "image/jpeg" ) },
			{ _T( ".jfif" ), _T( "image/jpeg" ) },
			{ _T( ".png" ), _T( "image/png" ) },
			{ _T( ".tiff" ), _T( "image/tiff" ) },
			{ _T( ".tif" ), _T( "image/tiff" ) }
		};

		// build a cross reference of file extensions to 
		// mime types
		const int nPairs = _countof( ExtensionLookup );
		for ( int nPair = 0; nPair < nPairs; nPair++ )
		{
			const CString csKey =
				ExtensionLookup[ nPair ].m_csFileExtension;

			CString* pValue = new CString
			(
				ExtensionLookup[ nPair ].m_csMimeType
			);

			// add the pair to the collection
			m_mapExtensions.add( csKey, pValue );
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// used for gdiplus library
ULONG_PTR m_gdiplusToken;

// the resolution being set
float m_fResolution;

/////////////////////////////////////////////////////////////////////////////
// this class creates a fast look up of the mime type and class ID as 
// defined by GDI+ for common file extensions
CExtension m_Extension;

/////////////////////////////////////////////////////////////////////////////
// the new folder under the image folder to contain the corrected images
static inline CString GetCorrectedFolder()
{
	return _T( "Corrected" );
}

/////////////////////////////////////////////////////////////////////////////
// the new folder under the image folder to contain the corrected images
static inline int GetCorrectedFolderLength()
{
	const CString csFolder = GetCorrectedFolder();
	const int value = csFolder.GetLength();
	return value;
}

/////////////////////////////////////////////////////////////////////////////
// This function creates a file system folder whose fully qualified 
// path is given by pszPath. If one or more of the intermediate 
// folders do not exist, they will be created as well. 
// returns true if the path is created or already exists
bool CreatePath( LPCTSTR pszPath )
{
	if ( ERROR_SUCCESS == SHCreateDirectoryEx( NULL, pszPath, NULL ) )
	{
		return true;
	}

	return false;
} // CreatePath

/////////////////////////////////////////////////////////////////////////////
// initialize GDI+
bool InitGdiplus()
{
	GdiplusStartupInput gdiplusStartupInput;
	Status status = GdiplusStartup
	(
		&m_gdiplusToken,
		&gdiplusStartupInput,
		NULL
	);
	return ( Ok == status );
} // InitGdiplus

/////////////////////////////////////////////////////////////////////////////
// remove reference to GDI+
void TerminateGdiplus()
{
	GdiplusShutdown( m_gdiplusToken );
	m_gdiplusToken = NULL;

}// TerminateGdiplus

/////////////////////////////////////////////////////////////////////////////
