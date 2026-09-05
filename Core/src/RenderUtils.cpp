/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#include "RenderUtils.h"

vec4 WHITE_COLOR(1.f, 1.f, 1.f, 1.f);
static const vec3 vec3_NEG_X(-1.f, 0.f, 0.f);
static const vec3 vec3_POS_X(1.f, 0.f, 0.f);
static const vec3 vec3_NEG_Y(0.f, -1.f, 0.f);
static const vec3 vec3_POS_Y(0.f, 1.f, 0.f);
static const vec3 vec3_NEG_Z(0.f, 0.f, -1.f);
static const vec3 vec3_POS_Z(0.f, 0.f, 1.f);
#define	DELETE_BUFFER(a)		if ((a) && m_gl->glIsBuffer(a)) { m_gl->glDeleteBuffers(1, &(a)); (a) = 0; }

static void createBox(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices)
{
	const auto i0 = (uint16_t)vertices.size();

	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, 1.f), vec3_NEG_X });
	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, 1.f), vec3_NEG_X });
	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, -1.f), vec3_NEG_X });
	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, -1.f), vec3_NEG_X });

	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, 1.f), vec3_POS_X });
	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, 1.f), vec3_POS_X });
	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, -1.f), vec3_POS_X });
	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, -1.f), vec3_POS_X });

	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, 1.f), vec3_NEG_Y });
	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, 1.f), vec3_NEG_Y });
	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, -1.f), vec3_NEG_Y });
	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, -1.f), vec3_NEG_Y });

	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, 1.f), vec3_POS_Y });
	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, 1.f), vec3_POS_Y });
	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, -1.f), vec3_POS_Y });
	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, -1.f), vec3_POS_Y });

	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, -1.f), vec3_NEG_Z });
	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, -1.f), vec3_NEG_Z });
	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, -1.f), vec3_NEG_Z });
	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, -1.f), vec3_NEG_Z });

	vertices.emplace_back(Vertex{ vec3(1.f, -1.f, 1.f), vec3_POS_Z });
	vertices.emplace_back(Vertex{ vec3(-1.f, -1.f, 1.f), vec3_POS_Z });
	vertices.emplace_back(Vertex{ vec3(-1.f, 1.f, 1.f), vec3_POS_Z });
	vertices.emplace_back(Vertex{ vec3(1.f, 1.f, 1.f), vec3_POS_Z });

	for (uint16_t i = i0; i < i0 + 24; i += 4) {
		indices.emplace_back(i);
		indices.emplace_back(i + 1);
		indices.emplace_back(i + 2);

		indices.emplace_back(i);
		indices.emplace_back(i + 2);
		indices.emplace_back(i + 3);
	}
}

static uint32_t VertexKey(uint32_t a, uint32_t b)
{
	return a < b ? ((a << 16) | b) : ((b << 16) | a);
}

static void addVertex(std::vector<Vertex> & vertices, const vec3 & dir)
{
	vertices.emplace_back(Vertex{ dir, dir });
};

static void AddSubDivSphereTriangle(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, uint32_t a, uint32_t b, uint32_t c, std::map<uint32_t, uint32_t> & vm, int level)
{
	if (level == 0) {
		indices.push_back(a);
		indices.push_back(b);
		indices.push_back(c);
	} else {
		const uint32_t keyAB = VertexKey(a, b);
		const uint32_t keyBC = VertexKey(b, c);
		const uint32_t keyCA = VertexKey(c, a);
		if (!vm.count(keyAB)) {
			vm[keyAB] = static_cast<uint16_t>(vertices.size());
			addVertex(vertices, glm::normalize(vertices[a].pos + vertices[b].pos));
		}
		if (!vm.count(keyBC)) {
			vm[keyBC] = static_cast<uint16_t>(vertices.size());
			addVertex(vertices, glm::normalize(vertices[b].pos + vertices[c].pos));
		}
		if (!vm.count(keyCA)) {
			vm[keyCA] = static_cast<uint16_t>(vertices.size());
			addVertex(vertices, glm::normalize(vertices[c].pos + vertices[a].pos));
		}
		const uint32_t ab = vm[keyAB];
		const uint32_t bc = vm[keyBC];
		const uint32_t ca = vm[keyCA];
		level--;
		AddSubDivSphereTriangle(vertices, indices, a, ab, ca, vm, level);
		AddSubDivSphereTriangle(vertices, indices, b, bc, ab, vm, level);
		AddSubDivSphereTriangle(vertices, indices, c, ca, bc, vm, level);
		AddSubDivSphereTriangle(vertices, indices, ab, bc, ca, vm, level);
	}
}

