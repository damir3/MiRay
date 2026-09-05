#pragma once

class IRenderLayer
{
protected:
	virtual ~IRenderLayer() {}

public:
	virtual IStringParameter & name() = 0;
};
