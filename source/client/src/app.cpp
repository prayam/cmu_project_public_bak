#include "app.h"
#include "TcpSendRecvJpeg.h"
#include "CommonStruct.h"
#include "Logger.h"
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

// Label: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Label.html
// Box: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Box.html
// CheckButton: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1CheckButton.html
// Button: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Button.html
// Entry: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Entry.html

using namespace cv;
using namespace std;

static gboolean change_thread_priority(gpointer data)
{
	LOG_INFO("hello timer");
	return G_SOURCE_REMOVE;
}

App::App()
	: m_VBox_Left(Gtk::ORIENTATION_VERTICAL),
	  m_VBox_Right(Gtk::ORIENTATION_VERTICAL),
	  m_Button_Login("Login"),
	  m_Button_Logout("Logout"),
	  m_Button_LearnCapture("Learn Mode - Capture"),
	  m_Button_LearnSave("Learn Mode - Save"),
	  m_Label_Login("Login: "),
	  m_Label_Password("Pass:  "),
	  m_Label_Name("Name:  "),
	  m_CheckButton_Secure("Secure Mode"),
	  m_CheckButton_Test("Test Mode")
{
	this->port_control = NULL;
	this->port_secure = NULL;
	this->port_nonsecure = NULL;
	this->port_meta = NULL;
	this->handle_port_secure_id = 0;

	// set_size_request(800, 600);
	set_title("App");

	add(m_HBox_Top);
	m_HBox_Top.add(m_VBox_Left);
	m_HBox_Top.add(m_VBox_Right);

	m_VBox_Left.add(m_HBox_LoginId);
	m_VBox_Left.add(m_HBox_LoginPassword);
	m_VBox_Left.add(m_HBox_LoginAction);

	m_Entry_Id.set_max_length(10);
	m_HBox_LoginId.add(m_Label_Login);
	m_HBox_LoginId.pack_start(m_Entry_Id);

	m_Entry_Password.set_max_length(20);
	m_HBox_LoginPassword.add(m_Label_Password);
	m_HBox_LoginPassword.pack_start(m_Entry_Password);

	m_HBox_LoginAction.pack_start(m_Button_Login);
	m_HBox_LoginAction.pack_start(m_Button_Logout);

	m_VBox_Left.pack_start(m_CheckButton_Secure);
	m_VBox_Left.pack_start(m_CheckButton_Test);
	m_VBox_Left.pack_start(m_Button_LearnCapture);
	m_VBox_Left.add(m_HBox_Name);

	m_Entry_Name.set_max_length(20);
	m_HBox_Name.pack_start(m_Label_Name);
	m_HBox_Name.pack_start(m_Entry_Name);

	m_VBox_Left.pack_start(m_Button_LearnSave);

	m_CheckButton_Secure.signal_toggled().connect( sigc::mem_fun(*this, &App::on_checkbox_secure_toggled) );
	m_CheckButton_Test.signal_toggled().connect( sigc::mem_fun(*this, &App::on_checkbox_test_toggled) );
	m_Button_Login.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_login) );
	m_Button_Logout.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_logout) );
	m_Button_LearnCapture.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_learn_capture) );
	m_Button_LearnSave.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_learn_save) );

	m_Entry_Password.set_visibility(false);
	m_Entry_Password.set_input_purpose(Gtk::INPUT_PURPOSE_PASSWORD);

	m_Entry_Password.signal_changed().connect( sigc::mem_fun(*this, &App::on_entry_changed) );
	m_Entry_Id.signal_changed().connect( sigc::mem_fun(*this, &App::on_entry_changed) );
	m_Entry_Name.signal_changed().connect( sigc::mem_fun(*this, &App::on_entry_changed) );

	m_CheckButton_Secure.set_active(true);
	m_CheckButton_Test.set_active(false);
	m_Button_Login.set_sensitive(false);
	m_Button_Logout.set_sensitive(false);
	m_Button_LearnCapture.set_sensitive(false);
	m_Button_LearnSave.set_sensitive(false);
	m_CheckButton_Secure.set_sensitive(false);
	m_CheckButton_Test.set_sensitive(false);
	m_Entry_Name.set_sensitive(false);
	m_Label_Name.set_sensitive(false);

	m_Entry_Id.set_can_default();
	m_Entry_Id.grab_default();

	show_all_children();
	g_timeout_add_seconds(5, (GSourceFunc) change_thread_priority, NULL);
}