static void createGeosphere(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, int level)
{
	const int MAX_VERTICES = 12 + 10 * ((1 << (level * 2)) - 1);
	const int MAX_INDICES = 20 * 3 * (1 << (level * 2));

	vertices.reserve(MAX_VERTICES);
	indices.reserve(MAX_INDICES);

	const float a = 2.f / (1.f + std::sqrt(5.f));
	const float b = 1.f / std::sqrt((3.f + std::sqrt(5.f)) / (1.f + std::sqrt(5.f)));
	addVertex(vertices, vec3( 0,  a,  b));
	addVertex(vertices, vec3( 0,  a, -b));
	addVertex(vertices, vec3( 0, -a,  b));
	addVertex(vertices, vec3( 0, -a, -b));
	addVertex(vertices, vec3( a,  b,  0));
	addVertex(vertices, vec3( a, -b,  0));
	addVertex(vertices, vec3(-a,  b,  0));
	addVertex(vertices, vec3(-a, -b,  0));
	addVertex(vertices, vec3( b,  0,  a));
	addVertex(vertices, vec3( b,  0, -a));
	addVertex(vertices, vec3(-b,  0,  a));
	addVertex(vertices, vec3(-b,  0, -a));

	const uint32_t topLevelIndices[] = {
		1, 4, 6,	0, 6, 4,
		0, 2, 10,	0, 8, 2,
		1, 3, 9,	1, 11, 3,
		2, 5, 7,	3, 7, 5,
		6, 10, 11,	7, 11, 10,
		4, 9, 8,	5, 8, 9,
		0, 10, 6,	0, 4, 8,
		1, 6, 11,	1, 9, 4,
		3, 11, 7,	3, 5, 9,
		2, 7, 10,	2, 8, 5,
	};

	std::map<uint32_t, uint32_t> vm;
	for (size_t i = 0; i < GLM_COUNTOF(topLevelIndices); i += 3) {
		AddSubDivSphereTriangle(vertices, indices, topLevelIndices[i], topLevelIndices[i+1], topLevelIndices[i+2], vm, level);
	}
}

static void createCylinder(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const vec3 & p1, const vec3 & p2, float radius, int segments)
{
	const auto dirZ = glm::normalize(p2 - p1);
	vec3 dirX = glm::perpendicular(dirZ);
	const auto dirY = glm::cross(dirZ, dirX);
	const float da = M_2PIf / segments;
	const auto i0 = (uint16_t)vertices.size();

	for (int i = 0; i < segments; i++) {
		const auto angle = i * da;
		const auto dir = dirX * std::cos(angle) + dirY * std::sin(angle);
		vertices.emplace_back(Vertex{ p1 + dir * radius, dir });
		vertices.emplace_back(Vertex{ p2 + dir * radius, dir });
	}

	for (int i = 0, j = segments - 1; i < segments; j = i++) {
		const uint16_t i1 = i0 + (j << 1), i2 = i0 + (i << 1);
		indices.emplace_back(i1);
		indices.emplace_back(i1 + 1);
		indices.emplace_back(i2);

		indices.emplace_back(i2);
		indices.emplace_back(i1 + 1);
		indices.emplace_back(i2 + 1);
	}

	const auto iCap1 = (uint16_t)vertices.size();
	for (int i = 0; i < segments; i++)
		vertices.emplace_back(Vertex{ vertices[i0 + (i << 1)].pos, -dirZ });

	const auto iCap2 = (uint16_t)vertices.size();
	for (int i = 0; i < segments; i++)
		vertices.emplace_back(Vertex{ vertices[i0 + 1 + (i << 1)].pos, dirZ });

	for (int i = 2; i < segments; i++) {
		indices.emplace_back(iCap1);
		indices.emplace_back(iCap1 + i - 1);
		indices.emplace_back(iCap1 + i);
	}

	for (int i = 2; i < segments; i++) {
		indices.emplace_back(iCap2);
		indices.emplace_back(iCap2 + i);
		indices.emplace_back(iCap2 + i - 1);
	}
}

static void createCone(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, const vec3 & p1, const vec3 & p2, float radius, int segments)
{
	auto dirZ = p2 - p1;
	const auto length = glm::length(dirZ);
	dirZ *= 1.f / length;
	const auto dirX = glm::perpendicular(dirZ);
	const auto dirY = glm::cross(dirZ, dirX);
	const auto da = M_2PIf / (float)segments;
	const auto g = std::sqrt(radius * radius + length * length);
	const auto f1 = radius / g;
	const auto f2 = length / g;

	const auto i0 = (uint16_t)vertices.size();

	for (int i = 0; i < segments; i++) {
		auto angle = i * da;
		auto dir = dirX * std::cos(angle) + dirY * std::sin(angle);
		const auto p = p1 + dir * radius;
		vertices.emplace_back(Vertex{ p, dirZ * f1 + dir * f2 });
		angle += da * 0.5f;
		dir = dirX * std::cos(angle) + dirY * std::sin(angle);
		vertices.emplace_back(Vertex{ p2, dirZ * f1 + dir * f2 });
	}

	for (int i = 0, j = segments - 1; i < segments; j = i++) {
		const uint16_t i1 = i0 + (j << 1);
		const uint16_t i2 = i0 + (i << 1);
		indices.emplace_back(i1);
		indices.emplace_back(i1 + 1);
		indices.emplace_back(i2);
	}

	const auto iCap = (uint16_t)vertices.size();
	for (int i = 0; i < segments; i++)
		vertices.emplace_back(Vertex{ vertices[i0 + (i << 1)].pos, -dirZ });

	for (int i = 2; i < segments; i++) {
		indices.emplace_back(iCap);
		indices.emplace_back(iCap + i - 1);
		indices.emplace_back(iCap + i);
	}
}

