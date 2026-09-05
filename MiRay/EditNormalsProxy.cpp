#include "EditNormalsProxy.h"

EditNormalsProxy::EditNormalsProxy(std::unique_ptr<IEditNormals> editNormals, QQmlContext * context)
	: m_editNormals(std::move(editNormals))
	, m_context(context)
	, m_calculateNormals("Calculate normals")
	, m_makeEdges("Make edges")
	, m_maxSoftAngle("Max soft angle")
	, m_flipNormals("Flip normals")
	, m_flipFacing("Flip facing")
{
	m_calculateNormals.setParam(&m_editNormals->calculateNormals());
	m_makeEdges.setParam(&m_editNormals->makeEdges());
	m_maxSoftAngle.setParam(&m_editNormals->maxSoftAngle());
	m_flipNormals.setParam(&m_editNormals->flipNormals());
	m_flipFacing.setParam(&m_editNormals->flipFacing());

	m_data["calculateNormals"] = QVariant::fromValue(static_cast<QObject *>(&m_calculateNormals));
	m_data["makeEdges"] = QVariant::fromValue(static_cast<QObject *>(&m_makeEdges));
	m_data["maxSoftAngle"] = QVariant::fromValue(static_cast<QObject *>(&m_maxSoftAngle));
	m_data["flipNormals"] = QVariant::fromValue(static_cast<QObject *>(&m_flipNormals));
	m_data["flipFacing"] = QVariant::fromValue(static_cast<QObject *>(&m_flipFacing));

	m_context->setContextProperty("editNormalsModel", m_data);
}

EditNormalsProxy::~EditNormalsProxy()
{
	m_context->setContextProperty("editNormalsModel", nullptr);
}

void EditNormalsProxy::accept()
{
	m_editNormals->accept();
}
