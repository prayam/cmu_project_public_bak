#ifndef APP_H
#define APP_H

#include <gtkmm.h>
#include "NetworkTCP.h"

typedef enum {
	CLIENT_STATE_INIT,
	CLIENT_STATE_LOGOUT,
	CLIENT_STATE_SECURE_RUN,
	CLIENT_STATE_SECURE_TESTRUN,
	CLIENT_STATE_NONSECURE_RUN,
	CLIENT_STATE_NONSECURE_TESTRUN,
	CLIENT_STATE_LEARN
} CLIENT_STATE;

typedef enum {
	LEARN_NONE,
	LEARN_REQUESTED,
	LEARN_READY,
	LEARN_DONE
} LEARN_MODE_STATE;

class App : public Gtk::Window
{
public:
	App();
	virtual ~App();

	LEARN_MODE_STATE learn_mode_state;
	Gtk::Image m_Image;
	guint handle_port_secure_id;
	gboolean connected_server;
	TTcpConnectedPort *port_recv_photo;
	TTcpConnectedPort *port_control;
	TTcpConnectedPort *port_secure;
	TTcpConnectedPort *port_nonsecure;
	TTcpConnectedPort *port_meta;

	void on_button_logout();
	void show_dialog(const char* contents);
	CLIENT_STATE get_current_app_state();

	//Signal handlers:
	void on_checkbox_secure_toggled();
	void on_checkbox_test_toggled();
	void on_button_login();
	void on_button_pause_resume();
	void on_button_learn_save();
	void on_entry_changed();
	void on_entry_id_pass_changed();
	virtual bool on_delete_event(GdkEventAny* any_event) override;

	//Child widgets:
	Gtk::Box m_HBox_Top, m_HBox_LoginId, m_HBox_LoginPassword, m_HBox_LoginAction, m_VBox_Left, m_VBox_Right, m_HBox_Name;
	Gtk::Entry m_Entry_Id, m_Entry_Password, m_Entry_Name;
	Gtk::Button m_Button_Login, m_Button_Logout, m_Button_PauseResume, m_Button_LearnSave;
	Gtk::Label m_Label_Login, m_Label_Password, m_Label_Name;
	Gtk::CheckButton m_CheckButton_Secure, m_CheckButton_Test;
	std::unique_ptr<Gtk::MessageDialog> m_pDialog;

private:
	gboolean check_valid_input (const gchar *regex, const gchar *target);
	gboolean connect_server ();
	gboolean disconnect_server ();
};

#endif