static void createTorus(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, float radius1, float radius2, int radialSegments, int tubularSegments)
{
	const auto i0 = (uint16_t)vertices.size();
	const auto dA1 = M_2PIf / radialSegments;
	const auto dA2 = M_2PIf / tubularSegments;
	const auto r2 = std::min(radius1, radius2);

	vertices.reserve(vertices.size() + (radialSegments + 1) * (tubularSegments + 1));
	for (int i = 0; i <= radialSegments; i++) {
		const auto radialAngle = i * dA1;
		const auto ca1 = std::cos(radialAngle), sa1 = std::sin(radialAngle);
		const vec3 pos(0.f, ca1 * radius1, sa1 * radius1);
		for (int j = 0; j <= tubularSegments; j++) {
			const auto tubularAngle = j * dA2;
			const auto ca2 = std::cos(tubularAngle), sa2 = std::sin(tubularAngle);
			const vec3 normal(sa2, ca1 * ca2, sa1 * ca2);
			vertices.emplace_back(Vertex{ pos + normal * r2, normal });
		}
	}

	indices.reserve(indices.size() + radialSegments * tubularSegments * 6);
	for (int i = 0; i < radialSegments; i++) {
		for (int j = 0; j < tubularSegments; j++) {
			const uint16_t vi = i0 + (uint16_t)(i * (tubularSegments + 1) + j);
			indices.emplace_back(vi);
			indices.emplace_back(vi + 1);
			indices.emplace_back(vi + (uint16_t)tubularSegments + 1);

			indices.emplace_back(vi + (uint16_t)tubularSegments + 1);
			indices.emplace_back(vi + 1);
			indices.emplace_back(vi + (uint16_t)tubularSegments + 2);
		}
	}
}

static void createCircle(std::vector<Vertex> & vertices, std::vector<uint16_t> & indices, float radius1, float radius2, int segments)
{
	const auto i0 = (uint16_t)vertices.size();
	for (int i = 0; i < segments; i++) {
		const auto a = i * (M_2PIf / segments);
		const auto ca = std::cos(a), sa = std::sin(a);
		vertices.emplace_back(Vertex{ vec3(0.f, radius1 * ca, radius1 * sa), vec3_NEG_X });
		vertices.emplace_back(Vertex{ vec3(0.f, radius2 * ca, radius2 * sa), vec3_NEG_X });
	}

	for (int i = 0, j = segments - 1; i < segments; j = i++) {
		const uint16_t vi1 = i0 + (j << 1);
		const uint16_t vi2 = i0 + (i << 1);

		indices.emplace_back(vi1);
		indices.emplace_back(vi1 + 1);
		indices.emplace_back(vi2);

		indices.emplace_back(vi2);
		indices.emplace_back(vi1 + 1);
		indices.emplace_back(vi2 + 1);
	}
}

// ------------------------------------------------------------------------ //

