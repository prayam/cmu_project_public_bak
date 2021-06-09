// File: main.cc

#include "app.h"
#include "Logger.h"
#include <gtkmm/application.h>
/*
    auto app = Gtk::Application::create(argc, argv);

    Gtk::Window window;
    window.set_default_size(600,400);

    return app->run(window);
*/

int main (int argc, char *argv[])
{
	log_enable("client");

	// LOG_ERROR("TEST 1");
	LOG_CRITICAL("TEST 2");
	LOG_WARNING("TEST 3");
	LOG_MESSAGE("TEST 4 %.2x", 0x55);
	LOG_INFO("TEST 5 %s", "hello");
	LOG_DEBUG("TEST 6 %d %d", 1, 2);

	guint8 buffer[10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};

	LOG_HEX_DUMP_CRITICAL(buffer, sizeof(buffer), "TEST 1");
	LOG_HEX_DUMP_WARNING(buffer, sizeof(buffer), "TEST 2");
	LOG_HEX_DUMP_MESSAGE(buffer, sizeof(buffer), "TEST 3");
	LOG_HEX_DUMP_INFO(buffer, sizeof(buffer), "TEST 4 %.2x", 0x55);
	LOG_HEX_DUMP_DEBUG(buffer, sizeof(buffer), "TEST 5 %s", "hello");

	auto app = Gtk::Application::create(argc, argv, "org.gtkmm.example");
	App window;

	//Shows the window and returns when it is closed.
	int ret = app->run(window);

	log_disable();
	return ret;
}
