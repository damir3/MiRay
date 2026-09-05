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

#include "Utils.h"

// ------------------------------------------------------------------------ //

void saveStringParam(QDomDocument & doc, QDomElement & node, const char * name, const QString & value)
{
	QDomElement elemParam = doc.createElement(name);
	auto textNode = doc.createTextNode(value);
	elemParam.appendChild(textNode);
	node.appendChild(elemParam);
}

bool loadStringParam(QString & value, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (elem.isNull())
		return false;

	value = elem.text();
	return true;
}

// ------------------------------------------------------------------------ //

void saveStringParam(QDomDocument & doc, QDomElement & node, const char * name, int value)
{
	saveStringParam(doc, node, name, QString().setNum(value));
}

bool loadIntParam(int & value, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (elem.isNull())
		return false;

	bool ok;
	int res = elem.text().toInt(&ok);
	if (ok) value = res;
	return ok;
}

// ------------------------------------------------------------------------ //

void saveFloatParam(QDomDocument & doc, QDomElement & node, const char * name, float value)
{
	saveStringParam(doc, node, name, QString().setNum(value));
}

bool loadFloatParam(float & value, const QDomNode & node, const char * name, float scale)
{
	QDomElement elem = node.firstChildElement(name);
	if (elem.isNull())
		return false;

	bool ok;
	float res = elem.text().toFloat(&ok);
	if (ok) value = res * scale;
	return ok;
}

// ------------------------------------------------------------------------ //

void saveBoolParam(QDomDocument & doc, QDomElement & node, const char * name, bool value)
{
	saveStringParam(doc, node, name, value ? "1" : "0");
}

bool loadBoolParam(bool & value, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (elem.isNull())
		return false;

	bool ok;
	int res = elem.text().toInt(&ok);
	if (ok) value = (res != 0);
	return ok;
}

// ------------------------------------------------------------------------ //

QString vec2ToString(const vec2 & v)
{
	return (v.x == v.y) ? QString().setNum(v.x) : QString().asprintf("%g %g", v.x, v.y);
}

vec2 vec2FromString(QString str, const vec2 & def)
{
	vec2 out;
	QTextStream stm(&str);
	stm >> out.x;
	if (stm.status() != QTextStream::Ok)
		return def;

	stm >> out.y;
	if (stm.status() != QTextStream::Ok)
		out.y = out.x;

	return out;
}

void saveVec2Param(QDomDocument & doc, QDomElement & node, const char * name, const vec2 & v)
{
	saveStringParam(doc, node, name, vec2ToString(v));
}

void loadVec2Param(vec2 & v, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (!elem.isNull())
		v = vec2FromString(elem.text(), v);
}

// ------------------------------------------------------------------------ //

QString vec3ToString(const vec3 & v)
{
	return (v.x == v.y && v.x == v.z) ? QString().setNum(v.x) : QString().asprintf("%g %g %g", v.x, v.y, v.z);
}

vec3 vec3FromString(QString str, const vec3 & def)
{
	vec3 out;
	QTextStream stm(&str);
	stm >> out.x;
	if (stm.status() != QTextStream::Ok)
		return def;

	stm >> out.y >> out.z;
	if (stm.status() != QTextStream::Ok)
		return vec3(out.x);

	return out;
}

void saveVec3Param(QDomDocument & doc, QDomElement & node, const char * name, const vec3 & v)
{
	saveStringParam(doc, node, name, vec3ToString(v));
}

void loadVec3Param(vec3 & v, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (!elem.isNull())
		v = vec3FromString(elem.text(), v);
}

// ------------------------------------------------------------------------ //

QString vec4ToString(const vec4 & v)
{
	return (v.x == v.y && v.x == v.z && v.x == v.w) ? QString().setNum(v.x) : QString().asprintf("%g %g %g %g", v.x, v.y, v.z, v.w);
}

vec4 vec4FromString(QString str, const vec4 & def)
{
	vec4 out;
	QTextStream stm(&str);
	stm >> out.x;
	if (stm.status() != QTextStream::Ok)
		return def;

	stm >> out.y >> out.z >> out.w;
	if (stm.status() != QTextStream::Ok)
		return vec4(out.x);

	return out;
}

void saveVec4Param(QDomDocument & doc, QDomElement & node, const char * name, const vec4 & v)
{
	saveStringParam(doc, node, name, vec4ToString(v));
}

void loadVec4Param(vec4 & v, const QDomNode & node, const char * name)
{
	QDomElement elem = node.firstChildElement(name);
	if (!elem.isNull())
		v = vec4FromString(elem.text(), v);
}

// ------------------------------------------------------------------------ //

vec2 getVec2(const QJsonObject & obj, const QString & key, const vec2 & defaultValue)
{
	if (obj.contains(key)) {
		QJsonArray arr = obj[key].toArray();
		if (arr.size() >= 2)
			return vec2(arr[0].toDouble(), arr[1].toDouble());
	}
	return defaultValue;
}

vec3 getVec3(const QJsonObject & obj, const QString & key, const vec3 & defaultValue)
{
	if (obj.contains(key)) {
		QJsonArray arr = obj[key].toArray();
		if (arr.size() >= 3)
		return vec3(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble());
	}
	return defaultValue;
}

vec4 getVec4(const QJsonObject & obj, const QString & key, const vec4 & defaultValue)
{
	if (obj.contains(key)) {
		QJsonArray arr = obj[key].toArray();
		if (arr.size() >= 4)
			return vec4(arr[0].toDouble(), arr[1].toDouble(), arr[2].toDouble(), arr[3].toDouble());
	}
	return defaultValue;
}

void getArray(const QJsonObject & obj, const QString & key, float *p, int count)
{
	if (obj.contains(key)) {
		QJsonArray arr = obj[key].toArray();
		if (arr.size() == count) {
			for (int i = 0; i < count; i++)
				p[i] = (float)arr[i].toDouble();
		}
	}
}

