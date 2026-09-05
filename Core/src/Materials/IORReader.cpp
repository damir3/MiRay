#include "IORReader.h"

bool ior::isIORFile(QString filename)
{
	QFileInfo fi(filename);
	auto ext = fi.suffix();

	if (ext.compare("txt", Qt::CaseInsensitive) == 0)
		return true;
	if (ext.compare("ior", Qt::CaseInsensitive) == 0)
		return true;

	return false;
}

std::vector<vec3> ior::readFromFile(QString fileName, ILog *log)
{
	QFileInfo fi(fileName);
	if (!fi.exists())
		return std::vector<vec3>();

	auto ext = fi.suffix();

	try {
		auto data = readFile(fileName);

		if (ext.compare("txt", Qt::CaseInsensitive) == 0)
			return text::readData(data);

		if (ext.compare("ior", Qt::CaseInsensitive) == 0 || ext.compare("nk", Qt::CaseInsensitive) == 0)
			return sopra::readData(data);
	} catch (const std::exception &ex) {
		if (log)
			log->error(QString("Error loading '%1': %2").arg(fileName).arg(ex.what()));
	}

	return std::vector<vec3>();
}

std::vector<vec3> ior::text::readData(QByteArray data)
{
	std::vector<vec3> iors;

	data.replace(',', '\t'); // convert CSV to TSV

	std::map<float, float>	map_n, map_k;

	int format = 0;
	QTextStream ts(&data);
	QRegExp re("\\s+");
	while (!ts.atEnd()) {
		QString line = ts.readLine();
		QStringList s = line.split(re);
		if (s.size() < 2) continue;
		if (!format) {
			if (!s[0].compare("Wavelength(nm)") && !s[1].compare("n"))
				format = 1;
			else if (!s[0].compare("wl") && !s[1].compare("n"))
				format = 2;
			else
				break;
		} else {
			switch (format) {
				case 1: {
					auto ior = s[0].toFloat();
					auto n = s[1].toFloat();
					map_n[ior] = n;
					if (s.size() > 2)
						map_k[ior] = s[2].toFloat();
					break;
				}
				case 2: {
					if (!s[0].compare("wl") && !s[1].compare("k")) {
						format = 3;
						break;
					}

					auto ior = s[0].toFloat() * 1e3f;
					auto n = s[1].toFloat();
					map_n[ior] = n;
					break;
				}
				case 3: {
					auto ior = s[0].toFloat() * 1e3f;
					auto k = s[1].toFloat();
					map_k[ior] = k;
					break;
				}
			}
		}
	}

	auto it_n = map_n.begin(), it_k = map_k.begin();

	auto has_n = !map_n.empty();
	auto has_k = !map_k.empty();

	auto wl_n = 0.f, wl_k = 0.f, n = 1.f, k = 0.f;
	while (true) {
		auto valid_n = it_n != map_n.end();
		auto valid_k = it_k != map_k.end();
		if (!valid_n && !valid_k)
			break;

		wl_n = valid_n ? it_n->first : wl_n;
		n = valid_n ? it_n->second : n;

		wl_k = valid_k ? it_k->first : wl_k;
		k = valid_k ? it_k->second : k;

		if (valid_n && valid_k && fabs(wl_n - wl_k) < 0.01f) {
			iors.push_back(vec3(wl_n, n, k));
			it_n++;
			it_k++;
		} else if (valid_n && (wl_n < wl_k || !valid_k)) {
			iors.push_back(vec3(wl_n, n, has_k ? -1 : 0.f));
			it_n++;
		} else if (valid_k && (wl_k < wl_n || !valid_n)) {
			iors.push_back(vec3(wl_k, has_n ? -1 : 1.f, k));
			it_k++;
		}
	}

	const auto original = iors; // we use interpolation for missing values, so base values should not be affected by the interpolated ones. that's why we make a copy of iors and use it as a source

	for (int i = 0; i < static_cast<int>(iors.size()); i++) {
		auto &v = iors[i];

		if (v.y < 0) {
			int idx_min = -1;
			for (int j = i - 1; j >= 0 && idx_min < 0; j--)
				if (original[j].y >= 0)
					idx_min = j;
			int idx_max = -1;
			for (int j = i + 1; j < static_cast<int>(original.size()) && idx_max < 0; j++)
				if (original[j].y >= 0)
					idx_max = j;

			if (idx_min < 0 && idx_max < 0) { // no N component at all
				iors[i].y = 1.f;
			} else if (idx_min < 0) { // no N components before this wavelength, use the upper one
				iors[i].y = original[idx_max].y;
			} else if (idx_max < 0) { // no N components after this wavelength, use the lower one
				iors[i].y = original[idx_min].y;
			} else {
				auto wl_min = original[idx_min].x;
				auto wl_max = original[idx_max].x;
				auto wl = iors[i].x;
				auto f = (wl - wl_min) / (wl_max - wl_min);
				iors[i].y = lerp(original[idx_min].y, original[idx_max].y, f);
			}
		}

		if (v.z < 0) {
			int idx_min = -1;
			for (int j = i - 1; j >= 0 && idx_min < 0; j--)
				if (original[j].z >= 0)
					idx_min = j;
			int idx_max = -1;
			for (int j = i + 1; j < static_cast<int>(original.size()) && idx_max < 0; j++)
				if (original[j].z >= 0)
					idx_max = j;

			if (idx_min < 0 && idx_max < 0) { // no K component at all
				iors[i].z = 0.f;
			} else if (idx_min < 0) { // no K components before this wavelength, use the upper one
				iors[i].z = original[idx_max].z;
			} else if (idx_max < 0) { // no K components after this wavelength, use the lower one
				iors[i].z = original[idx_min].z;
			} else {
				auto wl_min = original[idx_min].x;
				auto wl_max = original[idx_max].x;
				auto wl = iors[i].x;
				auto f = (wl - wl_min) / (wl_max - wl_min);
				iors[i].z = lerp(original[idx_min].z, original[idx_max].z, f);
			}
		}
	}

	return iors;
}

std::vector<vec3> ior::sopra::readData(QByteArray data)
{
	data.replace(',', '\t'); // convert CSV to TSV

	QTextStream ts(&data);

	int unit, steps;
	double from, to;

	ts >> unit >> from >> to >> steps;
	if (ts.status() != QTextStream::Ok)
		throw std::runtime_error("Can't read header");

	if (steps <= 0)
		throw std::runtime_error("Negative steps number in header");

	if (steps > 10000)
		throw std::runtime_error("The number of steps is a way too large to be true");

	if (unit < 1 || unit > 4)
		throw std::runtime_error(QString("Invalid spectral unit in header: %1").arg(unit).toStdString());

	std::vector<vec3> iors;
	iors.reserve(steps + 1);

	for (int i = 0; i <= steps; i++) {
		double d = lerp(from, to, static_cast<double>(i) / steps);
		double wl;

		switch (unit) {
			case 1: // eV
				wl = 1240 / d;
				break;
			case 2: // micro-meters - convert to nm
				wl = d * 1000;
				break;
			case 3: // 1/cm
				wl = 10 * 1000 * 1000 / d;
				break;
			case 4: // nm, default
				wl = d;
				break;
			default:
				throw std::runtime_error(QString("Invalid spectral unit in header: %1").arg(unit).toStdString());
		}

		double n, k;
		ts >> n >> k;

		if (ts.status() != QTextStream::Ok)
			throw std::runtime_error(QString("Error reading line #%1").arg(i + 1).toStdString());

		iors.push_back(vec3(wl, n, k));
	}

	return iors;
}
