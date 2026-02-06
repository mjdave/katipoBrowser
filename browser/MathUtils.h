
#ifndef __World__MathUtils__
#define __World__MathUtils__

#define GLM_FORCE_PURE 1
#define GLM_ENABLE_EXPERIMENTAL 1
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1

#include <stdio.h>
#include <string>
#include <vector>
#include <functional>
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/matrix_access.hpp"
#include "gtc/type_ptr.hpp"
#include "gtx/quaternion.hpp"
#include "gtx/closest_point.hpp"
#include "gtx/intersect.hpp"
#include "gtc/epsilon.hpp"
#include "gtx/vector_angle.hpp"

#include "MJLog.h"

using namespace glm;

#define RANDOM_FLOAT() (((float)rand()) / RAND_MAX)
#define RANDOM_DOUBLE() (((double)rand()) / RAND_MAX)

#if !defined(MIN)
#define MIN(__a,__b)	( (__a) < (__b) ? (__a) : (__b) )
#endif

#if !defined(MAX)
#define MAX(__a,__b)	( (__a) < (__b) ? (__b) : (__a) )
#endif

enum  {
    MJHorizontalAlignmentLeft,
    MJHorizontalAlignmentCenter,
    MJHorizontalAlignmentRight,
};


#ifdef _MSC_VER
#define MJ_PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
#else
#define MJ_PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif


typedef uint64_t uint64_t;


double randomValueForPositionAndSeed(dvec3 pos, uint32_t seed);
double randomValueForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed);
bool randomBoolForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed);
uint32_t randomIntegerValueForUniqueIDAndSeed(uint64_t uniqueID, uint32_t seed, uint32_t max);

uint32_t randomIntegerValueForUniqueIDAndSeedWithLogging(uint64_t uniqueID, uint32_t seed, uint32_t max);
uint32_t randomIntegerValueForUniqueIDAndSeedCompatibilityMode(uint64_t uniqueID_, uint32_t seed, uint32_t max, bool macCompatibilityMode);

bool getLocalOSPadsWithZeroCasted32(uint64_t uniqueID);

uint32_t seedValueFromSeedString(std::string seedString);

inline double reverseLinearInterpolate(double x, double a, double b)
{
    if(b == a)
    {
        return 0.0f;
    }
    
    return (x - a) / (b - a);
}

struct dtri
{
    dvec3 a;
    dvec3 b;
    dvec3 c;
    
    dtri(dvec3 a, dvec3 b, dvec3 c)
    {
        this->a = a;
        this->b = b;
        this->c = c;
    }
};

struct irect
{
    ivec2 origin;
    ivec2 size;
    irect(ivec2 origin, ivec2 size)
    {
        this->origin = origin;
        this->size = size;
    }
    irect()
    {
        this->origin = ivec2(0,0);
        this->size = ivec2(0,0);
    }
};

struct drect
{
    dvec2 origin;
    dvec2 size;
    drect(dvec2 origin, dvec2 size)
    {
        this->origin = origin;
        this->size = size;
    }
};

inline bool pointIsLeftOfLine(const dvec3& p1, const dvec3& a, const dvec3& b)
{
    dvec3 faceNormal = cross(a, b);
    return (dot(faceNormal, p1) >= 0);
}


inline bool pointIsLeftOfLinePreCrossed(const dvec3& p1, const dvec3& faceNormal)
{
	return (dot(faceNormal, p1) >= 0);
}


inline bool pointInTerrainTriangle(const dvec3& point,
	const dvec3& a,
	const dvec3& b,
	const dvec3& c)
{
    return(dot(point, a) > 0 &&
           pointIsLeftOfLine(point, c, b) &&
           pointIsLeftOfLine(point, a, c) &&
           pointIsLeftOfLine(point, b, a));
}

inline int closestVertIndex(const dvec3& point,
	const dvec3& a,
	const dvec3& b,
	const dvec3& c)
{
	double aDot = dot(a,point);
	double bDot = dot(b,point);
	double cDot = dot(c,point);
	if(aDot > bDot)
	{
		if(cDot > aDot)
		{
			return 2;
		}
		return 0;
	}
	if(cDot > bDot)
	{
		return 2;
	}
	return 1;
}