DrawGL::DrawGL(QOpenGLContext *context) : QOpenGLExtraFunctions(context)
{
	assert(GL_NO_ERROR == this->glGetError());

	m_shaderColor.reset(new GLShader(this, "blend", "", { "aP" }, { "uMVPM", "uColor" }));
	m_shaderTexture.reset(new GLShader(this, "blend", "#define TEXTURE\n", { "aP" }, { "uMVPM", "uColor", "uTM", "uT" }));
	m_shaderTexture_sRGB.reset(new GLShader(this, "blend", "#define TEXTURE\n#define sRGB\n", { "aP" }, { "uMVPM", "uColor", "uTM", "uT" }));
	m_shaderObject.reset(new GLShader(this, "object", "", { "aP", "aN" }, { "uMVPM", "uColor", "uNM" }));
	m_shaderLine.reset(new GLShader(this, "line", "", { "aP" }, { "uMVPM", "uColor", "uDash" }));

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	createCylinder(vertices, indices, vec3(0.f), vec3(0.7f, 0.f, 0.f), 0.02f, 12);
	createCone(vertices, indices, vec3(0.7f, 0.f, 0.f), vec3(1.f, 0.f, 0.f), 0.04f, 24);
	m_arrow.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createCylinder(vertices, indices, vec3(0.f), vec3(1.f, 0.f, 0.f), 1.f, 12);
	m_cylinder.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createBox(vertices, indices);
	m_box.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createGeosphere(vertices, indices, 3);
	m_geosphere.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createTorus(vertices, indices, 1.f, 0.01f, 64, 6);
	m_torus1.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createTorus(vertices, indices, 0.9f, 0.01f, 64, 6);
	m_torus2.reset(new GLObject(this, vertices, indices));

	vertices.clear();
	indices.clear();
	createCircle(vertices, indices, 1.f, 0.9f, 64);
	m_circle.reset(new GLObject(this, vertices, indices));

	m_line.reset(new GLObject(this, {
		Vertex{ vec3(0.f, -1.f, 0.f), vec3(0.f) },
		Vertex{ vec3(0.f, 1.f, 0.f), vec3(0.f) },
		Vertex{ vec3(1.f, -1.f, 0.f), vec3(1.f) },
		Vertex{ vec3(1.f, 1.f, 0.f), vec3(1.f) }
	}, { 0, 2, 1, 1, 2, 3 }));

	m_quad.reset(new GLObject(this, {
		Vertex{ vec3(-1.f, +1.f, 1.f), vec3_NEG_Z },
		Vertex{ vec3(+1.f, +1.f, 1.f), vec3_NEG_Z },
		Vertex{ vec3(+1.f, -1.f, 1.f), vec3_NEG_Z },
		Vertex{ vec3(-1.f, -1.f, 1.f), vec3_NEG_Z },
	}, { 0, 1, 2, 0, 2, 3}));

	assert(GL_NO_ERROR == this->glGetError());
}

void DrawGL::setViewportSize(int width, int height)
{
	m_size = vec2((float)width, (float)height);
	m_invSize = vec2(1.f / m_size.x, 1.f / m_size.y);
}

void DrawGL::setCameraInfo(const mat4 & matView, const mat4 & matProj, float zNear)
{
	m_matView = matView;
	m_matProj = matProj;
	m_matViewProj = matProj * matView;
	m_zNear = zNear;
}

static mat4 GetMatrix(const vec3 & pos, const vec3 & dir, float length)
{
	auto mat = glm::translate(glm::mat4(1.f), pos);
	glm::setAxisX(mat, dir * length);
	glm::setAxisY(mat, glm::perpendicular(dir) * length);
	glm::setAxisZ(mat, glm::cross(dir, glm::axisY(mat)));
	return mat;
}

void DrawGL::drawArrow(const vec3 & pos, const vec3 & dir, float length, const vec4 & color)
{
	m_arrow->draw(GetMatrix(pos, dir, length), m_matViewProj, *m_shaderObject, color);
}

void DrawGL::drawCircle(const vec3 & pos, const vec3 & dir, float size, const vec4 & color1, const vec4 & color2)
{
	const auto matModel = GetMatrix(pos, dir, size);
	m_torus1->draw(matModel, m_matViewProj, *m_shaderObject, color1);
	m_torus2->draw(matModel, m_matViewProj, *m_shaderObject, color1);
	this->glDisable(GL_CULL_FACE);
	m_circle->draw(matModel, m_matViewProj, *m_shaderColor, color2);
	this->glEnable(GL_CULL_FACE);
}

void DrawGL::drawCylinder(const vec3 & pos, const vec3 & dir, float length, float radius, const vec4 & color)
{
	auto mat = GetMatrix(pos, dir, length);
	glm::setAxisY(mat, glm::axisY(mat) * (radius / length));
	glm::setAxisZ(mat, glm::axisZ(mat) * (radius / length));
	m_cylinder->draw(mat, m_matViewProj, *m_shaderObject, color);
}

void DrawGL::drawBox(const BBox & box, const mat4 & matModel, const vec4 & c)
{
	auto mat = matModel;
	glm::setTranslation(mat, glm::transformCoord(box.center(), matModel));
	const auto size = box.size() * 0.5f;
	glm::setAxisX(mat, glm::axisX(mat) * size.x);
	glm::setAxisY(mat, glm::axisY(mat) * size.y);
	glm::setAxisZ(mat, glm::axisZ(mat) * size.z);
	m_box->draw(mat, m_matViewProj, *m_shaderObject, c);
}

void DrawGL::drawGeosphere(const vec3 & pos, float radius, const vec4 & color)
{
	m_geosphere->draw(MatrixBuilder().AddPosition(pos).AddScale(vec3(radius)), m_matViewProj, *m_shaderObject, color);
}

