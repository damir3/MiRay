#pragma once

class IMaterial;
struct ModelLoadingContext;
typedef std::set<IMaterial *>	MaterialSelection;

class SHAREDLIB_EXPORT IMaterialManager : public QObject
{
	Q_OBJECT

protected:
	virtual ~IMaterialManager() {}

public:
	virtual IMaterial * create(const QString & name) = 0;
	virtual IMaterial * load(const QByteArray & data, const ModelLoadingContext & ctx, size_t position) = 0;
	virtual void remove(const MaterialSelection & materials) = 0;
	virtual void move(IMaterial * material, size_t pos) = 0;
	virtual void removeUnusedMaterials() = 0;

	virtual void replaceWithSerializedOne(IMaterial * material, const QByteArray & data, const ModelLoadingContext & ctx) = 0;

	virtual int count() const = 0;
	virtual IMaterial *get(int pos) const = 0;
	virtual bool isDefault(const IMaterial * material) const = 0;

	virtual IMaterial *getByName(const QString & name) const = 0;

	virtual bool isIORFile(const QString & filename) const = 0;
	virtual IMaterial *createFromIORFile(const QString & filename) = 0;

signals:
	void materialsListChanged();
	void materialChanged(const IMaterial * material, bool completelyChanged);
};
