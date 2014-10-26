#pragma once

class MathUtility
{
public:
	MathUtility(void);
	~MathUtility(void);

	inline static float linearInterpolate(const QPointF& p0, const QPointF& p1, float x)
	{
		float dx = p1.x() - p0.x();
		float dy = p1.y() - p0.y();
		return dy / dx * (x - p0.x()) + p0.y();
	}

	inline static float linearInterpolateClamp(const QPointF& p0, const QPointF& p1, float x)
	{
		float dx = p1.x() - p0.x();
		float dy = p1.y() - p0.y();
		float r  = (x - p0.x()) / dx;
		r = max(min(r, 1.f), 0.f);
		return dy  * r + p0.y();
	}
};
