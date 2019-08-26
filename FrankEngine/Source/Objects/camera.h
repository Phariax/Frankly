////////////////////////////////////////////////////////////////////////////////////////
/*
	Camera
	Copyright 2013 Frank Force - http://www.frankforce.com

	- can be attached to an object
	- culls out stuff that isn't onscreen
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMERA_H
#define CAMERA_H

#include "../objects/gameObject.h"

class Camera : public GameObject
{
public:

	Camera(const XForm2& xf = XForm2::Identity(), float _minGameplayZoom = 5, float _maxGameplayZoom = 15, float _nearClip = 1.0f, float _farClip = 1000.0f);

	void SetNearClip(float _nearClip) { nearClip = _nearClip; }
	void SetFarClip(float _farClip) { farClip = _farClip; }

	void SetZoom(const float _zoom) { zoom = _zoom; }
	void SetZoomLast(const float _zoom) { zoomLast = _zoom; }
	float GetZoom() const { return zoom; }
	float GetZoomInterpoalted() const { return zoom - g_interpolatePercent * (zoom - zoomLast); }
	const Matrix44& GetMatrixProjection() const { return matrixProjection; }
	
	void SetMinGameplayZoom(float zoom) { minGameplayZoom = zoom; }
	void SetMaxGameplayZoom(float zoom) { maxGameplayZoom = zoom; }
	float GetMinGameplayZoom() const { return minGameplayZoom; }
	float GetMaxGameplayZoom() const { return maxGameplayZoom; }

	// returns true if circle is at least partially on screen
	bool CameraTest(const Vector2& pos, float radius = 0) const;
	bool CameraTest(const Circle& circle) const { return CameraTest(circle.position, circle.radius); }
	bool CameraTest(const Line2& line) const;
	bool CameraConeTest(const XForm2& xf, float radius = 0, float coneAngle = 0) const;
	bool CameraGameTest(const Vector2& pos, float radius = 0) const;
	bool CameraTest(const Box2AABB& aabbox) const { return aabbox.PartiallyContains(cameraWorldAABBox); }

	float CameraWindowDistance(const Vector2& pos) const;
	void CalculateCameraWindow(bool interpolate = false, const float* zoom = NULL);

	void PrepareForRender() { PrepareForRender(GetInterpolatedXForm(), GetZoomInterpoalted()); }
	void PrepareForRender(const XForm2& xf, float zoom);
	void SaveScreenshot() const;
	bool IsPersistant() const { return true; }
	bool DestroyOnWorldReset() const { return false; }

	void SetAllowZoom(bool _allowZoom) { allowZoom = _allowZoom; }
	bool GetAllowZoom() const { return allowZoom; }

	void SetFixedRotation(bool _fixedRotation) { fixedRotation = _fixedRotation; }
	bool GetFixedRotation() const { return fixedRotation; }
	
	Vector2 GetOffset() const { return offset; }
	void SetOffset(const Vector2& _offset) { offset = _offset; }
	
	void SetAspectRatio(float _aspectRatio) { aspectRatio = _aspectRatio; }
	float GetAspectRatio() const { return aspectRatio; }
	Vector2 GetAspectRatioScale() const { return Vector2(aspectRatio, 1); }

	void SetLockedAspectRatio(float a) { lockedAspectRatio = a; }
	float GetLockedAspectRatio() const { return lockedAspectRatio; }
	float GetAspectFix() const;
	
	void RenderPost();
	void PrepForUpdate();
	
	static bool followPlayer;
	static bool showGamplayCameraWindow;

protected:
	
	void UpdateAABB();
	void UpdateTransforms();

	XForm2 xfView;
	XForm2 xfViewInterp;
	Vector2 cameraMin;
	Vector2 cameraMax;
	Vector2 cameraGameMin;
	Vector2 cameraGameMax;
	Vector2 offset;
	Box2AABB cameraWorldAABBox;

	float zoom;
	float zoomLast;
	float minGameplayZoom;
	float maxGameplayZoom;
	float lockedAspectRatio;

	float aspectRatio;
	float nearClip;
	float farClip;

	bool fixedRotation;
	bool allowZoom;

	Matrix44 matrixProjection;
};

// returns true if circle would be visible at the max possible gameplay zoom
inline bool Camera::CameraGameTest(const Vector2& pos, float radius) const
{
	const Vector2 localPos = xfView.TransformCoord(pos);
	const Vector2 localBounds = Vector2(radius, radius);
	const Vector2 localMax = localPos + localBounds;
	const Vector2 localMin = localPos - localBounds;

	return (localMin.x < cameraGameMax.x && localMax.x > cameraGameMin.x
			&& localMin.y < cameraGameMax.y && localMax.y > cameraGameMin.y);
}

inline float Camera::CameraWindowDistance(const Vector2& pos) const
{
	const Vector2 localPos = xfViewInterp.TransformCoord(pos);
	if (localPos.x < cameraMax.x && localPos.x > cameraMin.x && localPos.y < cameraMax.y && localPos.y > cameraMin.y)
		return 0;

	Vector2 nearestPos = localPos;
	nearestPos.x = Cap(nearestPos.x, cameraMin.x, cameraMax.x);
	nearestPos.y = Cap(nearestPos.y, cameraMin.y, cameraMax.y);

	return (localPos - nearestPos).Magnitude();
}

// returns true if circle is at least partially on screen
inline bool Camera::CameraTest(const Vector2& pos, float radius) const
{
	const Vector2 localPos = xfView.TransformCoord(pos);
	const Vector2 localBounds = Vector2(radius, radius);
	const Vector2 localMax = localPos + localBounds;
	const Vector2 localMin = localPos - localBounds;

	return (localMin.x < cameraMax.x && localMax.x > cameraMin.x
			&& localMin.y < cameraMax.y && localMax.y > cameraMin.y);
}

// returns true if line segment is at least partially on screen
inline bool Camera::CameraTest(const Line2& line) const
{
	const Line2 l(xfView.TransformCoord(line.p1), xfView.TransformCoord(line.p2));

	if (Vector2::LineSegmentIntersection(l.p1, l.p2, Vector2(cameraMin.x, cameraMin.y), Vector2(cameraMax.x, cameraMin.y)))
		return true;
	if (Vector2::LineSegmentIntersection(l.p1, l.p2, Vector2(cameraMin.x, cameraMax.y), Vector2(cameraMax.x, cameraMax.y)))
		return true;
	if (Vector2::LineSegmentIntersection(l.p1, l.p2, Vector2(cameraMin.x, cameraMin.y), Vector2(cameraMin.x, cameraMax.y)))
		return true;
	if (Vector2::LineSegmentIntersection(l.p1, l.p2, Vector2(cameraMax.x, cameraMin.y), Vector2(cameraMax.x, cameraMax.y)))
		return true;

	return false;
}

#endif // CAMERA_H