void DrawGL::drawLine(const vec3 & pos1, const vec3 & pos2, const vec4 & color, float lineWidth, float dashSize, float dashOffset)
{
	vec4 v1(pos1, 1.f);
	vec4 v2(pos2, 1.f);
	auto clipPlane = vec4(m_matView[0][2], m_matView[1][2], m_matView[2][2], m_matView[3][2] + m_zNear);
	auto d1 = glm::dot(v1, clipPlane);
	auto d2 = glm::dot(v2, clipPlane);
	if ((d1 < 0.f) != (d2 < 0.f)) {
		auto clipPos = glm::mix(pos1, pos2, d1 / (d1 - d2));
		*reinterpret_cast<vec3 *>(d1 < 0.f ? &v2 : &v1) = clipPos;
	} else if (d1 > 0)
		return; // invisible
	v1 = m_matViewProj * v1;
	v2 = m_matViewProj * v2;
	const auto pos = reinterpret_cast<vec3 &>(v1) / v1.w;
	const auto dirX = (reinterpret_cast<vec3 &>(v2) / v2.w) - pos;
	vec2 dirY(dirX.y * m_size.y, -dirX.x * m_size.x);
	float length = glm::length(dirY);
	dirY *= 1.f / length; // normalize
	dirY *= m_invSize * lineWidth;
	mat4 mat(dirX.x, dirX.y, dirX.z, 0.f,
			 dirY.x, dirY.y, 0.f, 0.f,
			 0.f, 0.f, 0.f, 0.f,
			 pos.x, pos.y, pos.z, 1.f);

	this->glUseProgram(m_shaderLine->program());

	this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_line->indexBuffer());
	this->glBindBuffer(GL_ARRAY_BUFFER, m_line->vertexBuffer());
	this->glVertexAttribPointer(m_shaderLine->attributes()[0], 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
	this->glEnableVertexAttribArray(m_shaderLine->attributes()[0]);

	this->glUniformMatrix4fv(m_shaderLine->uniforms()[0], 1, false, glm::value_ptr(mat));
	this->glUniform4fv(m_shaderLine->uniforms()[1], 1, glm::value_ptr(color));
	this->glUniform2f(m_shaderLine->uniforms()[2], dashSize > 0.f ? length * 0.25f / dashSize : 0.f, dashOffset * 0.5f);

	this->glDrawElements(GL_TRIANGLES, m_line->indexCount(), GL_UNSIGNED_SHORT, 0);

	this->glUseProgram(0);
	this->glBindBuffer(GL_ARRAY_BUFFER, 0);
	this->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void DrawGL::drawWireframeBox(const BBox & box, const mat4 & matModel, const vec4 & c, float lineWidth, float dashSize, float dashOffset)
{
	vec3 verts[8];
	box.getVerts(verts);
	for (auto & v : verts)
		v = glm::transformCoord(v, matModel);

	drawLine(verts[0], verts[1], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[2], verts[3], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[0], verts[2], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[1], verts[3], c, lineWidth, dashSize, dashOffset);

	drawLine(verts[4], verts[5], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[6], verts[7], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[4], verts[6], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[5], verts[7], c, lineWidth, dashSize, dashOffset);

	drawLine(verts[0], verts[4], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[1], verts[5], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[2], verts[6], c, lineWidth, dashSize, dashOffset);
	drawLine(verts[3], verts[7], c, lineWidth, dashSize, dashOffset);
}

void DrawGL::drawGrid()
{
	vec4 c1(0.75f, 0.75f, 0.75f, 1.f);
	vec4 c2(0.5f, 0.5f, 0.5f, 1.f);
	for (int x = -9; x < 10; x++) {
		if (x == 0) continue;

		drawLine(vec3((float)x, -10.f, 0.f), vec3((float)x, 10.f, 0.f), c1);
		drawLine(vec3(-10.f, (float)x, 0.f), vec3(10.f, (float)x, 0.f), c1);

		drawLine(vec3(x * 10.f, -100.f, 0.f), vec3(x * 10.f, 100.f, 0.f), c2);
		drawLine(vec3(-100.f, x * 10.f, 0.f), vec3(100.f, x * 10.f, 0.f), c2);
	}

	drawLine(vec3(100.f, 100.f, 0.f), vec3(100.f, -100.f, 0.f), c2);
	drawLine(vec3(100.f, 100.f, 0.f), vec3(-100.f, 100.f, 0.f), c2);
	drawLine(vec3(-100.f, -100.f, 0.f), vec3(100.f, -100.f, 0.f), c2);
	drawLine(vec3(-100.f, -100.f, 0.f), vec3(-100.f, 100.f, 0.f), c2);

	drawLine(vec3(0.f), vec3(100.f, 0.f, 0.f), vec4(1.f, 0.f, 0.f, 1.f));
	drawLine(vec3(0.f), vec3(0.f, 100.f, 0.f), vec4(0.f, 1.f, 0.f, 1.f));
	drawLine(vec3(0.f), vec3(0.f, 0.f, 100.f), vec4(0.f, 0.f, 1.f, 1.f));
	drawLine(vec3(0.f), vec3(-100.f, 0.f, 0.f), c2);
	drawLine(vec3(0.f), vec3(0.f, -100.f, 0.f), c2);
	drawLine(vec3(0.f), vec3(0.f, 0.f, -100.f), c2);
}

void DrawGL::drawTexture(const mat4 & matModelViewProj, GLuint texture, const vec4 & color, const vec2 & uvScale, bool sRGB)
{
	assert(GL_NO_ERROR == this->glGetError());
	const auto & s = sRGB ? *m_shaderTexture_sRGB : *m_shaderTexture;
	this->glUseProgram(s.program());

	this->glBindBuffer(GL_ARRAY_BUFFER, m_quad->vertexBuffer());
	this->glVertexAttribPointer(s.attributes()[0], 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
	this->glEnableVertexAttribArray(s.attributes()[0]);

	this->glUniformMatrix4fv(s.uniforms()[0], 1, false, glm::value_ptr(matModelViewProj));
	this->glUniform4fv(s.uniforms()[1], 1, glm::value_ptr(color));
	this->glUniform4f(s.uniforms()[2], 0.5f * uvScale.x, -0.5f * uvScale.y, 0.5f * uvScale.x, 0.5f * uvScale.y);

	this->glUniform1i(s.uniforms()[3], 0);
	this->glActiveTexture(GL_TEXTURE0);
	this->glBindTexture(GL_TEXTURE_2D, texture);

	this->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	assert(GL_NO_ERROR == this->glGetError());

	this->glBindTexture(GL_TEXTURE_2D, 0);
	this->glUseProgram(0);
	this->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawGL::drawSprite(const vec2 & pos, const vec2 & size, float angle, GLuint texture, const vec4 & color)
{
	const auto vpSize = size * m_invSize;
	const auto dir = angleToDir(angle);
	drawTexture(mat4(dir.x * vpSize.x, dir.y * vpSize.y, 0.f, 0.f,
					 -dir.y * vpSize.x, dir.x * vpSize.y, 0.f, 0.f,
					 0.f, 0.f, 1.f, 0.f,
					 pos.x, pos.y, 0.f, 1.f), texture, color);
}

void DrawGL::drawRect(const vec2 & p1, const vec2 & p2, const vec4 & color)
{
	const auto & s = *m_shaderColor;
	this->glUseProgram(s.program());

	this->glBindBuffer(GL_ARRAY_BUFFER, m_quad->vertexBuffer());
	this->glVertexAttribPointer(s.attributes()[0], 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
	this->glEnableVertexAttribArray(s.attributes()[0]);

	this->glUniform4fv(s.uniforms()[1], 1, glm::value_ptr(color));

	mat4 matModelViewProj((p2.x - p1.x) * 0.5f, 0.f, 0.f, 0.f,
						  0.f, (p2.y - p1.y) * 0.5f, 0.f, 0.f,
						  0.f, 0.f, 0.f, 0.f,
						  (p2.x + p1.x) * 0.5f, (p2.y + p1.y) * 0.5f, -1.f, 1.f);
	this->glUniformMatrix4fv(s.uniforms()[0], 1, false, glm::value_ptr(matModelViewProj));

	this->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

	this->glUseProgram(0);
	this->glBindBuffer(GL_ARRAY_BUFFER, 0);

}

// ------------------------------------------------------------------------ //

bool getRayAxisIntersectionDelta(const vec3 & rayStart, const vec3 & rayEnd, const vec3 & axisPos, const vec3 & axisDir, float l, float t, vec3 & delta)
{
	auto rayDir = rayEnd - rayStart;
	auto normal = glm::cross(rayDir, axisDir);
	auto length = glm::length(normal);
	if (length == 0.f)
		return false;

	normal *= 1.f / length; // normalize

	auto deltaPos = rayStart - axisPos;
	if (t > 0.f) {
		float d = glm::dot(normal, deltaPos);
		if (std::fabs(d) > t)
			return false;
	}

	auto dir = glm::cross(normal, rayDir);
	auto dist = glm::dot(dir, deltaPos) / glm::dot(dir, axisDir);
	if (l > 0.f) {
		if (dist < 0.f || dist > l)
			return false;

		if (glm::length2(deltaPos - axisDir * dist) > glm::length2(rayDir))
			return false;
	}

	delta = axisDir * dist;
	return true;
}

bool getRayPlaneIntersectionDelta(const vec3 & rayStart, const vec3 & rayEnd, const vec3 & axisPos, const vec3 & axisDir, vec3 & delta)
{
	auto d1 = glm::dot(axisPos - rayStart, axisDir);
	auto d2 = glm::dot(axisPos - rayEnd, axisDir);
	if ((d1 > 0.f) ^ (d2 <= 0.f))
		return false;

	auto rayDir = rayEnd - rayStart;
	delta = rayStart + rayDir * (d1 / glm::dot(rayDir, axisDir)) - axisPos;
	return true;
}

float getRayBoxIntersection(const vec3 & rayStart, const vec3 & rayEnd, const BBox & box)
{
	float tmin = 0.f;
	float tmax = 1.f;
	for (int i = 0; i < 3; i++) {
		float delta = rayEnd[i] - rayStart[i];
		if (delta == 0.f)
			continue;

		float invDelta = 1.f / delta;
		float t1, t2;
		if (delta > 0.f) {
			t1 = (box.min[i] - rayStart[i]) * invDelta;
			t2 = (box.max[i] - rayStart[i]) * invDelta;
		} else {
			t1 = (box.max[i] - rayStart[i]) * invDelta;
			t2 = (box.min[i] - rayStart[i]) * invDelta;
		}
		if ((tmin > t2) || (t1 > tmax))
			return 1.f;

		tmin = std::max(tmin, t1);
		tmax = std::min(tmax, t2);
	}

	return tmin;
}

// ------------------------------------------------------------------------ //

GLuint createVertexBuffer(QOpenGLFunctions * gl, size_t count, const Vertex * data)
{
	assert(GL_NO_ERROR == gl->glGetError());
	GLuint vertexBuffer;
	gl->glGenBuffers(1, &vertexBuffer);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	gl->glBufferData(GL_ARRAY_BUFFER, count * sizeof(Vertex), data, GL_STATIC_DRAW);
	assert(GL_NO_ERROR == gl->glGetError());
	gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
	return vertexBuffer;
}

GLuint createIndexBuffer(QOpenGLFunctions * gl, size_t count, const uint16_t * data)
{
	assert(GL_NO_ERROR == gl->glGetError());
	GLuint indexBuffer;
	gl->glGenBuffers(1, &indexBuffer);
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
	gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint16_t), data, GL_STATIC_DRAW);
	assert(GL_NO_ERROR == gl->glGetError());
	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return indexBuffer;
}

GLuint createProgram(QOpenGLFunctions * gl, const QString & name, const std::string & definitions)
{
	assert(GL_NO_ERROR == gl->glGetError());

	GLint status;
	GLchar infoLog[4096] = { 0 };

	QByteArray vertexShaderSource = definitions.c_str();
	QByteArray fragmentShaderSource = QOpenGLContext::currentContext()->isOpenGLES() ? ("precision mediump float;\n" + definitions).c_str() : definitions.c_str();

	{
		QFile fileVertex(":/" + name + ".vs");
		if (fileVertex.open(QIODevice::ReadOnly))
			vertexShaderSource += fileVertex.readAll();

		QFile fileFragment(":/" + name + ".fs");
		if (fileFragment.open(QIODevice::ReadOnly))
			fragmentShaderSource += fileFragment.readAll();
	}

	GLuint vertexShader = gl->glCreateShader(GL_VERTEX_SHADER);
	const char * vertexShaderStrings[1] = { vertexShaderSource.data() };
	GLint vertexShaderLengths[1] = { vertexShaderSource.length() };

	gl->glShaderSource(vertexShader, 1, vertexShaderStrings, vertexShaderLengths);
	gl->glCompileShader(vertexShader);
	gl->glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(vertexShader, GLM_COUNTOF(infoLog), nullptr, infoLog);
		LogError() << ("Error in " + name + " vertex shader compilation:\n" + infoLog);
		gl->glDeleteShader(vertexShader);
		return 0;
	}

	GLuint fragmentShader = gl->glCreateShader(GL_FRAGMENT_SHADER);
	const char * fragmentShaderStrings[1] = { fragmentShaderSource.data() };
	GLint fragmentShaderLengths[1] = { fragmentShaderSource.length() };
	gl->glShaderSource(fragmentShader, 1, fragmentShaderStrings, fragmentShaderLengths);
	gl->glCompileShader(fragmentShader);
	gl->glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		gl->glGetShaderInfoLog(fragmentShader, GLM_COUNTOF(infoLog), nullptr, infoLog);
		LogError() << ("Error in " + name + " fragment shader compilation:\n" + infoLog);
		gl->glDeleteShader(fragmentShader);
		gl->glDeleteShader(vertexShader);
		return 0;
	}

	GLuint program = gl->glCreateProgram();

	gl->glAttachShader(program, vertexShader);
	gl->glDeleteShader(vertexShader);

	gl->glAttachShader(program, fragmentShader);
	gl->glDeleteShader(fragmentShader);

	gl->glLinkProgram(program);
	gl->glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(program, GLM_COUNTOF(infoLog), nullptr, infoLog);
		LogError() << ("Error in " + name + " program link:\n" + infoLog);
		gl->glDeleteProgram(program);
		program = 0;
	}

	assert(GL_NO_ERROR == gl->glGetError());

	return program;
}