inline bool rayIntersectsTriangle2D(dvec2 rayOrigin, 
    dvec2 rayDirection, 
    dvec2 triVertA,
    dvec2 triVertB,
    dvec2 triVertC,
    dvec2* outIntersectionPoint)
{
    const double EPSILON = 0.0000001;
    //dvec2 s, q;
    //double a,f,u,v;

    dvec2 edge1 = triVertB - triVertA;
    dvec2 edge2 = triVertC - triVertA;

    dvec2 h = dvec2(-rayDirection.y, rayDirection.x);

    double a = dot(edge1, h);
    if (a > -EPSILON && a < EPSILON)
        return false;    // This ray is parallel to edge1.

    double f = 1.0/a;
    dvec2 s = rayOrigin - triVertA;
    double u = f * dot(s,h);
    if (u < 0.0 || u > 1.0)
        return false;

    dvec2 q = dvec2(-s.y, s.x);//s.crossProduct(edge1);
    double v = f * dot(rayDirection, q);
    if (v < 0.0 || u + v > 1.0)
        return false;
    // At this stage we can compute t to find out where the intersection point is on the line.
    double t = f * dot(edge2, q);
    if (t > EPSILON) // ray intersection
    {
        if(outIntersectionPoint)
        {
            *outIntersectionPoint = rayOrigin + rayDirection * t;
        }
        return true;
    }
    else // This means that there is a line intersection but not a ray intersection.
        return false;
}

inline dvec3 baryFractions(const dvec3& pointNormal,
	const dvec3& vN0,
	const dvec3& vN1,
	const dvec3& vN2)
{ 
    dvec3 fractions;
    
    dvec3 e1 = vN1 - vN0;
    dvec3 p0 = pointNormal - vN0;
    dvec3 a0 = cross(e1, p0);
    fractions.z = length(a0);
    
    dvec3 e4 = vN2 - vN1;
    dvec3 p1 = pointNormal - vN1;
    dvec3 a1 = cross(e4, p1);
    fractions.x = length(a1);
    
    dvec3 e5 = vN0 - vN2;
    dvec3 p2 = pointNormal - vN2;
    dvec3 a2 = cross(e5, p2);
    fractions.y = length(a2);
    
    return fractions / (fractions.x + fractions.y + fractions.z);
}

inline quat slerpNoInvert(const quat &q1, const quat &q2, float t)
{
    float dotProduct = dot(q1, q2);
    
    if (dotProduct > -0.95f && dotProduct < 0.95f)
    {
        float angle = acosf(dotProduct);
        return (q1*sinf(angle*(1-t)) + q2*sinf(angle*t))/sinf(angle);
    } else  // if the angle is small, use linear interpolation
        return lerp(q1,q2,t);
}

inline quat bezier(const quat &qAA,const quat &qA,const quat &qB,const quat &qBB,float t, float duration)
{
    // level 1
    /*quat q11= normalize(slerp(qAA,qA,t));
    quat q12= normalize(slerp(qA,qB,t));
    quat q13= normalize(slerp(qB,qBB,t));
    // level 2 and 3
    return normalize(slerp(q11,q12,t));//slerp(slerp(q11,q12,t), slerp(q12,q13,t), t);*/
    
    /*float slerpT = 2.0 * t * (1.0 - t);
    
    quat slerp1 = slerpNoInvert(qA,qB,t);
    quat slerp2 = slerpNoInvert(qAA,qBB,t);
    
    return slerpNoInvert(slerp1, slerp2, slerpT);*/
    
    /*float slerpT = 2.0 * t * (1.0 - t);
    quat slerp1 = slerp(qAA,qBB,t);
    quat slerp2 = slerp(qA,qB,t);
    float slerpFactor = 1.0f;//min(duration, 1.0f);
    
    return slerp(slerp2, slerp1, slerpT * slerpFactor);*/
    
    quat slerp1 = slerp(qA,qAA,t);
    quat slerp2 = slerp(qAA,qBB,t);
    quat slerp3 = slerp(qBB,qB,t);
    
    quat veryloopy = slerp(slerp(slerp1, slerp2, t), slerp(slerp2, slerp3, t), t);
    
    return slerp(slerp(qA, qB, t), veryloopy, 0.5f * min(duration, 2.0f));
}

inline quat controlPoint(const quat &qnm1,const quat &qn)
{
    //quat qni(qn.w, -qn.x, -qn.y, -qn.z);
    //return qn * exp(( log(qni*qnm1) + log(qni*qnp1) )/-4.0f);
    quat sub = qn * conjugate(qnm1);
    return qn * sub;
}

inline quat bezierNew(const quat &qAA,const quat &qA,const quat &qB,const quat &qBB,float t)
{
	quat slerpB = slerp(qB,qBB,t * 0.5f);
	quat slerpA = slerp(qA,qB,0.5f + t * 0.5f);
	return slerp(slerpA,slerpB,t);
}

