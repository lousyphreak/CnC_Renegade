/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/******************************************************************************
*
* FILE
*     $Archive: /Commando/Code/WWOnline/WOLProduct.h $
*
* DESCRIPTION
*     This class specifies product-specific information, such as SKU.
*
*     Client code should create a Product::Initializer object. This will create
*     a Product object and set it as the current product. Creating additional
*     Initializer objects will replace the current product information. This
*     will change the application's "identity" on the fly; this may cause
*     problems if Westwood Online activity is in progress. I don't expect there
*     to be any need to do this, except perhaps during early product development.
*
* PROGRAMMER
*     $Author: Denzil_l $
*
* VERSION INFO
*     $Revision: 4 $
*     $Modtime: 1/25/02 6:45p $
*
******************************************************************************/

#ifndef __WOLPRODUCT_H__
#define __WOLPRODUCT_H__

#include <cstdint>

#include "RefCounted.h"
#include "RefPtr.h"
#include <wwlib/wwstring.h>
#include <wwlib/widestring.h>

namespace WWOnline {

class Product :
		public RefCounted
	{
	public:
		static RefPtr<Product> Current(void);

		const char* GetRegistryPath(void) const
			{return mRegistryPath;}

		uint32_t GetSKU(void) const
			{return mProductSKU;}

		uint32_t GetLanguageSKU(void) const
			{return mProductSKU | mLanguageCode;}

		uint32_t GetLadderSKU(void) const
			{return mLadderSKU;}

		uint32_t GetLanguageCode(void) const
			{return mLanguageCode;}

		uint32_t GetVersion(void) const
			{return mProductVersion;}

		int32_t GetGameCode(void) const
			{return mGameCode;}

		const wchar_t* GetChannelPassword(void) const
			{return mChannelPassword;}

		class Initializer
			{
			public:
				Initializer(const char* registryPath, int gameCode, const wchar_t* chanPass, uint32_t ladderSKU);
				~Initializer();
			};

	private:
		friend class Initializer;
		static RefPtr<Product> Create(const char* registryPath, int gameCode, const wchar_t* chanPass, uint32_t ladderSKU);

		Product(const char* registryPath, int gameCode, const wchar_t* chanPass, uint32_t ladderSKU);

		StringClass mRegistryPath;
		uint32_t mProductSKU;
		uint32_t mProductVersion;
		uint32_t mLanguageCode;
		uint32_t mLadderSKU;
		int32_t mGameCode;
		WideStringClass mChannelPassword;
	};

}
#endif // __WOLPRODUCT_H__