GLShader::GLShader(QOpenGLFunctions * gl, const QString & name, const std::string & definitions,
		std::initializer_list<const char *> attributes, std::initializer_list<const char *> uniforms)
	: m_gl(gl)
	, m_program(createProgram(gl, name, definitions))
{
	assert(GL_NO_ERROR == gl->glGetError());

	m_attributes.reserve(attributes.size());
	for (auto * attribute : attributes)
		m_attributes.emplace_back(gl->glGetAttribLocation(m_program, attribute));

	assert(GL_NO_ERROR == gl->glGetError());

	m_uniforms.reserve(uniforms.size());
	for (auto * uniform : uniforms)
		m_uniforms.emplace_back(gl->glGetUniformLocation(m_program, uniform));

	assert(GL_NO_ERROR == gl->glGetError());
}

GLShader::~GLShader()
{
	if (m_program && m_gl->glIsProgram(m_program)) { m_gl->glDeleteProgram(m_program); }
}

// ------------------------------------------------------------------------ //

GLObject::GLObject(QOpenGLFunctions * gl, const std::vector<Vertex> & vertices, const std::vector<uint16_t> & indices)
	: m_gl(gl)
	, m_vertexBuffer(createVertexBuffer(gl, vertices.size(), vertices.data()))
	, m_indexBuffer(createIndexBuffer(gl, indices.size(), indices.data()))
	, m_indexCount((GLsizei)indices.size())
{
}

