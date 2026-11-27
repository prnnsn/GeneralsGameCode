/*
**	Command & Conquer Generals Zero Hour(tm)
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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: ww3d2/texturefilter.cpp												$*
 *                                                                                             *
 *                  $Org Author:: Kenny Mitchell                                              $*
 *                                                                                             *
 *                       Author : Kenny Mitchell                                               *
 *                                                                                             *
 *                     $Modtime:: 08/05/02 1:27p                                              $*
 *                                                                                             *
 *                    $Revision:: 1                                                          $*
 *                                                                                             *
 * 08/05/02 KM Texture filter class abstraction																			*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "texturefilter.h"
#include "dx8wrapper.h" // MAX_TEXTURE_STAGES and legacy common definitions
#include "Renderer.h"

const char* const TextureFilterClass::TextureFilterModeString[] = {
	"None",
	"Point",
	"Bilinear",
	"Trilinear",
	"Anisotropic"
};
static_assert(ARRAY_SIZE(TextureFilterClass::TextureFilterModeString) == TextureFilterClass::TEXTURE_FILTER_COUNT, "Incorrect array size");

TextureFilterClass::TextureFilterMode TextureFilterClass::getTextureFilterMode(const char* str) {
	for (int i = 0; i < TextureFilterClass::TEXTURE_FILTER_COUNT; ++i) {
		if (stricmp(str, TextureFilterClass::TextureFilterModeString[i]) == 0) {
			return (TextureFilterClass::TextureFilterMode)i;
		}
	}

	return TextureFilterClass::TEXTURE_FILTER_NONE;
}

Renderer::TextureFilter _MinTextureFilters[MAX_TEXTURE_STAGES][TextureFilterClass::FILTER_TYPE_COUNT];
Renderer::TextureFilter _MagTextureFilters[MAX_TEXTURE_STAGES][TextureFilterClass::FILTER_TYPE_COUNT];
Renderer::TextureFilter _MipMapFilters[MAX_TEXTURE_STAGES][TextureFilterClass::FILTER_TYPE_COUNT];

/*************************************************************************
**                             TextureFilterClass
*************************************************************************/
TextureFilterClass::TextureFilterClass(MipCountType mip_level_count)
:	TextureMinFilter(FILTER_TYPE_DEFAULT),
	TextureMagFilter(FILTER_TYPE_DEFAULT),
	UAddressMode(TEXTURE_ADDRESS_REPEAT),
	VAddressMode(TEXTURE_ADDRESS_REPEAT)
{
	if (mip_level_count!=MIP_LEVELS_1)
	{
		MipMapFilter=FILTER_TYPE_DEFAULT;
	}
	else
	{
		MipMapFilter=FILTER_TYPE_NONE;
	}
}

//**********************************************************************************************
//! Apply filters (legacy)
/*!
*/
void TextureFilterClass::Apply(unsigned int stage)
{
	Renderer::Set_Texture_Min_Filter(stage, _MinTextureFilters[stage][TextureMinFilter]);
	Renderer::Set_Texture_Mag_Filter(stage, _MagTextureFilters[stage][TextureMagFilter]);
	Renderer::Set_Texture_Mip_Filter(stage, _MipMapFilters[stage][MipMapFilter]);

	switch (Get_U_Addr_Mode())
	{
	case TEXTURE_ADDRESS_REPEAT:
		Renderer::Set_Texture_Address_U(stage, Renderer::TextureAddress::Wrap);
		break;

	case TEXTURE_ADDRESS_CLAMP:
		Renderer::Set_Texture_Address_U(stage, Renderer::TextureAddress::Clamp);
		break;
	}

	switch (Get_V_Addr_Mode())
	{
	case TEXTURE_ADDRESS_REPEAT:
		Renderer::Set_Texture_Address_V(stage, Renderer::TextureAddress::Wrap);
		break;

	case TEXTURE_ADDRESS_CLAMP:
		Renderer::Set_Texture_Address_V(stage, Renderer::TextureAddress::Clamp);
		break;
	}
}

