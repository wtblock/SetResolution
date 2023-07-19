/////////////////////////////////////////////////////////////////////////////
// Copyright © by W. T. Block, all rights reserved
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "SetResolution.h"
#include "CHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object
CWinApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Save the data inside pImage to the given filename but relocated to the 
// sub-folder "Corrected"
bool Save( LPCTSTR lpszPathName, Gdiplus::Image* pImage )
{
	USES_CONVERSION;

	// save and overwrite the selected image file with current page
	int iValue =
		EncoderValue::EncoderValueVersionGif89 |
		EncoderValue::EncoderValueCompressionLZW |
		EncoderValue::EncoderValueFlush;

	EncoderParameters param;
	param.Count = 1;
	param.Parameter[ 0 ].Guid = EncoderSaveFlag;
	param.Parameter[ 0 ].Value = &iValue;
	param.Parameter[ 0 ].Type = EncoderParameterValueTypeLong;
	param.Parameter[ 0 ].NumberOfValues = 1;

	// writing to the same file will fail, so save to a corrected folder
	// below the image being corrected
	const CString csCorrected = GetCorrectedFolder();
	const CString csFolder = CHelper::GetFolder( lpszPathName ) + csCorrected;
	if ( !::PathFileExists( csFolder ) )
	{
		if ( !CreatePath( csFolder ) )
		{
			return false;
		}
	}

	// filename plus extension
	const CString csData = CHelper::GetDataName( lpszPathName );

	// create a new path from the pieces
	const CString csPath = csFolder + _T( "\\" ) + csData;

	// use the extension member class to get the class ID of the file
	CLSID clsid = m_Extension.ClassID;

	// save the image to the corrected folder
	Status status = pImage->Save( T2CW( csPath ), &clsid, &param );

	// return true if the save worked
	return status == Ok;
} // Save

/////////////////////////////////////////////////////////////////////////////
// crawl through the directory tree looking for supported image extensions
void RecursePath( LPCTSTR path )
{
	USES_CONVERSION;

	// valid file extensions
	const CString csValidExt = _T( ".jpg;.jpeg;.png;.gif;.bmp;.tif;.tiff" );

	// the new folder under the image folder to contain the corrected images
	const CString csCorrected = GetCorrectedFolder();
	const int nCorrected = GetCorrectedFolderLength();

	// get the folder which will trim any wild card data
	CString csPathname = CHelper::GetFolder( path );

	// wild cards are in use if the pathname does not equal the given path
	const bool bWildCards = csPathname != path;
	csPathname.TrimRight( _T( "\\" ) );
	CString csData;

	// build a string with wild-cards
	CString strWildcard;
	if ( bWildCards )
	{
		csData = CHelper::GetDataName( path );
		strWildcard.Format( _T( "%s\\%s" ), csPathname, csData );

	} else // no wild cards, just a folder
	{
		strWildcard.Format( _T( "%s\\*.*" ), csPathname );
	}

	// start trolling for files we are interested in
	CFileFind finder;
	BOOL bWorking = finder.FindFile( strWildcard );
	while ( bWorking )
	{
		bWorking = finder.FindNextFile();

		// skip "." and ".." folder names
		if ( finder.IsDots() )
		{
			continue;
		}

		// if it's a directory, recursively search it
		if ( finder.IsDirectory() )
		{
			// do not recurse into the corrected folder
			const CString str =
				finder.GetFilePath().TrimRight( _T( "\\" ) );
			if ( str.Right( nCorrected ) == csCorrected )
			{
				continue;
			}

			// if wild cards are in use, build a path with the wild cards
			if ( bWildCards )
			{
				CString csPath;
				csPath.Format( _T( "%s\\%s" ), str, csData );

				// recurse into the new directory with wild cards
				RecursePath( csPath );

			} else // recurse into the new directory
			{
				RecursePath( str + _T( "\\" ) );
			}

		} else // write the properties if it is a valid extension
		{
			const CString csPath = finder.GetFilePath();
			const CString csExt = CHelper::GetExtension( csPath ).MakeLower();
			const CString csFile = CHelper::GetFileName( csPath );

			if ( -1 != csValidExt.Find( csExt ) )
			{
				m_Extension.FileExtension = csExt;

				CStdioFile fout( stdout );
				fout.WriteString( csPath + _T( "\n" ) );

				unique_ptr<Gdiplus::Bitmap> pImage =
					unique_ptr<Gdiplus::Bitmap>
					(
						Gdiplus::Bitmap::FromFile( T2CW( csPath ) )
					);
				pImage->SetResolution( m_fResolution, m_fResolution );

				// save the image to the new path
				Save( csPath, pImage.get() );
			}
		}
	}

	finder.Close();

} // RecursePath

