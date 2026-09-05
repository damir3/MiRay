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

#pragma once

#include <QOpenGLExtraFunctions>

extern vec4 WHITE_COLOR;

#define	DELETE_TEXTURE(a)		if ((a) && m_gl->glIsTexture(a)) { m_gl->glDeleteTextures(1, &(a)); (a) = 0; }

bool getRayAxisIntersectionDelta(const vec3 & rayStart, const vec3 & rayDir, const vec3 & axisPos, const vec3 & axisDir, float l, float t, vec3 & delta);
bool getRayPlaneIntersectionDelta(const vec3 & rayStart, const vec3 & rayEnd, const vec3 & axisPos, const vec3 & axisDir, vec3 & delta);
float getRayBoxIntersection(const vec3 & rayStart, const vec3 & rayEnd, const BBox & box);

template <typename T>
void fillTextureLineRGBA(T * dest, GLsizei n, float y, int imgWidth, int imgHeight, const T * imageData, int imgPitch)
{
	const int iy = std::clamp<int>(int(y), 0, imgHeight-1);
	const float t = std::clamp(y - float(iy), 0.f, 1.f);
	const T * srcY0 = imageData + (iy * imgPitch);
	const T * srcY1 = iy < imgHeight-1 ? (srcY0 + imgPitch) : srcY0;
	const float dx = float(imgWidth  - 1) / float(n - 1);
	float fx = 0.5 / float(n) - 0.5f / float(imgWidth);

	for (GLsizei i = 0; i < n; i++, fx += dx, dest += 4) {
		int ix = std::clamp<int>(int(fx), 0, imgWidth-1);
		float s = std::clamp(fx - float(ix), 0.f, 1.f);
		int ci1 = ix * 4;
		int ci2 = ix < imgWidth-1 ? ci1 + 4 : ci1;
		dest[0] = lerp2(srcY0[ci1+0], srcY0[ci2+0], srcY1[ci1+0], srcY1[ci2+0], s, t);
		dest[1] = lerp2(srcY0[ci1+1], srcY0[ci2+1], srcY1[ci1+1], srcY1[ci2+1], s, t);
		dest[2] = lerp2(srcY0[ci1+2], srcY0[ci2+2], srcY1[ci1+2], srcY1[ci2+2], s, t);
		dest[3] = lerp2(srcY0[ci1+3], srcY0[ci2+3], srcY1[ci1+3], srcY1[ci2+3], s, t);
	}
}

template <typename T>
void fillTextureLineRGB(T * dest, GLsizei n, float y, int imgWidth, int imgHeight, const T * imageData, int imgPitch, T alpha)
{
	const int iy = std::clamp<int>(int(y), 0, imgHeight-1);
	const float t = std::clamp(y - float(iy), 0.f, 1.f);
	const T * srcY0 = imageData + (iy * imgPitch);
	const T * srcY1 = iy < imgHeight-1 ? (srcY0 + imgPitch) : srcY0;
	const float dx = float(imgWidth  - 1) / float(n - 1);
	float fx = 0.5 / float(n) - 0.5f / float(imgWidth);

	for (GLsizei i = 0; i < n; i++, fx += dx, dest += 4) {
		int ix = std::clamp<int>(int(fx), 0, imgWidth-1);
		float s = std::clamp(fx - float(ix), 0.f, 1.f);
		int ci1 = ix * 3;
		int ci2 = ix < imgWidth-1 ? ci1 + 3 : ci1;
		dest[0] = lerp2(srcY0[ci1+0], srcY0[ci2+0], srcY1[ci1+0], srcY1[ci2+0], s, t);
		dest[1] = lerp2(srcY0[ci1+1], srcY0[ci2+1], srcY1[ci1+1], srcY1[ci2+1], s, t);
		dest[2] = lerp2(srcY0[ci1+2], srcY0[ci2+2], srcY1[ci1+2], srcY1[ci2+2], s, t);
		dest[3] = alpha;
	}
}

