struct PostProcessRenderer
{
	enum FX
	{
		eDownsample,
		eBloom,
		eCircularDof,
		eSSAO,
		eCount
	};

	void init();
	void render(FX fx);
	void render_downsample();
};