inline vec3 daveBoneTranslationLerp(vec3 aa, vec3 a, vec3 b, vec3 bb, float t)
{
	vec3 slerpB = mix(b,bb,t * 0.5f);
	vec3 slerpA = mix(a,b,0.5f + t * 0.5f);
	return mix(slerpA,slerpB,t);
}


inline vec3 cardinalSplineInterpolate(vec3 aa, vec3 a, vec3 b, vec3 bb, float fraction, float factor = 0.5f)
{
    float fSquared = fraction * fraction;
    float fCubed = fSquared * fraction;
    
    float h1 =  2.0f * fCubed - 3.0f * fSquared + 1.0f;
    float h2 = -2.0f * fCubed + 3.0f * fSquared;
    float h3 =         fCubed - 2.0f * fSquared + fraction;
    float h4 =         fCubed - fSquared;
    
    vec3 t1 = (b - aa) * factor;
    vec3 t2 = (bb - a) * factor;
    
    vec3 p = a*h1 +
    b*h2 +
    t1*h3 +
    t2*h4;
    
    return p;
}

/*inline quat bezier(const quat &aa,const quat &a,const quat &b,const quat &bb,float fraction, float factor = 0.5f)
{
    float fSquared = fraction * fraction;
    float fCubed = fSquared * fraction;
    
    float h1 =  2.0f * fCubed - 3.0f * fSquared + 1.0f;
    float h2 = -2.0f * fCubed + 3.0f * fSquared;
    float h3 =         fCubed - 2.0f * fSquared + fraction;
    float h4 =         fCubed - fSquared;
    
   // quat slerp1 = slerp(qAA,qBB,t);
    
    quat t1 = (b - aa) * factor;
    quat t2 = (bb - a) * factor;
    
    quat p = a*h1 +
    b*h2 +
    t1*h3 +
    t2*h4;
    
    return normalize(p);
}*/

inline float easeInEaseOut(float fractionIn, float powerIn, float powerOut)
{
    return fractionIn < 0.5f ? powf(fractionIn * 2.0f, clamp(powerIn, 0.1f, 10.0f)) * 0.5f : (1.0f - powf(1.0f - ((fractionIn - 0.5f) * 2.0f), clamp(powerOut, 0.1f, 10.0f))) * 0.5f + 0.5f;
}

inline void printMat3(dmat3 matrix)
{
    const double *pSource = (const double*)value_ptr(matrix);
    MJLog("%.2f, %.2f, %.2f\n%.2f, %.2f, %.2f\n%.2f, %.2f, %.2f\n", pSource[0], pSource[1], pSource[2],
          pSource[3], pSource[4], pSource[5],
          pSource[6], pSource[7], pSource[8]);
}

inline void printMat4(dmat4 matrix)
{
	const double *pSource = (const double*)value_ptr(matrix);
	MJLog("%.4f, %.4f, %.4f, %.4f\n%.4f, %.4f, %.4f, %.4f\n%.4f, %.4f, %.4f, %.4f\n%.4f, %.4f, %.4f, %.4f\n", pSource[0], pSource[1], pSource[2], pSource[3],
		pSource[4], pSource[5], pSource[6], pSource[7],
		pSource[8], pSource[9], pSource[10], pSource[11],
		pSource[12], pSource[13], pSource[14], pSource[15]);
}

inline void printVec3(dvec3 vec)
{
    MJLog("%.9f, %.9f, %.9f", vec.x, vec.y, vec.z);
}

inline bool approxEqualVec(const dvec3& a, const dvec3& b)
{
    return glm::all(epsilonEqual(a, b, 0.000000000000001));
}

inline bool approxEqualVec2(dvec2 a, dvec2 b)
{
	return glm::all(epsilonEqual(a, b, 0.000000000000001));
}

inline bool approxEqual(double a, double b)
{
    return epsilonEqual(a, b, 0.000000000000001);
}



template<typename T, qualifier Q>
inline tquat<T, Q> animationSlerp(tquat<T, Q> const& x, tquat<T, Q> const& y, T a)
{
	tquat<T, Q> z = y;

	T cosTheta = dot(x, y);

	/*if(abs(cosTheta) < 0.01)
	{
		return y;
	}*/

	// If cosTheta < 0, the interpolation will take the long way around the sphere. 
	// To fix this, one quat must be negated.
	if (cosTheta < T(0))
	{
		z        = -y;
		cosTheta = -cosTheta;
	}

	// Perform a linear interpolation when cosTheta is close to 1 to avoid side effect of sin(angle) becoming a zero denominator
	if(cosTheta > T(1) - epsilon<T>())
	{
		// Linear interpolation
		return tquat<T, Q>(
			mix(x.w, z.w, a),
			mix(x.x, z.x, a),
			mix(x.y, z.y, a),
			mix(x.z, z.z, a));
	}
	else
	{
		// Essential Mathematics, page 467
		T angle = acos(cosTheta);
		return (sin((T(1) - a) * angle) * x + sin(a * angle) * z) / sin(angle);
	}
}