App::~App()
{
	LOG_INFO("exit");
}

static gboolean handle_port_secure(gpointer data) {
	App *app = static_cast<App *>(data);
	Mat image;
    float fontScaler;
	std::vector<struct APP_meta> meta_vector;

	namedWindow( "Server", WINDOW_AUTOSIZE );// Create a window for displa.

	while (!app->disconn_req) {
		if (TcpRecvImageAsJpeg(app->port_secure, &image) <= 0) {
			LOG_WARNING("TcpRecvImageAsJpeg fail");
			break;
		}

		meta_vector.clear();
		if (!TcpRecvMeta(app->port_meta, meta_vector)) {
			LOG_WARNING("TcpRecvMeta fail");
			break;
		}

		for (struct APP_meta const & meta : meta_vector) {
			fontScaler = static_cast<float>(meta.x2 - meta.x1) / static_cast<float>(image.cols);
			putText(image, meta.name, Point(meta.y1 + 2, meta.x1 - 3),
				FONT_HERSHEY_DUPLEX, 0.1 + (2 * fontScaler * 3), Scalar(0, 0, 255, 255), 1);
			rectangle(image, Point(meta.y1, meta.x1), Point(meta.y2, meta.x2), Scalar(0, 0, 255), 2, 8, 0);
		}

		imshow("Server", image); // If a valid image is received then display it
		waitKey(10);
	}

	destroyWindow("Server");
	LOG_INFO("end func");

	app->handle_port_secure_id = 0;
	return G_SOURCE_REMOVE;
}

void App::show_dialog(const char* contents)
{
	m_pDialog.reset(new Gtk::MessageDialog(*this, contents, false, Gtk::MessageType::MESSAGE_ERROR, Gtk::ButtonsType::BUTTONS_OK, true));
	m_pDialog->signal_response().connect(sigc::hide(sigc::mem_fun(*m_pDialog, &Gtk::Widget::hide)));
	m_pDialog->show();
}

gboolean App::connect_server ()
{
	const gchar *ca = "../../custom/keys/ca/ca.crt";
	const gchar *crt = "../../custom/keys/client/client.crt";
	const gchar *key = "../../custom/keys/client/client.key";

	if (this->port_control != NULL) {
		LOG_WARNING("this.port_control is not NULL");
		return FALSE;
	}

	if (this->port_secure != NULL) {
		LOG_WARNING("this.port_secure is not NULL");
		return FALSE;
	}

	if (this->port_nonsecure != NULL) {
		LOG_WARNING("this.port_nonsecure is not NULL");
		return FALSE;
	}

	if (this->port_meta != NULL) {
		LOG_WARNING("this.port_meta is not NULL");
		return FALSE;
	}

	if ((this->port_control = OpenTcpConnection("192.168.0.228", "5000", ca, crt, key)) == NULL) // Open TCP TLS port for control data
	{
		LOG_WARNING("error open port_control");
		show_dialog("Connection Fail");
		return FALSE;
	}

	// send login info and check
	guint8 res = 255;
	TcpSendLoginData(this->port_control, m_Entry_Id.get_text().c_str(), m_Entry_Password.get_text().c_str());
	TcpRecvLoginRes(this->port_control, &res);

	if (res == 0) {
		LOG_WARNING("login fail");
		disconnect_server();
		show_dialog("Login Fail");
		return FALSE;
	}

	if ((this->port_secure = OpenTcpConnection("192.168.0.228", "5000", ca, crt, key)) == NULL)	// Open TCP TLS port for secure mode
	{
		LOG_WARNING("error open port_secure");
		disconnect_server();
		show_dialog("Connection Fail");
		return FALSE;
	}

	if ((this->port_nonsecure = OpenTcpConnection("192.168.0.228", "5000", NULL, NULL, NULL)) == NULL) // Open TCP TLS port for non_secure mode
	{
		LOG_WARNING("error open port_nonsecure");
		disconnect_server();
		show_dialog("Connection Fail");
		return FALSE;
	}

	if ((this->port_meta = OpenTcpConnection("192.168.0.228", "5000", ca, crt, key)) == NULL) // Open TCP TLS port for meta data
	{
		LOG_WARNING("error open port_meta");
		disconnect_server();
		show_dialog("Connection Fail");
		return FALSE;
	}

	this->disconn_req = FALSE;
	this->handle_port_secure_id = g_idle_add(handle_port_secure, this);
	return TRUE;
}

