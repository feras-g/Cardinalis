#include "engine/Application.h"

#include "SampleProject.h"

int main(int argc, char* argv[])
{
	SampleProject app("Sample Project", 1024u, 1024u);
	app.run();
	return 0;
}