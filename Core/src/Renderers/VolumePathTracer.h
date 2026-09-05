#pragma once

class VolumePathTracer final : public BaseRenderer
{
	void traceCameraPath(TraceResult & res, TraceContext & ctx) override;

public:
	VolumePathTracer(Scene & scene);
	~VolumePathTracer();
};