/////////////////////////////////////////////////////////////////////////////
// set the current file extension which will automatically lookup the
// related mime type and class ID and set their respective properties
void CExtension::SetFileExtension( CString value )
{
	m_csFileExtension = value;

	if ( m_mapExtensions.Exists[ value ] )
	{
		MimeType = *m_mapExtensions.find( value );

		// populate the mime type map the first time it is referenced
		if ( m_mapMimeTypes.Count == 0 )
		{
			UINT num = 0;
			UINT size = 0;

			// gets the number of available image encoders and 
			// the total size of the array
			Gdiplus::GetImageEncodersSize( &num, &size );
			if ( size == 0 )
			{
				return;
			}

			// create a smart pointer to the image codex information
			unique_ptr<ImageCodecInfo> pImageCodecInfo =
				unique_ptr<ImageCodecInfo>
				(
					(ImageCodecInfo*)malloc( size )
				);
			if ( pImageCodecInfo == nullptr )
			{
				return;
			}

			// Returns an array of ImageCodecInfo objects that contain 
			// information about the image encoders built into GDI+.
			Gdiplus::GetImageEncoders( num, size, pImageCodecInfo.get() );

			// populate the map of mime types the first time it is 
			// needed
			for ( UINT nIndex = 0; nIndex < num; ++nIndex )
			{
				CString csKey;
				csKey = CW2A( pImageCodecInfo.get()[ nIndex ].MimeType );
				CLSID classID = pImageCodecInfo.get()[ nIndex ].Clsid;
				m_mapMimeTypes.add( csKey, new CLSID( classID ) );
			}
		}

		ClassID = *m_mapMimeTypes.find( MimeType );

	} else
	{
		MimeType = _T( "" );
	}
} // CExtension::SetFileExtension

