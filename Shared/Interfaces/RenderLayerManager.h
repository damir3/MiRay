#pragma once

class IRenderLayer;

class SHAREDLIB_EXPORT IRenderLayerManager : public QObject
{
	Q_OBJECT

protected:
	virtual ~IRenderLayerManager() {}

public:
	virtual int count() const = 0;
	virtual IRenderLayer *get(int i) const = 0;
	virtual size_t getIndex(IRenderLayer * renderLayer) const = 0;

	virtual IRenderLayer *create(size_t position = (size_t)-1) = 0;
	virtual void remove(size_t index) = 0;
	virtual void move(size_t from, size_t to) = 0;

signals:
	void changed();
	void stateChanged(IRenderLayer *);
};