GLObject::~GLObject()
{
	assert(GL_NO_ERROR == m_gl->glGetError());
	DELETE_BUFFER(m_vertexBuffer);
	DELETE_BUFFER(m_indexBuffer);
	assert(GL_NO_ERROR == m_gl->glGetError());
}

void GLObject::draw(const mat4 & matModel, const mat4 & matViewProj, const GLShader & shader, const vec4 & color) const
{
	m_gl->glUseProgram(shader.program());

	m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	m_gl->glVertexAttribPointer(shader.attributes()[0], 3, GL_FLOAT, false, sizeof(Vertex), (void*)0);
	m_gl->glEnableVertexAttribArray(shader.attributes()[0]);
	if (shader.attributes().size() > 1) {
		m_gl->glVertexAttribPointer(shader.attributes()[1], 3, GL_FLOAT, false, sizeof(Vertex), (void*)12);
		m_gl->glEnableVertexAttribArray(shader.attributes()[1]);
	}

	m_gl->glUniformMatrix4fv(shader.uniforms()[0], 1, false, glm::value_ptr(matViewProj * matModel));
	m_gl->glUniform4fv(shader.uniforms()[1], 1, glm::value_ptr(color));
	if (shader.uniforms().size() > 2) {
		const auto matWorldInv = glm::inverse(matModel);
		m_gl->glUniformMatrix4fv(shader.uniforms()[2], 1, false, glm::value_ptr(matWorldInv));
	}

	m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_SHORT, 0);

	m_gl->glUseProgram(0);
	m_gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

