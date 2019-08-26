////////////////////////////////////////////////////////////////////////////////////////
/*
	Camera
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../objects/camera.h"

static bool clampRenderingToGameWindow = false;
ConsoleCommand(clampRenderingToGameWindow, clampRenderingToGameWindow);

bool Camera::showGamplayCameraWindow = true;
ConsoleCommand(Camera::showGamplayCameraWindow, showGamplayCameraWindow);

bool Camera::followPlayer = true;
ConsoleCommand(Camera::followPlayer, cameraFollowPlayer);

////////////////////////////////////////////////////////////////////////////////////////
/*
	Camera Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

Camera::Camera(const XForm2& xf, float _minGameplayZoom, float _maxGameplayZoom, float _nearClip, float _farClip) :
	GameObject(xf),
	aspectRatio(1.0f),
	nearClip(_nearClip),
	farClip(_farClip),
	xfView(0),
	cameraMin(Vector2::Zero()),
	cameraMax(Vector2::Zero()),
	cameraGameMin(Vector2::Zero()),
	cameraGameMax(Vector2::Zero()),
	offset(Vector2::Zero()),
	minGameplayZoom(_minGameplayZoom),
	maxGameplayZoom(_maxGameplayZoom),
	lockedAspectRatio(0),
	zoom(_maxGameplayZoom),
	zoomLast(_maxGameplayZoom),
	matrixProjection(Matrix44::Identity()),
	fixedRotation(!Terrain::isCircularPlanet),
	allowZoom(true)
{
	ASSERT(!g_cameraBase);
}

void Camera::UpdateTransforms()
{
	// todo: make camera a child of the object it is following
	// gameplay mode: camera is wherever the player is
	// edit mode: camera is where editor wants it
	Vector2 desiredPos = GetPosWorld();
	if (followPlayer && g_gameControlBase->IsGameplayMode())
	{
		if (g_gameControlBase->GetPlayer())
		{
			if (g_gameControlBase->GetPlayer()->HasPhysics())
				desiredPos = g_gameControlBase->GetPlayer()->GetPhysicsXForm().position;
			else
				desiredPos = g_gameControlBase->GetPlayer()->GetPosWorld();
			desiredPos += offset;
		}
	}
	SetPosLocal(desiredPos);

	if (g_gameControlBase->IsEditMode() && fixedRotation)
	{
		// set angle to 0 when in fixed rotation mode for map editing
		SetAngleLocal(0);
	}
	else if (Terrain::isCircularPlanet)//if (GetPosLocal().MagnitudeSquared() > 10000)
	{
		/*float a = GetAngleLocal();
		float aDesired = CapAngle(GetPosLocal().GetAngle() - PI/2);
		float aDelta = CapAngle((aDesired - a));
		const float moveRate = 0.05f;
		if (aDelta > moveRate)
			aDelta = moveRate;
		else if (aDelta < -moveRate)
			aDelta = -moveRate;
		SetAngleLocal(a + aDelta);*/

		// rotate around the origin
		SetAngleLocal(GetPosLocal().GetAngle() - PI/2);
	}

	// set my view transform
	xfView = GetXFormLocal().Inverse();
	xfViewInterp = GetInterpolatedXForm().Inverse();
	
	GameObject::UpdateTransforms();
}

void Camera::PrepareForRender(const XForm2& xf, float zoom)
{
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	// set the view transform
	xfViewInterp = xf.Inverse();
	const Matrix44 matrixView = Matrix44::BuildXFormZ(xfViewInterp.position, xfViewInterp.angle, 10.0f);
	pd3dDevice->SetTransform( D3DTS_VIEW, &matrixView.GetD3DXMatrix() );

    // Set the projection matrix
	const float height = zoom * GetAspectFix();
	const float width = height * aspectRatio;
	D3DXMatrixOrthoLH( &matrixProjection.GetD3DXMatrix(), width, height, nearClip, farClip );
	pd3dDevice->SetTransform( D3DTS_PROJECTION, &matrixProjection.GetD3DXMatrix() );
	
	// update the camera culling window
	CalculateCameraWindow(true, &zoom);
}

