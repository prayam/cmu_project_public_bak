// File: main.cc

#include "app.h"
#include "Logger.h"
#include <gtkmm/application.h>

gint main (gint argc, gchar *argv[])
{
	log_enable("client");

	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");
	App window;

	//Shows the window and returns when it is closed.
	gint ret = app->run(window);

	log_disable();
	return ret;
}
