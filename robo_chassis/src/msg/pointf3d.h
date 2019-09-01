#ifndef POINTF3D_H
#define POINTF3D_H

struct PointF3D
{
    double x;
    double y;
    double z;

	const PointF3D operator-(const PointF3D& rhs) const
	{
		return {x - rhs.x, y - rhs.y, z - rhs.z};
	}
};


#endif // POINTF3D_H
