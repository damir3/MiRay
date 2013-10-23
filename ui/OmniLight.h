//
//  OmniLight.h
//  ui
//
//  Created by Damir Sagidullin on 25.08.13.
//  Copyright (c) 2013 Damir Sagidullin. All rights reserved.
//

namespace mr
{

class OmniLight : public ISceneLight
{
	Vec3	m_origin;
	float	m_radius;
	Vec3	m_intensity;
	
public:
	void SetOrigin(const Vec3 & o) { m_origin = o; }
	void SetRadius(float r) { m_radius = r; }
	void SetIntensity(const Vec3 & i) { m_intensity = i; }
	
	const Vec3 & Origin() const { return m_origin; }
	float Radius() const { return m_radius; }
	const Vec3 & Intensity() const { return m_intensity; }
	
	Vec3 Position(const Vec3 & p) const;
	Vec3 Intensity(float squared_distance) const; // diffuse
	Vec3 Intensity(const Vec3 & rayPos, const Vec3 & rayDir) const; // specular
	bool TraceRay(const Vec3 & vFrom, const Vec3 & vTo, TraceResult & tr) const;

	bool Load(pugi::xml_node node);
	void Save(pugi::xml_node node) const;

	void Draw();
	
	BBoxF OOBB() const;
	Matrix Transformation() const;
	void SetTransformation(const Matrix & mat);
};

}