void getArray(const QJsonObject & obj, const QString & key, int *p, int count)
{
	if (obj.contains(key)) {
		QJsonArray arr = obj[key].toArray();
		if (arr.size() == count) {
			for (int i = 0; i < count; i++)
				p[i] = arr[i].toInt();
		}
	}
}

QJsonArray toJsonArray(const vec2 & v)
{
	return QJsonArray{ v.x, v.y };
}

QJsonArray toJsonArray(const vec3 & v)
{
	return QJsonArray{ v.x, v.y, v.z };
}

QJsonArray toJsonArray(const vec4 & v)
{
	return QJsonArray{ v.x, v.y, v.z, v.w };
}

// ------------------------------------------------------------------------ //

SceneModification sceneModificationAdd(ISceneElement *element, INode *targetNode, int position)
{
	SceneModification mod;
	mod.type = SceneModification_Add;
	mod.sourceElement = element;
	mod.targetNode = targetNode;
	mod.insertPosition = position >= 0 ? position : (targetNode ? static_cast<int>(targetNode->numChildren(SceneElement_All)) : 0);
	return mod;
}

SceneModification sceneModificationDelete(ISceneElement *element, int pos)
{
	SceneModification mod;
	mod.type = SceneModification_Delete;
	mod.sourceElement = element;
	mod.targetNode = element->parent();
	mod.insertPosition = pos;
	return mod;
}

SceneModification sceneModificationMove(ISceneElement *element, INode *targetNode, int position)
{
	SceneModification mod;
	mod.type = SceneModification_Move;
	mod.sourceElement = element;
	mod.targetNode = targetNode;
	mod.insertPosition = position;
	return mod;
}

SceneModification sceneModificationVisibility(ISceneElement *element)
{
	SceneModification mod;
	mod.type = SceneModification_Visibility;
	mod.sourceElement = element;
	return mod;
}

SceneModification sceneModificationMaterial(ISceneElement *element)
{
	SceneModification mod;
	mod.type = SceneModification_Material;
	mod.sourceElement = element;
	return mod;
}

// ------------------------------------------------------------------------ //

QMap<QString, QString> generateUniqueFileNamesForResourcesCollection(const QSet<QString> &filenames, const QSet<QString> &namesToAvoid)
{
	QMap<QString, QString> res;

	struct InsensitiveCompare {
		bool operator() (const QString &a, const QString &b) const { return a.compare(b, Qt::CaseInsensitive) < 0; }
	};

	std::set<QString, InsensitiveCompare> names;
	for (auto a : namesToAvoid)
		names.insert(a);

	auto listFileNames = filenames.values();
	std::sort(listFileNames.begin(), listFileNames.end());
	for (auto fname : listFileNames) {
		QFileInfo fi(fname);
		auto filename = fi.fileName();
		if (names.contains(filename)) {
			auto base = fi.completeBaseName();
			auto suffix = fi.suffix();

			for (int i = 1; i < INT_MAX; i++) {
				auto newname = QString("%1-%2.%3").arg(base).arg(i).arg(suffix);
				if (!names.contains(newname)) {
					filename = newname;
					break;
				}
			}
		}

		names.insert(filename);
		res[fname] = filename;
	}

	return res;
}

// ------------------------------------------------------------------------ //

template<typename T, int MAX_VAL>
bool doCheckIfImageHasTransparentPixels(const IImage *image)
{
	const int numPixels = image->width() * image->height();
	auto data = (const T *)image->rawData() + 3;
	for (int i = 0; i < numPixels; i++, data += 4)
		if (*data < MAX_VAL)
			return true;
	return false;
}

bool checkIfImageHasTransparentPixels(const IImage *image)
{
	if (image->format() != eImageFormat::RGBA)
		return false;

	switch (image->dataType()) {
		case eImageDataType::Byte:
			return doCheckIfImageHasTransparentPixels<byte, 255>(image);
		case eImageDataType::Float:
			return doCheckIfImageHasTransparentPixels<float, 1>(image);
	}

	assert(false);
	return false;
}

bool checkIfImageIsNormalMap(const ImagePtr &image)
{
	if (!image || image->format() == eImageFormat::Grayscale)
		return false;

	const int width = image->width();
	const int height = image->height();
	if (width <= 0 || height <= 0)
		return false;

	const int stepX = std::max(1, width / 32);
	const int stepY = std::max(1, height / 32);

	int validNormals = 0;
	int totalSamples = 0;
	float sumR = 0.f;
	float sumG = 0.f;
	float sumB = 0.f;

	for (int y = 0; y < height; y += stepY) {
		for (int x = 0; x < width; x += stepX) {
			const vec4 c = image->getPixel(x, y);
			if (c.a < 0.1f)
				continue;

			sumR += c.r;
			sumG += c.g;
			sumB += c.b;

			const float nx = 2.f * c.r - 1.f;
			const float ny = 2.f * c.g - 1.f;
			const float nz = 2.f * c.b - 1.f;
			const float lenSq = nx * nx + ny * ny + nz * nz;

			if (lenSq >= 0.5f && lenSq <= 1.6f && c.b > 0.4f)
				validNormals++;

			totalSamples++;
		}
	}

	if (totalSamples == 0)
		return false;

	const float meanR = sumR / (float)totalSamples;
	const float meanG = sumG / (float)totalSamples;
	const float meanB = sumB / (float)totalSamples;
	const float validRatio = (float)validNormals / (float)totalSamples;

	return (validRatio > 0.7f &&
			meanR > 0.35f && meanR < 0.65f &&
			meanG > 0.35f && meanG < 0.65f &&
			meanB > 0.65f);
}