void Camera::PrepForUpdate()
{
	// update camera aspect ratio
	SetAspectRatio(g_aspectRatio);
	
	zoomLast = GetZoom();

	// allow zooming with mouse wheel
	if (allowZoom || g_gameControlBase->IsEditMode())
	{
		// mouse zoom
		const bool debugZoom = GameControlBase::devMode;

		float zoomMin = minGameplayZoom;
		float zoomMax = maxGameplayZoom;
		if (debugZoom || !g_gameControlBase->IsGameplayMode())
		{
			zoomMin = 1;
			zoomMax = 10000000.0f;
		}

		const float lastZoom = zoom;
		if (g_input->IsDown(GB_Tab) && g_input->IsDown(GB_MouseRight))
		{
			const float zoomSpeed = 0.2f;
			zoom *= powf(NATURAL_LOG, g_input->GetMouseDeltaLocalSpace().y * zoomSpeed * GAME_TIME_STEP);
		}
		else
		{
			const float zoomSpeed = -0.03f;
			zoom *= powf(NATURAL_LOG, g_input->GetMouseWheel() * zoomSpeed * GAME_TIME_STEP);
		}

		//if (fabs(zoom - maxGameplayZoom) < 0.1f)
		//	zoom = maxGameplayZoom;
		//else 
		if (lastZoom < maxGameplayZoom && zoom > maxGameplayZoom || lastZoom > maxGameplayZoom && zoom < maxGameplayZoom)
		{
			// always use gameplay zoom when crossing the threshold
			// this is for debugging when zooming out to make it easier to get it back to the normal gameplay zoom
			zoom = maxGameplayZoom;
		}

		zoom = Cap(zoom, zoomMin, zoomMax);
	}

	CalculateCameraWindow();

	if (g_gameControlBase->IsGameplayMode() && zoom > maxGameplayZoom && showGamplayCameraWindow)
	{
		// show where the camera would clip
		Vector2 cameraMax = 0.5f * Vector2(maxGameplayZoom * aspectRatio, maxGameplayZoom);
		Vector2 cameraMin = -cameraMax;

		const Vector2 c1 = GetXFormLocal().TransformCoord(cameraMin);
		const Vector2 c2 = GetXFormLocal().TransformCoord(Vector2(cameraMin.x, cameraMax.y));
		const Vector2 c3 = GetXFormLocal().TransformCoord(cameraMax);
		const Vector2 c4 = GetXFormLocal().TransformCoord(Vector2(cameraMax.x, cameraMin.y));
		
		g_debugRender.RenderLine(c1, c2, Color::Red(0.5f), 0);
		g_debugRender.RenderLine(c2, c3, Color::Red(0.5f), 0);
		g_debugRender.RenderLine(c3, c4, Color::Red(0.5f), 0);
		g_debugRender.RenderLine(c4, c1, Color::Red(0.5f), 0);
	}
}

void Camera::UpdateAABB()
{
	const Vector2 p1 = GetXFormLocal().TransformCoord(cameraMin);
	const Vector2 p2 = GetXFormLocal().TransformCoord(Vector2(cameraMin.x, cameraMax.y));
	const Vector2 p3 = GetXFormLocal().TransformCoord(cameraMax);
	const Vector2 p4 = GetXFormLocal().TransformCoord(Vector2(cameraMax.x, cameraMin.y));
		
	cameraWorldAABBox.lowerBound.x = Min(Min(Min(p1.x, p2.x), p3.x), p4.x);
	cameraWorldAABBox.upperBound.x = Max(Max(Max(p1.x, p2.x), p3.x), p4.x);
	cameraWorldAABBox.lowerBound.y = Min(Min(Min(p1.y, p2.y), p3.y), p4.y);
	cameraWorldAABBox.upperBound.y = Max(Max(Max(p1.y, p2.y), p3.y), p4.y);
}

void Camera::CalculateCameraWindow(bool interpolate, const float* zoomOverride)
{
	const float actualAspectRatio = GetAspectFix() * aspectRatio;

	if (zoomOverride)
	{
		const float zoom = (*zoomOverride);
		cameraMax = 0.5f * Vector2(zoom * actualAspectRatio, zoom);
		cameraMin = -cameraMax;
		UpdateAABB();
		return;
	}
	else if (interpolate)
	{
		// use the larger zoom for this interpolation frame
		const float zoom = GetZoomInterpoalted();
		cameraMax = 0.5f * Vector2(zoom * actualAspectRatio, zoom);
		cameraMin = -cameraMax;
	}
	else
	{
		// use the larger zoom for this interpolation frame
		const float zoomLarger = zoom > zoomLast? zoom : zoomLast;
		cameraMax = 0.5f * Vector2(zoomLarger * actualAspectRatio, zoomLarger);
		cameraMin = -cameraMax;
	}
	
	cameraGameMax = 0.5f * Vector2(maxGameplayZoom * actualAspectRatio, maxGameplayZoom);
	cameraGameMin = -cameraGameMax;

	if (clampRenderingToGameWindow)
	{
		// clamp to what is actually being rendered
		cameraMax = cameraGameMax;
		cameraMin = cameraGameMin;
	}

	UpdateAABB();
}