/////////////////////////////////////////////////////////////////////////////
// a console application that can crawl through the file
// system and troll for image metadata properties
int _tmain( int argc, TCHAR* argv[], TCHAR* envp[] )
{
	HMODULE hModule = ::GetModuleHandle( NULL );
	if ( hModule == NULL )
	{
		_tprintf( _T( "Fatal Error: GetModuleHandle failed\n" ) );
		return 1;
	}

	// initialize MFC and error on failure
	if ( !AfxWinInit( hModule, NULL, ::GetCommandLine(), 0 ) )
	{
		_tprintf( _T( "Fatal Error: MFC initialization failed\n " ) );
		return 2;
	}

	// do some common command line argument corrections
	vector<CString> arrArgs = CHelper::CorrectedCommandLine( argc, argv );
	size_t nArgs = arrArgs.size();

	CStdioFile fOut( stdout );
	CString csMessage;

	// display the number of arguments if not 1 to help the user 
	// understand what went wrong if there is an error in the
	// command line syntax
	if ( nArgs != 1 )
	{
		fOut.WriteString( _T( ".\n" ) );
		csMessage.Format( _T( "The number of parameters are %d\n.\n" ), nArgs - 1 );
		fOut.WriteString( csMessage );

		// display the arguments
		for ( int i = 1; i < nArgs; i++ )
		{
			csMessage.Format( _T( "Parameter %d is %s\n.\n" ), i, arrArgs[ i ] );
			fOut.WriteString( csMessage );
		}
	}

	// three arguments are expected 
	if ( nArgs != 3 )
	{
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString
		(
			_T( "SetResolution, Copyright (c) 2023, " )
			_T( "by W. T. Block.\n" )
		);

		fOut.WriteString
		(
			_T( ".\n" )
			_T( "A Windows command line program to set the image(s)\n" )
			_T( "  resolution and by default will also process" )
			_T( "  sub-directories.\n" )
			_T( ".\n" )
			_T( "Also a \"Corrected\" folder will be created with the\n" )
			_T( "  same filenames, but these files will have their \n" )
			_T( "  resolution metadata set. The original files\n" )
			_T( "  will remain unmodified.\n" )
			_T( ".\n" )
		);

		fOut.WriteString
		(
			_T( ".\n" )
			_T( "Usage:\n" )
			_T( ".\n" )
			_T( ".  SetResolution pathname resolution\n" )
			_T( ".\n" )
			_T( "Where:\n" )
			_T( ".\n" )
		);

		fOut.WriteString
		(
			_T( ".  pathname is the root of the tree to be scanned, but\n" )
			_T( ".  may contain wild cards like the following:\n" )
			_T( ".    \"c:\\Picture\\DisneyWorldMary2 *.JPG\"\n" )
			_T( ".  will process all files with that pattern, or\n" )
			_T( ".    \"c:\\Picture\\DisneyWorldMary2 231.JPG\"\n" )
			_T( ".  will process a single defined image file.\n" )
			_T( ".  (NOTE: using wild cards will prevent recursion\n" )
			_T( ".    into sub-directories because the folders will likely\n" )
			_T( ".    not fall into the same pattern and therefore\n" )
			_T( ".    sub-folders will not be found by the search).\n" )
		);

		fOut.WriteString
		(
			_T( ".\n" )
			_T( "  resolution is the horizontal and vertical value\n" )
			_T( "  to be set.\n" )
			_T( ".\n" )
		);

		return 3;
	}

	// display the executable path
	//csMessage.Format( _T( "Executable pathname: %s\n" ), arrArgs[ 0 ] );
	//fOut.WriteString( _T( ".\n" ) );
	//fOut.WriteString( csMessage );
	//fOut.WriteString( _T( ".\n" ) );

	// retrieve the pathname which may include wild cards
	CString csPath = arrArgs[ 1 ];

	// retrieve the resolution
	CString csResolution = arrArgs[ 2 ];

	// convert the string to a float
	m_fResolution = (float)_tstof( csResolution );

	// trim off any wild card data
	const CString csFolder = CHelper::GetFolder( csPath );

	// test for current folder character (a period)
	bool bExists = csPath == _T( "." );

	// if it is a period, add a wild card of *.* to retrieve
	// all folders and files
	if ( bExists )
	{
		csPath = _T( ".\\*.*" );

		// if it is not a period, test to see if the folder exists
	} else
	{
		if ( ::PathFileExists( csFolder ) )
		{
			bExists = true;
		}
	}

	if ( !bExists )
	{
		csMessage.Format( _T( "Invalid pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
		fOut.WriteString( _T( ".\n" ) );
		return 4;

	} else
	{
		csMessage.Format( _T( "Given pathname:\n\t%s\n" ), csPath );
		fOut.WriteString( _T( ".\n" ) );
		fOut.WriteString( csMessage );
	}

	// start up COM
	AfxOleInit();
	::CoInitialize( NULL );

	// create a reference to GDI+
	InitGdiplus();

	// crawl through directory tree defined by the command line
	// parameter trolling for supported image files
	RecursePath( csPath );

	// clean up references to GDI+
	TerminateGdiplus();

	// all is good
	return 0;

} // _tmain

/////////////////////////////////////////////////////////////////////////////
