#include "engine/Application.h"

#include "ComputeShaderToy.h"

int main(int argc, char* argv[])
{
	ComputeShaderToy app("Compute Shader Toy", 1024u, 1024u);
	app.run();
	return 0;
}