// ------------------------------------------------------------------------ //

GLuint createTexture2D(QOpenGLFunctions * gl, int width, int height, const void *data, GLint wrap, GLint filter, GLenum type)
{
	assert(wrap == GL_CLAMP_TO_EDGE || wrap == GL_REPEAT);
	assert(filter == GL_LINEAR || filter == GL_NEAREST || filter == GL_LINEAR_MIPMAP_LINEAR);
	assert(type == GL_UNSIGNED_BYTE || type == GL_FLOAT);

//	if (filter == GL_LINEAR_MIPMAP_LINEAR && !glGenerateMipmap)
//		filter = GL_LINEAR;

	GLuint texture;
	gl->glGenTextures(1, &texture);
	gl->glBindTexture(GL_TEXTURE_2D, texture);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter == GL_LINEAR_MIPMAP_LINEAR ? GL_LINEAR : filter);
	assert(GL_NO_ERROR == gl->glGetError());

	gl->glTexImage2D(GL_TEXTURE_2D, 0, type == GL_FLOAT ? GL_RGBA32F : GL_RGBA, width, height, 0, GL_RGBA, type, data);
	assert(GL_NO_ERROR == gl->glGetError());

//	if (filter == GL_LINEAR_MIPMAP_LINEAR)
//	{
//		float anisotropy;
//		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
//		glGenerateMipmap(GL_TEXTURE_2D);
//	}

	return texture;
}

GLuint createTexture2D(QOpenGLFunctions * gl, const ImagePtr image, GLint wrap, GLint filter)
{
	assert(image->dataType() == eImageDataType::Byte);
	assert(image->format() == eImageFormat::RGBA);

	const auto count = image->width() * image->height();
	std::vector<uint32_t> dataRGBA(count);
	const auto * srcBGRA = (const uint32_t *)image->rawData();
	std::transform(srcBGRA, srcBGRA + count, dataRGBA.begin(), [](uint32_t c) { return (c & 0xFF00FF00) | ((c >> 16) & 0xFF) | ((c & 0xFF) << 16); });
	return createTexture2D(gl, image->width(), image->height(), dataRGBA.data(), wrap, filter);
}
