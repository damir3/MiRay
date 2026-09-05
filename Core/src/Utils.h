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

#include "../../Shared/Interfaces/Scene.h"
#include "../../Shared/Interfaces/Image.h"

#define DEFER_CONCAT_IMPL(x, y) x##y
#define DEFER_CONCAT(x, y) DEFER_CONCAT_IMPL(x, y)

template <typename F>
struct Defer {
	F f;
	Defer(F f) : f(f) {}
	~Defer() { f(); }
};

#define DEFER(...) Defer DEFER_CONCAT(defer_var_, __COUNTER__){[&](){ __VA_ARGS__; }}

void saveStringParam(QDomDocument & doc, QDomElement & node, const char * name, const QString & value);
bool loadStringParam(QString & value, const QDomNode & node, const char * name);

void saveStringParam(QDomDocument & doc, QDomElement & node, const char * name, int value);
bool loadIntParam(int & value, const QDomNode & node, const char * name);

void saveFloatParam(QDomDocument & doc, QDomElement & node, const char * name, float value);
bool loadFloatParam(float & value, const QDomNode & node, const char * name, float scale);

void saveBoolParam(QDomDocument & doc, QDomElement & node, const char * name, bool value);
bool loadBoolParam(bool & value, const QDomNode & node, const char * name);

void saveVec2Param(QDomDocument & doc, QDomElement & node, const char * name, const vec2 & v);
void loadVec2Param(vec2 & v, const QDomNode & node, const char * name);

vec3 vec3FromString(QString str, const vec3 & def);
void saveVec3Param(QDomDocument & doc, QDomElement & node, const char * name, const vec3 & v);
void loadVec3Param(vec3 & v, const QDomNode & node, const char * name);

void saveVec4Param(QDomDocument & doc, QDomElement & node, const char * name, const vec4 & v);
void loadVec4Param(vec4 & v, const QDomNode & node, const char * name);

vec2 getVec2(const QJsonObject & obj, const QString & key, const vec2 & defaultValue);
vec3 getVec3(const QJsonObject & obj, const QString & key, const vec3 & defaultValue);
vec4 getVec4(const QJsonObject & obj, const QString & key, const vec4 & defaultValue);
void getArray(const QJsonObject & obj, const QString & key, float *p, int count);
void getArray(const QJsonObject & obj, const QString & key, int *p, int count);

QJsonArray toJsonArray(const vec2 & v);
QJsonArray toJsonArray(const vec3 & v);
QJsonArray toJsonArray(const vec4 & v);
template <typename T> QJsonArray toJsonArray(const T *p, size_t count)
{
	QJsonArray list;
	for (size_t i = 0; i < count; i++)
		list.append(p[i]);
	return list;
}

SceneModification sceneModificationAdd(ISceneElement *element, INode *targetNode, int position);
SceneModification sceneModificationDelete(ISceneElement *element, int pos);
SceneModification sceneModificationMove(ISceneElement *element, INode *targetNode, int position);
SceneModification sceneModificationVisibility(ISceneElement *element);
SceneModification sceneModificationMaterial(ISceneElement *element);

QMap<QString, QString> generateUniqueFileNamesForResourcesCollection(const QSet<QString> &filenames, const QSet<QString> &namesToAvoid);

bool checkIfImageHasTransparentPixels(const IImage *image);
bool checkIfImageIsNormalMap(const ImagePtr &image);