inline void quickSort(std::vector<double>& numbers, std::vector<unsigned int>& orderedIndices, int low, int high)
{
    int i = low;
    int j = high;
    double pivot = numbers[orderedIndices[(i + j) / 2]];
    int temp;
    
    while (i <= j)
    {
        while (numbers[orderedIndices[i]] < pivot)
            i++;
        while (numbers[orderedIndices[j]] > pivot)
            j--;
        if (i <= j)
        {
            temp = orderedIndices[i];
            orderedIndices[i] = orderedIndices[j];
            orderedIndices[j] = temp;
            i++;
            j--;
        }
    }
    if (j > low)
        quickSort(numbers, orderedIndices, low, j);
    if (i < high)
        quickSort(numbers, orderedIndices, i, high);
}

inline void insertionSort(std::vector<double>& numbers, std::vector<unsigned int>& orderedIndices)
{
    int count = numbers.size();
    for(int i = 1; i < count; i++)
    {
        unsigned int compareIndex = orderedIndices[i];
        double compare = numbers[compareIndex];
        int j = i - 1;
        while(j >= 0 && compare < numbers[orderedIndices[j]])
        {
            orderedIndices[j + 1] = orderedIndices[j];
            --j;
        }
        orderedIndices[j + 1] = compareIndex;
    }
}

inline void mjSort(std::vector<double>& numbers, std::vector<unsigned int>& orderedIndices, unsigned int count)
{
    quickSort(numbers, orderedIndices, 0, count - 1);
}

inline glm::mat4 makeInfReversedZProjRH(float fovY_radians, float aspectWbyH, float zNear)
{
    float f = 1.0f / tan(fovY_radians / 2.0f);
    return glm::mat4(
        f / aspectWbyH, 0.0f,  0.0f,  0.0f,
        0.0f,    f,  0.0f,  0.0f,
        0.0f, 0.0f,  0.0f, -1.0f,
        0.0f, 0.0f, zNear,  0.0f);
}

inline glm::mat4 makeProjectionMat(const float left, const float right,
	const float top, const float bottom, const float zNear)
{
	return glm::mat4(
		2.0f/(right - left), 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f/(bottom - top), 0.0f, 0.0f,
		(right + left)/(right - left), (bottom + top)/(bottom - top), 0.0f, -1.0f,
		0.0f, 0.0f, zNear, 0.0f
	);
}

