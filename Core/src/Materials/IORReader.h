#pragma once

namespace ior
{
	bool isIORFile(QString filename);
	std::vector<vec3> readFromFile(QString fileName, ILog *log);

	namespace text
	{
		std::vector<vec3> readData(QByteArray data);
	}; // namespace text

	namespace sopra
	{
		std::vector<vec3> readData(QByteArray data);

	}; // namespace sopra

}; // namespace ior