//**********************************************************************************************
//! Init filters (legacy)
/*!
*/
void TextureFilterClass::_Init_Filters(TextureFilterMode texture_filter, AnisotropicFilterMode anisotropy_level)
{
	const auto caps = Renderer::Get_Texture_Filter_Caps();

	// TheSuperHackers @info Init zero stage filter defaults, point filtering is the lowest type for non mip filtering
	_MinTextureFilters[0][FILTER_TYPE_NONE]=Renderer::TextureFilter::Point;
	_MagTextureFilters[0][FILTER_TYPE_NONE]=Renderer::TextureFilter::Point;
	_MipMapFilters[0][FILTER_TYPE_NONE]=Renderer::TextureFilter::None;

	// Bilinear
	_MinTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Linear;
	_MagTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Linear;
	_MipMapFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;

	// Anisotropic - MipMap interlayer filtering only goes up to linear
	_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Anisotropic;
	_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Anisotropic;
	_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;

	// TheSuperHackers @feature Mauller 08/03/2026 Add full support for all texture filtering modes;
	// None, Point, Bilinear, Trilinear, Anisotropic.
	switch (texture_filter) {

	default:
		// TheSuperHackers @info if we have an invalid filter_type, set the filtering to none
		DEBUG_CRASH(("Invalid filter type passed into TextureFilterClass::_Init_Filters()"));
		FALLTHROUGH;

	case TEXTURE_FILTER_NONE:

		_MinTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;
		_MagTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;
		_MipMapFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::None;

		_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::None;
		break;

	case TEXTURE_FILTER_POINT:

		_MinTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;
		_MagTextureFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;
		_MipMapFilters[0][FILTER_TYPE_FAST]=Renderer::TextureFilter::Point;

		_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		break;

	case TEXTURE_FILTER_BILINEAR:

		if (caps.minLinear) {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		if (caps.magLinear) {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		break;

	case TEXTURE_FILTER_TRILINEAR:

		if (caps.minLinear) {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		if (caps.magLinear) {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		if (caps.mipLinear) {
			_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			// TheSuperHackers @info if only linear mipmap filtering is unsupported,
			// Trilinear filtering becomes Bilinear filtering by default
			_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}
		break;

	case TEXTURE_FILTER_ANISOTROPIC:

		if (caps.minAnisotropic || caps.magAnisotropic) {
			// Set the Anisotropic filtering level for all stages
			_Set_Max_Anisotropy(anisotropy_level);
		}

		if (caps.minAnisotropic) {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Anisotropic;
		}
		else if (caps.minLinear) {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MinTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		if (caps.magAnisotropic) {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Anisotropic;
		}
		else if (caps.magLinear) {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MagTextureFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}

		if (caps.mipLinear) {
			_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MipMapFilters[0][FILTER_TYPE_BEST]=Renderer::TextureFilter::Point;
		}
		break;

	}


	// For stages above zero, set best filter to the same as the stage zero
	int i=1;
	for (;i<MAX_TEXTURE_STAGES;++i) {
		_MinTextureFilters[i][FILTER_TYPE_NONE]=_MinTextureFilters[0][FILTER_TYPE_NONE];
		_MagTextureFilters[i][FILTER_TYPE_NONE]=_MagTextureFilters[0][FILTER_TYPE_NONE];
		_MipMapFilters[i][FILTER_TYPE_NONE]=_MipMapFilters[0][FILTER_TYPE_NONE];

		_MinTextureFilters[i][FILTER_TYPE_FAST]=_MinTextureFilters[0][FILTER_TYPE_FAST];
		_MagTextureFilters[i][FILTER_TYPE_FAST]=_MagTextureFilters[0][FILTER_TYPE_FAST];
		_MipMapFilters[i][FILTER_TYPE_FAST]=_MipMapFilters[0][FILTER_TYPE_FAST];

		// When Anisotropic filtering is used, all stages above zero use trilinear filtering
		if (_MagTextureFilters[0][FILTER_TYPE_BEST]==Renderer::TextureFilter::Anisotropic) {
			_MagTextureFilters[i][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MagTextureFilters[i][FILTER_TYPE_BEST]=_MagTextureFilters[0][FILTER_TYPE_BEST];
		}

		if (_MinTextureFilters[0][FILTER_TYPE_BEST]==Renderer::TextureFilter::Anisotropic) {
			_MinTextureFilters[i][FILTER_TYPE_BEST]=Renderer::TextureFilter::Linear;
		}
		else {
			_MinTextureFilters[i][FILTER_TYPE_BEST]=_MinTextureFilters[0][FILTER_TYPE_BEST];
		}
		_MipMapFilters[i][FILTER_TYPE_BEST]=_MipMapFilters[0][FILTER_TYPE_BEST];

	}

	// Set default to best. The level of best filter mode is controlled by the input parameter.
	for (i=0;i<MAX_TEXTURE_STAGES;++i) {
		_MinTextureFilters[i][FILTER_TYPE_DEFAULT]=_MinTextureFilters[i][FILTER_TYPE_BEST];
		_MagTextureFilters[i][FILTER_TYPE_DEFAULT]=_MagTextureFilters[i][FILTER_TYPE_BEST];
		_MipMapFilters[i][FILTER_TYPE_DEFAULT]=_MipMapFilters[i][FILTER_TYPE_BEST];
	}

}


//**********************************************************************************************
//! Set mip mapping filter (legacy)
/*!
*/
void TextureFilterClass::Set_Mip_Mapping(FilterType mipmap)
{
//	if (mipmap != FILTER_TYPE_NONE && Get_Mip_Level_Count() <= 1 && Is_Initialized())
//	{
//		WWASSERT_PRINT(0, "Trying to enable MipMapping on texture w/o Mip levels!");
//		return;
//	}
	MipMapFilter=mipmap;
}

//**********************************************************************************************
//! Set anisotropic filter level
/*!
*/
void TextureFilterClass::_Set_Max_Anisotropy(AnisotropicFilterMode mode)
{
	for (int stage = 0; stage < MAX_TEXTURE_STAGES; ++stage)
		Renderer::Set_Texture_Max_Anisotropy(stage, mode);
}

//**********************************************************************************************
//! Set default min filter (legacy)
/*!
*/
void TextureFilterClass::_Set_Default_Min_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i)
	{
		_MinTextureFilters[i][FILTER_TYPE_DEFAULT]=_MinTextureFilters[i][filter];
	}
}


//**********************************************************************************************
//! Set default mag filter (legacy)
/*!
*/
void TextureFilterClass::_Set_Default_Mag_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i)
	{
		_MagTextureFilters[i][FILTER_TYPE_DEFAULT]=_MagTextureFilters[i][filter];
	}
}

//**********************************************************************************************
//! Set default mip filter (legacy)
/*!
*/
void TextureFilterClass::_Set_Default_Mip_Filter(FilterType filter)
{
	for (int i=0;i<MAX_TEXTURE_STAGES;++i)
	{
		_MipMapFilters[i][FILTER_TYPE_DEFAULT]=_MipMapFilters[i][filter];
	}
}