gboolean App::disconnect_server ()
{
	this->disconn_req = TRUE;

	if (this->port_control != NULL) {
		CloseTcpConnectedPort(&this->port_control); // Close network port;
		this->port_control = NULL;
	}

	if (this->port_secure != NULL) {
		CloseTcpConnectedPort(&this->port_secure); // Close network port;
		this->port_secure = NULL;
	}

	if (this->port_nonsecure != NULL) {
		CloseTcpConnectedPort(&this->port_nonsecure); // Close network port;
		this->port_nonsecure = NULL;
	}

	if (this->port_meta != NULL) {
		CloseTcpConnectedPort(&this->port_meta); // Close network port;
		this->port_meta = NULL;
	}

	if(this->handle_port_secure_id) {
		g_source_remove(this->handle_port_secure_id);
		this->handle_port_secure_id = 0;
	}

	return TRUE;
}

void App::on_button_login()
{
	if (this->connect_server()) {
		/* disables */
		m_Entry_Id.set_sensitive(false);
		m_Entry_Password.set_sensitive(false);
		m_Button_Login.set_sensitive(false);
		m_Button_LearnSave.set_sensitive(false);
		m_Label_Login.set_sensitive(false);
		m_Label_Password.set_sensitive(false);

		/* enables */
		m_Button_Logout.set_sensitive(true);
		m_Button_LearnCapture.set_sensitive(true);
		m_CheckButton_Secure.set_sensitive(true);
		m_CheckButton_Test.set_sensitive(true);
		m_Entry_Name.set_sensitive(true);
		m_Label_Name.set_sensitive(true);
	}
	else {
		m_Entry_Id.set_text("");
		m_Entry_Password.set_text("");
	}
}

void App::on_button_logout()
{
	this->disconnect_server();
	TcpSendLogoutReq(this->port_control);

	/* disables */
	m_Button_Logout.set_sensitive(false);
	m_Button_LearnCapture.set_sensitive(false);
	m_Button_LearnSave.set_sensitive(false);
	m_CheckButton_Secure.set_sensitive(false);
	m_CheckButton_Test.set_sensitive(false);
	m_Entry_Name.set_sensitive(false);
	m_Label_Name.set_sensitive(false);

	/* enables */
	m_Entry_Id.set_text("");
	m_Entry_Password.set_text("");
	m_Entry_Id.set_sensitive(true);
	m_Entry_Password.set_sensitive(true);
	m_Button_Login.set_sensitive(true);
	m_Label_Login.set_sensitive(true);
	m_Label_Password.set_sensitive(true);
}

void App::on_checkbox_secure_toggled()
{
	LOG_INFO("on_checkbox_secure_toggled, %d", m_CheckButton_Secure.get_active());

	if (m_CheckButton_Secure.get_active()) {
		// send secure mode
		TcpSendSecureModeReq(this->port_control);
	}
	else {
		// send non secure mode
		TcpSendNonSecureModeReq(this->port_control);
	}
}

void App::on_checkbox_test_toggled()
{
	LOG_INFO("on_checkbox_test_toggled, %d", m_CheckButton_Test.get_active());

	if (m_CheckButton_Test.get_active()) {
		// send test mode
		TcpSendTestRunModeReq(this->port_control);
	}
	else {
		// send run mode
		TcpSendRunModeReq(this->port_control);
	}
}

void App::on_button_learn_capture()
{
	//:TODO: Need to consider sequence.
	LOG_INFO("on_button_learn_capture");
	TcpSendCaptureReq(this->port_control);
}

void App::on_button_learn_save()
{
	LOG_INFO("Name: %s", m_Entry_Name.get_text().c_str());
	LOG_INFO("on_button_learn_save");
	TcpSendSaveReq(this->port_control, m_Entry_Name.get_text().c_str());
}

void App::on_entry_changed()
{
	if (m_Entry_Id.get_text().length() != 0 && m_Entry_Password.get_text().length() != 0) {
		m_Button_Login.set_sensitive(true);
	}
	else {
		m_Button_Login.set_sensitive(false);
	}

	if (m_Entry_Name.get_text().length() != 0) {
		m_Button_LearnSave.set_sensitive(true);
	}
	else {
		m_Button_LearnSave.set_sensitive(false);
	}
}

bool App::on_delete_event(GdkEventAny* any_event)
{
	// when app closing
	this->disconnect_server();
	return Gtk::Window::on_delete_event(any_event);
}