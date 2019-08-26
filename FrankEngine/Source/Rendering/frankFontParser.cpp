////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Font System
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include <string>
#include "../rendering/frankFont.h"
	
bool FrankFont::ParseFont(istream& Stream, CharSet& CharSetDesc)
{
	string Line;
	string Read, Key, Value;
	std::size_t i;
	while( !Stream.eof() )
	{
		std::stringstream LineStream;
		std::getline( Stream, Line );
		LineStream << Line;

		//read the line's type
		LineStream >> Read;
		if( Read == "common" )
		{
			//this holds common data
			while( !LineStream.eof() )
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find( '=' );
				Key = Read.substr( 0, i );
				Value = Read.substr( i + 1 );

				//assign the correct value
				Converter << Value;
				if( Key == "lineHeight" )
					Converter >> CharSetDesc.lineHeight;
				else if( Key == "base" )
					Converter >> CharSetDesc.base;
				else if( Key == "scaleW" )
					Converter >> CharSetDesc.scale.x;
				else if( Key == "scaleH" )
					Converter >> CharSetDesc.scale.y;
				else if( Key == "pages" )
					Converter >> CharSetDesc.pages;
			}
		}
		else if( Read == "char" )
		{
			//this is data for a specific char
			unsigned short CharID = 0;

			while( !LineStream.eof() )
			{
				std::stringstream Converter;
				LineStream >> Read;
				i = Read.find( '=' );
				Key = Read.substr( 0, i );
				Value = Read.substr( i + 1 );

				//assign the correct value
				Converter << Value;
				if( Key == "id" )
				{
					Converter >> CharID;
					ASSERT(CharID < 256); // only basic ascii set of 256 chars is supported
				}
				else if( Key == "x" )
					Converter >> CharSetDesc.Chars[CharID].position.x;
				else if( Key == "y" )
					Converter >> CharSetDesc.Chars[CharID].position.y;
				else if( Key == "width" )
					Converter >> CharSetDesc.Chars[CharID].size.x;
				else if( Key == "height" )
					Converter >> CharSetDesc.Chars[CharID].size.y;
				else if( Key == "xoffset" )
					Converter >> CharSetDesc.Chars[CharID].offset.x;
				else if( Key == "yoffset" )
					Converter >> CharSetDesc.Chars[CharID].offset.y;
				else if( Key == "xadvance" )
					Converter >> CharSetDesc.Chars[CharID].advanceX;
				else if( Key == "page" )
					Converter >> CharSetDesc.Chars[CharID].page;
			}
		}
	}

	return true;
}