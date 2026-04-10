/*! @file SampleMetadata.cpp

*  @brief Internal routines used for processing sample metadata.
*  
*  Interface to the CineForm HD decoder.  The decoder API uses an opaque
*  data type to represent an instance of a decoder.  The decoder reference
*  is returned by the call to CFHD_OpenDecoder.
*
*  @version 1.0.0
*
*  (C) Copyright 2017 GoPro Inc (http://gopro.com/).
*
*  Licensed under either:
*  - Apache License, Version 2.0, http://www.apache.org/licenses/LICENSE-2.0  
*  - MIT license, http://opensource.org/licenses/MIT
*  at your option.
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/


#include "StdAfx.h"

#ifdef _WIN32
#else
#define MAX_PATH	260
#if __APPLE__
#include <string.h>
#include "CoreFoundation/CoreFoundation.h"
#else
#include  <mm_malloc.h>
#endif
#endif

#include "CFHDMetadata.h"
#include "../Common/AVIExtendedHeader.h"
#include "SampleMetadata.h"
#include "../Codec/metadata.h"
#include "../Codec/lutpath.h"


/*
 * TODO: Should change this routine to pass the length of the output strings
 * or use the string class from the standard template library to avoid writing
 * beyond the end of the output strings.
 */


void InitGetLUTPaths(char *pPathStr, size_t pathSize, char *pDBStr, size_t DBSize)
{
	if (pPathStr && pDBStr)
	{
#ifdef _WIN32
		DWORD dwType = REG_SZ, length = 260;
		HKEY hKey = 0;
		const TCHAR* CPsubkey = TEXT("SOFTWARE\\CineForm\\ColorProcessing");

		const size_t max_path_len = 260;
		const size_t max_name_len = 64;
		TCHAR defaultLUTpath[max_path_len] = TEXT("NONE");
		TCHAR defaultOverridePath[max_path_len] = TEXT("");
		TCHAR DbNameStr[max_name_len] = TEXT("db");

		RegOpenKey(HKEY_CURRENT_USER, CPsubkey, &hKey);
		if (hKey != 0)
		{
			length = 260;
			RegQueryValueEx(hKey, TEXT("LUTPath"), NULL, &dwType, (LPBYTE)defaultLUTpath, &length);
			length = 260;
			RegQueryValueEx(hKey, TEXT("OverridePath"), NULL, &dwType, (LPBYTE)defaultOverridePath, &length);
			length = 64;
			RegQueryValueEx(hKey, TEXT("DBPath"), NULL, &dwType, (LPBYTE)DbNameStr, &length);
		}

		if (0 == wcscmp(defaultLUTpath, TEXT("NONE")))
		{
			int n;
			TCHAR PublicPath[80];

			if (n = GetEnvironmentVariable(TEXT("PUBLIC"), PublicPath, 79)) // Vista and Win7
			{
				_stprintf_s(defaultLUTpath, max_path_len, TEXT("%s\\%s"), PublicPath, TEXT("CineForm\\LUTs")); //Vista & 7 default
				_stprintf_s(defaultOverridePath, max_path_len, TEXT("%s\\%s"), PublicPath, TEXT("CineForm\\LUTs")); //Vista & 7 default
			}
			else
			{
				const TCHAR* CVsubkey = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion");
				RegOpenKey(HKEY_LOCAL_MACHINE, CVsubkey, &hKey);
				if (hKey != 0)
				{
					TCHAR commonpath[64] = TEXT("NONE");
					length = 64;
					RegQueryValueEx(hKey, TEXT("CommonFilesDir (x86)"), NULL, &dwType, (LPBYTE)commonpath, &length);
					if (0 == wcscmp(commonpath, TEXT("NONE")))
					{
						length = 64;
						RegQueryValueEx(hKey, TEXT("CommonFilesDir"), NULL, &dwType, (LPBYTE)commonpath, &length);
					}
					_stprintf_s(defaultLUTpath, max_path_len, TEXT("%s\\%s"), commonpath, TEXT("CineForm\\LUTs"));
					_stprintf_s(defaultOverridePath, max_path_len, TEXT("%s\\%s"), commonpath, TEXT("CineForm\\LUTs"));
				}
			}
		}

		// I found that the old code was not actually unicode safe, despite the attempt to be.
		// I don't want to change the encoder and decoder structures at this time,
		// so I am converting the strings back to multibyte before copying them into the structures.
		// This is not ideal, but it is better than the previous code which could potentially write
		// past the end of the buffers if the registry values were too long.
#if defined(UNICODE) || defined(_UNICODE)
		size_t result_length = 0;
		wcstombs_s(&result_length, pPathStr, pathSize, defaultLUTpath, 259);
		wcstombs_s(&result_length, pDBStr, DBSize, DbNameStr, 63);
#else
		strncpy_s(pPathStr, pathSize, defaultLUTpath, 259);
		strncpy_s(pDBStr, DBSize, DbNameStr, 63);
#endif

#elif __APPLE_REMOVE__

		// This code has not been tested
		CFPropertyListRef	appValue;

		CFPreferencesAppSynchronize( CFSTR("com.cineform.codec") );

		/*  encoder not defined in this.
			OverridePathString not needed

		appValue = CFPreferencesCopyAppValue( CFSTR("OverridePath"),  CFSTR("com.cineform.codec") );
		//fprintf(stderr,"(Enc)AppValue for OverridePath = %08x\n",appValue);
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(encoder->OverridePathStr, pathStr);
			} else
			{
				strcpy(encoder->OverridePathStr, "/Library/Application Support/CineForm");
			}
		}
		else
		{
			strcpy(encoder->OverridePathStr, "/Library/Application Support/CineForm");
		}
		 */
		appValue = CFPreferencesCopyAppValue( CFSTR("LUTsPath"),  CFSTR("com.cineform.codec") );
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(pPathStr, pathStr);
			}
			else
			{
				strcpy(pPathStr, "/Library/Application Support/CineForm/LUTs");
			}
		}
		else
		{
			strcpy(pPathStr, "/Library/Application Support/CineForm/LUTs");
		}
		//strcpy(decoder->OverridePathStr, "/Library/Application Support/CineForm/LUTs");
		appValue = CFPreferencesCopyAppValue( CFSTR("CurrentDBPath"),  CFSTR("com.cineform.codec") );
		if ( appValue && CFGetTypeID(appValue) == CFStringGetTypeID()  ) {
			const char	*	pathStr = CFStringGetCStringPtr( (CFStringRef)appValue, kCFStringEncodingASCII);
			if(pathStr) {
				strcpy(pDBStr, pathStr);
			}
			else
			{
				if( !CFStringGetCString( (CFStringRef)appValue, pDBStr, 260, kCFStringEncodingASCII) ) {
					strcpy(pDBStr, "db");
				}
			}
		}
		else
		{
			strcpy(pDBStr, "db");
		}

#else
		// Initialize the default locations on Linux
		strcpy(pPathStr, LUT_PATH_STRING);
		strcpy(pDBStr, DATABASE_PATH_STRING);

		// Open the first user preferences file that exists
		//FILE *file = fopen(SETTINGS_PATH_STRING, "r");
		char pathname[PATH_MAX];
		FILE *file = OpenUserPrefsFile(pathname, sizeof(pathname));
		if (file)
		{
			SCANNER scanner;

			// Parse the preferences file and set parameters in the decoder
			CODEC_ERROR error = ParseUserMetadataPrefs(file, &scanner,
													   pPathStr, pathSize,
													   pDBStr, DBSize);
			if (error != CODEC_ERROR_OKAY)
			{
				// Restore the default paths
				strcpy(pPathStr, LUT_PATH_STRING);
				strcpy(pDBStr, DATABASE_PATH_STRING);

				// Report the error code and line number from the scanner
				FILE *logfile = OpenLogFile();
				if (logfile)
				{
					int error = scanner.error;
					fprintf(logfile, "Error %s line %d: %s (%d)\n", pathname, scanner.line, Message(error), error);
					fclose(logfile);
				}
			}

			fclose(file);
		}
#endif
	}
}
