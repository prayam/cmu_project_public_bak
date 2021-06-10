#ifndef APP_H
#define APP_H

#include <gtkmm.h>
#include "NetworkTCP.h"

class App : public Gtk::Window
{
public:
	App();
	virtual ~App();

	guint handle_port_secure_id;
	gboolean disconn_req;
	TTcpConnectedPort *port_control;
	TTcpConnectedPort *port_secure;
	TTcpConnectedPort *port_nonsecure;
	TTcpConnectedPort *port_meta;

protected:
	//Signal handlers:
	void on_checkbox_secure_toggled();
	void on_checkbox_test_toggled();
	void on_button_login();
	void on_button_logout();
	void on_button_learn_capture();
	void on_button_learn_save();
	void on_entry_changed();
	virtual bool on_delete_event(GdkEventAny* any_event) override;

	//Child widgets:
	Gtk::Box m_HBox_Top, m_HBox_LoginId, m_HBox_LoginPassword, m_HBox_LoginAction, m_VBox_Left, m_VBox_Right, m_HBox_Name;
	Gtk::Entry m_Entry_Id, m_Entry_Password, m_Entry_Name;
	Gtk::Button m_Button_Login, m_Button_Logout, m_Button_LearnCapture, m_Button_LearnSave;
	Gtk::Label m_Label_Login, m_Label_Password, m_Label_Name;
	Gtk::CheckButton m_CheckButton_Secure, m_CheckButton_Test;
	std::unique_ptr<Gtk::MessageDialog> m_pDialog;

private:
	void show_dialog(const char* contents);
	gboolean connect_server ();
	gboolean disconnect_server ();
};

#endif