void Camera::SaveScreenshot() const
{
	WCHAR filename[256];
	#define screenshotDirectory	L"snapshots/"
	#define screenshotPrefix	L"snapshot_"
	#define screenshotFileType	L"png"
	WIN32_FIND_DATA fileData;
	CreateDirectory(screenshotDirectory, NULL);
	HANDLE hFind = FindFirstFile( screenshotDirectory screenshotPrefix L"*." screenshotFileType, &fileData);

	// find the next one in sequence
	int count = 0;
	if (hFind  != INVALID_HANDLE_VALUE)
	{
		WCHAR lastFilename[255];

		do
		{
			swprintf_s(lastFilename, 256, L"%s", fileData.cFileName);
		}
		while (FindNextFile(hFind, &fileData));

		swscanf_s(lastFilename, screenshotPrefix L"%d%*s", &count);
		++count;
	}

	if (count < 10)
		swprintf_s(filename, 256, screenshotDirectory screenshotPrefix L"000%d." screenshotFileType, count);
	else if (count < 100)
		swprintf_s(filename, 256, screenshotDirectory screenshotPrefix L"00%d." screenshotFileType, count);
	else if (count < 1000)
		swprintf_s(filename, 256, screenshotDirectory screenshotPrefix L"0%d." screenshotFileType, count);
	else if (count < 10000)
		swprintf_s(filename, 256, screenshotDirectory screenshotPrefix L"%d." screenshotFileType, count);

	DXUTSnapD3D9Screenshot( filename );

	//g_debugMessageSystem.AddFormatted(L"Screenshot saved to \"%s\"", filename);
}

void Camera::RenderPost()
{
	if (lockedAspectRatio == 0 || g_gameControlBase->IsEditMode())
		return;

	// render black bars to cover area outside of limited aspect ratio
	const float w = (float)g_backBufferWidth;
	const float h = (float)g_backBufferHeight;

	if (aspectRatio > lockedAspectRatio)
	{
		Vector2 size(w * (1 - lockedAspectRatio / aspectRatio)/4, h/2);
		Vector2 pos1(w - size.x, h/2);
		Vector2 pos2(size.x, h/2);

		g_render->RenderScreenSpaceQuad(pos1, size, Color::Black());
		g_render->RenderScreenSpaceQuad(pos2, size, Color::Black());
	}
	else if (aspectRatio < lockedAspectRatio)
	{
		Vector2 size(w/2, h * (1 - aspectRatio / lockedAspectRatio)/4);
		Vector2 pos1(w/2, h - size.y);
		Vector2 pos2(w/2, size.y);

		g_render->RenderScreenSpaceQuad(pos1, size, Color::Black());
		g_render->RenderScreenSpaceQuad(pos2, size, Color::Black());
	}
}

float Camera::GetAspectFix() const
{
	if (g_gameControlBase->IsEditMode())
		return 1; // use normal aspect ratio when in edit mode

	// when using a locked aspect ratio we need to fix the aspect ratio so black bars can appear
	return (aspectRatio > lockedAspectRatio)? 1 : lockedAspectRatio / aspectRatio;
}

bool Camera::CameraConeTest(const XForm2& xf, float radius, float coneAngle) const
{
	// check if light is in range of camera
	if (!CameraTest(xf.position, radius))
		return false;

	if (coneAngle >= 2*PI)
		return true;

	// cull out cones that aren't on screen
	bool isOnScreen = false;
		
	// check direct line to center of screen
	const Vector2 localSpaceCamera = xf.Inverse().TransformCoord(GetPosWorld());
	const float localSpaceCameraAngle = localSpaceCamera.GetAngle();
	if (fabs(localSpaceCameraAngle) < coneAngle)
	{
		// quick center test
		if (g_cameraBase->CameraTest(Line2(xf.position, GetPosWorld())))
			isOnScreen = true;
	}

	// cast rays around the cone to check
	const float angleStep = PI/4;
	float angle = -coneAngle;
	bool lastCheck = false;
	while(!isOnScreen)
	{
		const Vector2 testPoint = XForm2(xf.position, angle + xf.angle).TransformCoord(Vector2(0, 10*GetMaxGameplayZoom()));
		if (CameraTest(Line2(xf.position, testPoint)))
			isOnScreen = true;
		angle += angleStep;
		if (angle >= coneAngle)
		{
			if (lastCheck)
				break;
			lastCheck = true;
			angle = coneAngle;
		}
	}

	return isOnScreen;
}