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

#include "FileNameParameterImpl.h"
#include "../../../Shared/Interfaces/SerializationContext.h"

FileNameParameterImpl::FileNameParameterImpl(const FileFormatsList& exts, eParamId paramId, IParameterOwner & owner)
	: m_owner(owner)
	, m_extensions(exts)
	, m_id(paramId)
	, m_enabled(true)
	, m_visible(true)
{
}

FileNameParameterImpl::~FileNameParameterImpl()
{
}

void FileNameParameterImpl::setEnabled(bool b)
{
	if (m_enabled != b) {
		m_enabled = b;
		emit changed();
	}
}

void FileNameParameterImpl::setVisible(bool b)
{
	if (m_visible != b) {
		m_visible = b;
		emit changed();
	}
}

void FileNameParameterImpl::fireChanged(eParamId paramId)
{
	emit changed();
	m_owner.fireChanged(m_id);
}

void FileNameParameterImpl::set(const QString & fileName)
{
	const auto processedFileName = m_owner.core().appContext().getShortResourcePath(fileName);
	if (m_fileName.compare(processedFileName))
		m_owner.pushCommand(new ParamCommand<FileNameParameterImpl, QString>(*this, m_owner.core().scene(), m_fileName, processedFileName, m_id));
}

void FileNameParameterImpl::_set(const QString & fileName)
{
	QString processedFileName = m_owner.core().appContext().getShortResourcePath(fileName);
	if (processedFileName.startsWith("owlet://")) { // owlet:// is a legacy prefix, we need to replace it with miray://
		processedFileName.replace(0, 8, "miray://");
	}
	if (!processedFileName.startsWith("miray://")) {
		processedFileName = QDir::toNativeSeparators(processedFileName);
	}
	if (m_fileName.compare(processedFileName)) {
		m_fileName = processedFileName;
		emit changed();
	}
}

bool FileNameParameterImpl::load(const QDomElement & node, const char * name, const ModelLoadingContext &ctx)
{
	bool res = loadStringParam(m_fileName, node, name);
	if (m_owner.core().appContext().isInternalResource(m_fileName)) {
		m_fileName = m_owner.core().appContext().getShortResourcePath(m_fileName);
		if (m_fileName.startsWith("owlet://")) { // owlet:// is a legacy prefix, we need to replace it with miray://
			m_fileName.replace(0, 8, "miray://");
		}
	} else {
		m_fileName = resolveFilePath(m_fileName, ctx.sourceFolder);
	}
	return res;
}

void FileNameParameterImpl::save(QJsonObject & obj, const char * name, const ModelSavingContext &ctx) const
{
	if (m_fileName.isEmpty())
		return;

	QString filenameToSave;

	if (!ctx.mapFileNamesOverride.empty() && ctx.mapFileNamesOverride.contains(m_fileName)) {
		// resources collection
		filenameToSave = ctx.mapFileNamesOverride[m_fileName];
	} else {
		// standard saving
		if (m_owner.core().appContext().isInternalResource(m_fileName))
			filenameToSave = m_owner.core().appContext().getShortResourcePath(m_fileName);
		else
			filenameToSave = relativePath(m_fileName, ctx.targetFolder);
	}

	obj[name] = filenameToSave;
}

bool FileNameParameterImpl::load(const QJsonObject & obj, const char * name, const ModelLoadingContext &ctx)
{
	if (!obj.contains(name))
		return false;

	m_fileName = obj[name].toString();
	if (m_owner.core().appContext().isInternalResource(m_fileName)) {
		m_fileName = m_owner.core().appContext().getShortResourcePath(m_fileName);
		if (m_fileName.startsWith("owlet://")) { // owlet:// is a legacy prefix, we need to replace it with miray://
			m_fileName.replace(0, 8, "miray://");
		}
	} else {
		m_fileName = resolveFilePath(m_fileName, ctx.sourceFolder);
	}
	return true;
}
