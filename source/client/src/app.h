#ifndef APP_H
#define APP_H

#include <gtkmm.h>
#include "NetworkTCP.h"

class App : public Gtk::Window
{
public:
    App();
    virtual ~App();

	guint compute_func_id;
	gboolean disconn_req;
	TTcpConnectedPort *tcp_connected_port;

protected:
    //Signal handlers:
    void on_checkbox_secure_toggled();
    void on_button_login();
    void on_button_logout();
    void on_button_run();
    void on_button_test_run();
    void on_button_learn_capture();
    void on_button_learn_save();

    //Child widgets:
    Gtk::Box m_HBox_Top, m_HBox_LoginId, m_HBox_LoginPassword, m_HBox_LoginAction, m_VBox_Left, m_VBox_Right, m_HBox_Name;
    Gtk::Entry m_Entry_Id, m_Entry_Password, m_Entry_Name;
    Gtk::Button m_Button_Login, m_Button_Logout, m_Button_Run, m_Button_TestRun, m_Button_LearnCapture, m_Button_LearnSave;
    Gtk::Label m_Label_Login, m_Label_Password, m_Label_Name;
    Gtk::CheckButton m_CheckButton_Secure;

private:
    gboolean connect_server ();
};

#endif