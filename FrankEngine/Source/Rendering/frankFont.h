////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Font System
	Copyright 2013 Frank Force - http://www.frankforce.com
	
	- uses BMFont to generate font textures - http://www.angelcode.com/products/bmfont/
	- some code is from Promit's tutorial - http://www.gamedev.net/topic/330742-quick-tutorial-variable-width-bitmap-fonts/
	- fonts are loaded from texture
	- fonts can be rendered in world or screen space and scaled
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_FONT_H
#define FRANK_FONT_H

enum FontFlags
{
	FontFlag_None		= 0x00,
	FontFlag_CenterX	= 0x01,
	FontFlag_CenterY	= 0x02,
	FontFlag_CastShadows= 0x04,			// allows font to cast shadows as if it were a foreground object
	FontFlag_NoNormals	= 0x08,			// skip normals pass, allows normal map below it to show through
	FontFlag_AlignRight	= 0x10,
};

struct FrankFont
{
	FrankFont(GameTextureID texture, WCHAR* dataFilename);

	static void InitDeviceObjects();
	static void DestroyDeviceObjects();
	
	void Render(const WCHAR* string, const XForm2& xf, float size, const Color& color = Color::White(), FontFlags flags = FontFlag_None, bool cameraCheck = true);
	void Render(const char* string, const XForm2& xf, float size, const Color& color = Color::White(), FontFlags flags = FontFlag_None, bool cameraCheck = true);
	void Render(const char* string, const Matrix44& matrix, const Color& color = Color::White(), FontFlags flags = FontFlag_None, bool screenSpace = false);
	
	void RenderScreenSpace(const WCHAR* string, const XForm2& xf, float size, const Color& color = Color::White(), FontFlags flags = FontFlag_None);
	void RenderScreenSpace(const char* string, const XForm2& xf, float size, const Color& color = Color::White(), FontFlags flags = FontFlag_None);

	Box2AABB GetBBox(const char* string, const XForm2& xf, float size, FontFlags flags = FontFlag_None);

private:
	
	struct CharDescriptor
	{
		Vector2 position;
		Vector2 size;
		Vector2 offset;
		float advanceX;
		float page;

		CharDescriptor() : position(0), size(0), offset(0), advanceX(0), page(0) {}
	};

	struct CharSet
	{
		float lineHeight;
		float base;
		float pages;
		Vector2 scale;
		CharDescriptor Chars[256];
	};
	
	struct FontVertex
	{
		Vector3 position;
		Vector2 textureCoords;
	};
	
	void GetLineBounds(const char* string, float& minX, float& maxX);	
	void GetBounds(const char* string, Vector2& minPos, Vector2& maxPos);
	void BuildTriStrip(const char* string, FontFlags flags = FontFlag_None);

	static bool ParseFont(istream& Stream, CharSet& CharSetDesc);

	CharSet charSet;
	GameTextureID texture;

	static FrankRender::RenderPrimitive primitiveTris;
	static const int maxStringSize = 1024;
};

#endif //FRANK_FONT_H