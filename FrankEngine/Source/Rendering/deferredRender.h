///////////////////////////////////////////////////////////////////////////////////////////////
/*
	Deferred Rendering System
	Copyright 2013 - Frank Force
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef DEFERRED_RENDER_H
#define DEFERRED_RENDER_H

class Light;

class DeferredRender
{
public:
	static void InitDeviceObjects();
	static void DestroyDeviceObjects();
	static void GlobalRender();
	static void GlobalUpdate();
	static int GetDynamicLightCount()				{ return dynamicLightCount; }
	static int GetSimpleLightCount()				{ return simpleLightCount; }
	static bool GetLightValue(const Vector2& pos, float& value, float sampleRadius = 1.0f);

	enum RenderPass
	{
		RenderPass_diffuse,
		RenderPass_emissive,
		RenderPass_lightShadow,
		RenderPass_directionalShadow,
		RenderPass_vision,
		RenderPass_normals,
		RenderPass_specular,
	};

	static RenderPass GetRenderPass()				{ return renderPass; }
	static bool GetRenderPassIsDiffuse()			{ return renderPass == RenderPass_diffuse; }
	static bool GetRenderPassIsEmissive()			{ return renderPass == RenderPass_emissive; }
	static bool GetRenderPassIsLight()				{ return renderPass == RenderPass_lightShadow; }
	static bool GetRenderPassIsDirectionalShadow()	{ return renderPass == RenderPass_directionalShadow; }
	static bool GetRenderPassIsVision()				{ return renderPass == RenderPass_vision; }
	static bool GetRenderPassIsNormalMap()			{ return renderPass == RenderPass_normals; }
	static bool GetRenderPassIsSpecular()			{ return renderPass == RenderPass_specular; }
	static bool GetRenderPassIsShadow()				{ return GetRenderPassIsLight() || GetRenderPassIsDirectionalShadow() || GetRenderPassIsVision(); }
	static bool GetRenderPassIsDeferred()			{ return GetRenderPassIsNormalMap() || GetRenderPassIsSpecular() || GetRenderPassIsEmissive(); }

	// helper class to start and end an emissive render block
	struct EmissiveRenderBlock
	{
		EmissiveRenderBlock(bool enable = true) { if (enable && GetRenderPassIsEmissive()) DXUTGetD3D9Device()->SetRenderState( D3DRS_AMBIENT, 0x00FFFFFF ); }
		~EmissiveRenderBlock() { if (GetRenderPassIsEmissive()) DXUTGetD3D9Device()->SetRenderState( D3DRS_AMBIENT, 0x00000000 ); }
	};
	
	// helper class to start and end a transparent render block
	struct TransparentRenderBlock
	{
		TransparentRenderBlock()	{  active = true; }
		~TransparentRenderBlock()	{ active = false; }
		static bool IsActive()		{ return active; }

		private:

		static bool active;
	};
	
	// helper class to start and end a specular render block
	struct SpecularRenderBlock
	{
		SpecularRenderBlock()	{  active = true; }
		~SpecularRenderBlock()	{ active = false; }
		static bool IsActive()	{ return active; }

		private:

		static bool active;
	};
	
	struct DiffuseNormalAlphaRenderBlock
	{
		DiffuseNormalAlphaRenderBlock(float _alphaScale)	{ {alphaScale = _alphaScale; active = true;} }
		~DiffuseNormalAlphaRenderBlock()					{ active = false; }
		static bool IsActive()								{ return active; }
		static float GetAlphaScale()						{ return alphaScale; }

		private:

		static float alphaScale;
		static bool active;
	};
	
	// helper class to start and end an additive render block
	struct AdditiveRenderBlock
	{
		AdditiveRenderBlock(bool enable = true) { if (enable && !(GetRenderPassIsNormalMap() || GetRenderPassIsSpecular())) DXUTGetD3D9Device()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); }
		~AdditiveRenderBlock() { DXUTGetD3D9Device()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); }
	};

	// global light settings
	static bool lightEnable;				// enable lighting
	static int maxDynamicLights;			// max limit on how many shadow and cone lights lights per frame
	static int maxSimpleLights;				// max limit on how many simple lights per frame
	static float alphaScale;				// globaly scales alpha of all light colors
	static float haloAlpha;					// globaly scales alpha of all halo colors
	static float textureSize;				// size of texture used for lightmapping
	static float shadowMapTextureSize;		// size of texture used for texture maps
	static Color ambientLightColor;			// color of ambient light
	static float shadowMapScale;			// how much bigger to make the shadow map
	static bool normalMappingEnable;		// toggles normal mapping
	static float lightNormalBias;			// how much to bias the normal dot product
	static float lightCullScale;			// how much to scale radius before camera 
	static bool visionEnable;				// final pass that creates a mask for the player's vision
	static float visionRadius;				// radius of visiom
	static float visionOverbrightRadius;	// overbright for vision
	static float visionSoftening;			// softening for vision
	static float visionCastScale;			// shadow cast scale for visio
	static int visionPassCount;				// how many vision passes to do
	static float visionTextureSize;			// size of texture used for vision
	static int visionPreBlurPassCount;		// blur settings for vision
	static int visionPostBlurPassCount;		// blur settings for vision
	static float defaultLightRadius;		// radius for point lights
	static bool use32BitTextures;			// determines format used for lighting texture
	static bool showFinalAccumulator;		// special rendering mode to only show accumulator texture

	// shadow casting pass settings
	static bool shadowEnable;				// enable lights casting shadows
	static float shadowSoftening;			// how soft are shadows
	static float shadowCastScale;			// how much to scale shadow cast each pass
	static int shadowPassCount;				// how many shadow passes to do
	static float shadowPassStartSize;		// how big to start stretching shadow by, 1 for best results 
	static float shadowLightHeightDefault;	// default height used for normal mapping
	static Color defaultShadowColor;		// default shadow color to use for objects

	// emissive pass settings
	static bool emissiveLightEnable;		// enable emsisive lighting pass
	static int emissiveBlurPassCount;		// how many emissive blurs to do
	static float emissiveBlurAlpha;			// how bright is the emissive blur
	static float emissiveBlurSize;			// how much to blur the emssive pass
	static Color emissiveBackgroundColor;	// background color for emissive pass
	static float emissiveTextureSize;		// size of emissive texturets
	static float emissiveAlpha;				// alpha for normal emissive blend
	static float emissiveBloomAlpha;		// alpha for emissive bloom blend

	// directional light pass settings
	static bool directionalLightEnable;			// enables directional lighting pass
	static Color directionalLightColor;			// color of directional light
	static Vector2 directionalLightDirection;	// direction of directional light
	static float directionalLightSoftening;		// how much to soften directional shadows
	static float directionalLightCastScale;		// how much to move directional cast each pass
	static float directionalLightHeight;		// height used for normal mapping
	static int directionalLightPassCount;		// number of passes for directional light
	static float directionalLightRedrawAlpha;	// alpha to use for directional redraw pass

	// specular settings
	static float specularPower;
	static float specularHeight;
	static float specularAmmount;

	// final pass settings
	static Vector2 finalTextureSize;		// size of final light overlay texture
	static float finalTextureSizeScale;		// how much to scale final texture based on back buffer size
	static float finalTextureCameraScale;	// how much extra to scale the final texture

	static RenderPass renderPass;
	static int dynamicLightCount;
	static int simpleLightCount;
	static bool lightDebug;
	static int showTexture;

private:

	static void RenderLightTexture(Light& light);
	static void RenderCone(Light& light, float angle);
	static void UpdateSimpleLight(Light& light, const XForm2& xfFinal, const Vector2& cameraSize);
	static void UpdateDynamicLight(Light& light);

	static XForm2 GetFinalTransform(const Vector2& textureRoundSize, Vector2* cameraSize = NULL, const float* zoomOverride = NULL);
	static void SetFiltering(bool forceLinear = false);

	static bool CreateTexture(const IntVector2& size, LPDIRECT3DTEXTURE9& texture, D3DFORMAT format = D3DFMT_X8R8G8B8);
	static LPDIRECT3DTEXTURE9& GetSwapTexture(LPDIRECT3DTEXTURE9& texture);
	static void SwapTextures(LPDIRECT3DTEXTURE9& texture1, LPDIRECT3DTEXTURE9& texture2);
	
	static float GetShadowMapZoom();
	static void UpdateSimpleLights(list<Light*> simpleLights);
	static void Invert(LPDIRECT3DTEXTURE9& texture, const Vector2& invertTextureSize);
	static void ApplyBlur(LPDIRECT3DTEXTURE9& texture, const Vector2& blurTextureSize, float brightness, float blurSize, int passCount = 1);
	static void RenderShadowMap();
	static void RenderNormalMap();
	static void RenderSpecularMap();
	static void RenderEmissivePass();
	static void RenderDirectionalPass();
	static void RenderVisionShadowMap();
	static void RenderVisionPass();

	static const int textureSwapStartSize = 32;
	static const int textureSwapArraySize = 6;
	static LPDIRECT3DTEXTURE9 textureSwapArray[textureSwapArraySize];
	static LPDIRECT3DTEXTURE9 texture;
	static LPDIRECT3DTEXTURE9 textureVision;
	static LPDIRECT3DTEXTURE9 textureShadowMap;
	static LPDIRECT3DTEXTURE9 textureShadowMapDirectional;
	static LPDIRECT3DTEXTURE9 textureNormalMap;
	static LPDIRECT3DTEXTURE9 textureSpecularMap;
	static LPDIRECT3DTEXTURE9 textureEmissive;
	static LPDIRECT3DTEXTURE9 textureFinal;
	static LPDIRECT3DPIXELSHADER9 deferredLightShader;
	static LPD3DXCONSTANTTABLE shadowLightConstantTable;
	static LPDIRECT3DPIXELSHADER9 visionShader;
	static LPD3DXCONSTANTTABLE visionConstantTable;
	static LPDIRECT3DPIXELSHADER9 blurShader;
	static LPD3DXCONSTANTTABLE blurConstantTable;
	static LPDIRECT3DPIXELSHADER9 directionalLightShader;
	static LPD3DXCONSTANTTABLE directionalLightConstantTable;
    static FrankRender::RenderPrimitive primitiveLightMask;
};

#endif // DEFERRED_RENDER_H