class GLShader
{
	QOpenGLFunctions * const m_gl;
	const GLuint		m_program;
	std::vector<GLint>	m_attributes;
	std::vector<GLint>	m_uniforms;

public:
	GLShader(QOpenGLFunctions * funcs, const QString & name, const std::string & definitions,
			std::initializer_list<const char *> attributes, std::initializer_list<const char *> uniforms);
	~GLShader();

	GLuint program() const { return m_program; }
	const std::vector<GLint> & attributes() const { return m_attributes; }
	const std::vector<GLint> & uniforms() const { return m_uniforms; }
};

class GLObject
{
	QOpenGLFunctions * const m_gl;
	GLuint		m_vertexBuffer;
	GLuint		m_indexBuffer;
	GLsizei		m_indexCount;

public:
	GLObject(QOpenGLFunctions * funcs, const std::vector<Vertex> & vertices, const std::vector<uint16_t> & indices);
	~GLObject();

	void draw(const mat4 & matModel, const mat4 & matViewProj, const GLShader & shader, const vec4 & color) const;

	GLuint vertexBuffer() const { return m_vertexBuffer; }
	GLuint indexBuffer() const { return m_indexBuffer; }
	GLsizei indexCount() const { return m_indexCount; }
};

class DrawGL : public QOpenGLExtraFunctions {
	std::unique_ptr<GLShader>	m_shaderColor;
	std::unique_ptr<GLShader>	m_shaderTexture;
	std::unique_ptr<GLShader>	m_shaderTexture_sRGB;
	std::unique_ptr<GLShader>	m_shaderObject;
	std::unique_ptr<GLShader>	m_shaderLine;
	std::unique_ptr<GLObject>	m_arrow;
	std::unique_ptr<GLObject>	m_cylinder;
	std::unique_ptr<GLObject>	m_box;
	std::unique_ptr<GLObject>	m_geosphere;
	std::unique_ptr<GLObject>	m_torus1;
	std::unique_ptr<GLObject>	m_torus2;
	std::unique_ptr<GLObject>	m_circle;
	std::unique_ptr<GLObject>	m_line;
	std::unique_ptr<GLObject>	m_quad;

	vec2	m_size; // viewport size
	vec2	m_invSize; // viewport size
	mat4	m_matView;
	mat4	m_matProj;
	mat4	m_matViewProj;
	float	m_zNear;

public:
	DrawGL(QOpenGLContext *context);

	void setViewportSize(int width, int height);
	void setCameraInfo(const mat4 & matView, const mat4 & matProj, float zNear);

	const vec2 & getViewportSize() const { return m_size; }
	const vec2 & getInvViewportSize() const { return m_invSize; }

	void drawArrow(const vec3 & pos, const vec3 & dir, float length, const vec4 & color);
	void drawCircle(const vec3 & pos, const vec3 & dir, float size, const vec4 & color1, const vec4 & color2);
	void drawCylinder(const vec3 & pos, const vec3 & dir, float length, float radius, const vec4 & color);
	void drawBox(const BBox & box, const mat4 & matModel, const vec4 & color);
	void drawGeosphere(const vec3 & pos, float radius, const vec4 & color);
	void drawLine(const vec3 & p1, const vec3 & p2, const vec4 & color, float lineWidth = 1.f, float dashSize = 0.f, float dashOffset = 0.f);
	void drawWireframeBox(const BBox & box, const mat4 & matModel, const vec4 & color, float lineWidth = 1.f, float dashSize = 0.f, float dashOffset = 0.f);
	void drawGrid();
	void drawTexture(const mat4 & matModelViewProj, GLuint texture, const vec4 & color, const vec2 & uvScale = vec2(1.f), bool sRGB = false);
	void drawSprite(const vec2 & pos, const vec2 & size, float angle, GLuint texture, const vec4 & color);
	void drawRect(const vec2 & p1, const vec2 & p2, const vec4 & color);
};

GLuint createTexture2D(QOpenGLFunctions * funcs, int width, int height, const void *data, GLint wrap, GLint filter, GLenum type = GL_UNSIGNED_BYTE);
GLuint createTexture2D(QOpenGLFunctions * funcs, const ImagePtr image, GLint wrap = GL_CLAMP_TO_EDGE, GLint filter = GL_LINEAR);