inline bool intersectRayOOB(const dvec3& ray_origin,
	const dvec3& ray_direction,
	const dvec3& aabb_min,          // Minimum X,Y,Z coords of the mesh when not transformed at all.
	const dvec3& aabb_max,          // Maximum X,Y,Z coords. Often aabb_min*-1 if your mesh is centered, but it's not always the case.
	const dmat4& ModelMatrix,       // Transformation applied to the mesh (which will thus be also applied to its bounding box)
	double& intersection_distance,
	dvec3* normal)
{
	double tMin = 0.0;
	double tMax = FLT_MAX;

	dvec3 OBBposition_worldspace(ModelMatrix[3].x, ModelMatrix[3].y, ModelMatrix[3].z);

	dvec3 delta = OBBposition_worldspace - ray_origin;

	// Test intersection with the 2 planes perpendicular to the OBB's X axis
	{
		dvec3 xaxis(ModelMatrix[0].x, ModelMatrix[0].y, ModelMatrix[0].z);
		double e = dot(xaxis, delta);
		double f = dot(ray_direction, xaxis);

		if ( fabs(f) > 0.000001 ){ // Standard case
			double t1 = (e+aabb_min.x)/f; // Intersection with the "left" plane
			double t2 = (e+aabb_max.x)/f; // Intersection with the "right" plane
			bool flipped = false;
										 // t1 and t2 now contain distances betwen ray origin and ray-plane intersections

										 // We want t1 to represent the nearest intersection, 
										 // so if it's not the case, invert t1 and t2
			if (t1>t2){
				double w=t1;t1=t2;t2=w; // swap t1 and t2
				flipped = true;
			}

			// tMax is the nearest "far" intersection (amongst the X,Y and Z planes pairs)
			if ( t2 < tMax )
				tMax = t2;
			// tMin is the farthest "near" intersection (amongst the X,Y and Z planes pairs)
			if ( t1 > tMin )
			{
				tMin = t1;
				if(flipped)
				{
					*normal = dvec3(-1,0,0);
				}
				else
				{
					*normal = dvec3(1,0,0);
				}
			}

			// And here's the trick :
			// If "far" is closer than "near", then there is NO intersection.
			// See the images in the tutorials for the visual explanation.
			if (tMax < tMin )
				return false;

		}else{ // Rare case : the ray is almost parallel to the planes, so they don't have any "intersection"
			if(-e+aabb_min.x > 0.0 || -e+aabb_max.x < 0.0)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Y axis
	// Exactly the same thing than above.
	{
		dvec3 yaxis(ModelMatrix[1].x, ModelMatrix[1].y, ModelMatrix[1].z);
		double e = dot(yaxis, delta);
		double f = dot(ray_direction, yaxis);

		if ( fabs(f) > 0.000001 ){

			double t1 = (e+aabb_min.y)/f;
			double t2 = (e+aabb_max.y)/f;
			bool flipped = false;

			if (t1>t2){
				float w=t1;t1=t2;t2=w;
				flipped = true;
			}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
			{
				tMin = t1;
				if(flipped)
				{
					*normal = dvec3(0,-1,0);
				}
				else
				{
					*normal = dvec3(0,1,0);
				}
			}
			if (tMin > tMax)
				return false;

		}else{
			if(-e+aabb_min.y > 0.0 || -e+aabb_max.y < 0.0)
				return false;
		}
	}


	// Test intersection with the 2 planes perpendicular to the OBB's Z axis
	// Exactly the same thing than above.
	{
		dvec3 zaxis(ModelMatrix[2].x, ModelMatrix[2].y, ModelMatrix[2].z);
		double e = dot(zaxis, delta);
		double f = dot(ray_direction, zaxis);

		if ( fabs(f) > 0.000001 ){

			double t1 = (e+aabb_min.z)/f;
			double t2 = (e+aabb_max.z)/f;
			bool flipped = false;

			if (t1>t2){
				float w=t1;t1=t2;t2=w;
				flipped = true;
			}

			if ( t2 < tMax )
				tMax = t2;
			if ( t1 > tMin )
			{
				tMin = t1;
				if(flipped)
				{
					*normal = dvec3(0,-0,-1);
				}
				else
				{
					*normal = dvec3(0,0,1);
				}
			}
			if (tMin > tMax)
				return false;

		}else{
			if(-e+aabb_min.z > 0.0 || -e+aabb_max.z < 0.0)
				return false;
		}
	}

	intersection_distance = tMin;
	return true;
}

inline bool raySphereIntersectionDistance(const dvec3& rayOrigin, const dvec3& rayDir, const dvec3& sphereCenter, double sphereRadiusSquered, double* intersectionDistance)
{
	dvec3 diff = sphereCenter - rayOrigin;
	double t0 = diff.x * rayDir.x + diff.y * rayDir.y + diff.z * rayDir.z;
	double dSquared = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z - t0 * t0;
	if(dSquared > sphereRadiusSquered)
	{
		return false;
	}
	double t1 = sqrt(sphereRadiusSquered - dSquared);
	if(t0 > t1 + std::numeric_limits<double>::epsilon())
	{
		*intersectionDistance = t0 - t1;
	}
	else
	{
		*intersectionDistance = t0 + t1;
	}

	if(*intersectionDistance > std::numeric_limits<double>::epsilon())
	{
		return true;
	}

	return false;
}

inline dvec3 randomPointWithinTriangle(const dvec3& a, const dvec3& b, const dvec3& c, double r1, double r2)
{
	dvec3 ba = b - a;
	dvec3 ca = c - a;
	dvec3 p = a + ba * r1 + ca * r2;
	if(pointIsLeftOfLine(p, b, c))
	{
		dvec3 bcMidPoint = b + (c-b) * 0.5;
		return bcMidPoint - (p - bcMidPoint);
	}
	return p;
}

inline dvec3 perpendicularUnitVec(const dvec3& a)
{
	if(approxEqual(fabs(a.y), 1.0))
	{
		return normalize(cross(a, dvec3(1.0,0.0,0.0)));
	}
	return normalize(cross(a, dvec3(0.0,1.0,0.0)));
}



inline uint32_t prevPow2(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v / 2;
}

#endif /* defined(__World__MathUtils__)  */
 //0
