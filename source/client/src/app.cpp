#include "app.h"
#include "NetworkTCP.h"
#include "TcpSendRecvJpeg.h"
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>


// Label: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Label.html
// Box: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Box.html
// CheckButton: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1CheckButton.html
// Button: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Button.html
// Entry: https://developer.gnome.org/gtkmm/3.24/classGtk_1_1Entry.html

using namespace cv;
using namespace std;

App::App()
    : m_VBox_Left(Gtk::ORIENTATION_VERTICAL),
      m_VBox_Right(Gtk::ORIENTATION_VERTICAL),
      m_Button_Login("Login"),
      m_Button_Logout("Logout"),
      m_CheckButton_Secure("Secure Mode"),
      m_Button_Run("Run Mode"),
      m_Button_TestRun("Test Run Mode"),
      m_Button_LearnCapture("Learn Mode - Capture"),
      m_Button_LearnSave("Learn Mode - Save"),
      m_Label_Password("Pass:  "),
      m_Label_Login("Login: "),
      m_Label_Name("Name:  ")
{
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
    m_CheckButton_Secure.set_active(true);
    m_VBox_Left.pack_start(m_Button_Run);
    m_VBox_Left.pack_start(m_Button_TestRun);
    m_VBox_Left.pack_start(m_Button_LearnCapture);
    m_VBox_Left.add(m_HBox_Name);

    m_Entry_Name.set_max_length(20);
    m_HBox_Name.pack_start(m_Label_Name);
    m_HBox_Name.pack_start(m_Entry_Name);

    m_VBox_Left.pack_start(m_Button_LearnSave);

    m_CheckButton_Secure.signal_toggled().connect( sigc::mem_fun(*this, &App::on_checkbox_secure_toggled) );
    m_Button_Login.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_login) );
    m_Button_Logout.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_logout) );
    m_Button_Run.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_run) );
    m_Button_TestRun.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_test_run) );
    m_Button_LearnCapture.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_learn_capture) );
    m_Button_LearnSave.signal_clicked().connect( sigc::mem_fun(*this, &App::on_button_learn_save) );

    m_Entry_Password.set_visibility(false);
    m_Entry_Password.set_input_purpose(Gtk::INPUT_PURPOSE_PASSWORD);
    m_CheckButton_Secure.set_active(true);

    m_Button_Logout.set_sensitive(false);
    m_Button_Run.set_sensitive(false);
    m_Button_TestRun.set_sensitive(false);
    m_Button_LearnCapture.set_sensitive(false);
    m_Button_LearnSave.set_sensitive(false);
    m_CheckButton_Secure.set_sensitive(false);
    m_Entry_Name.set_sensitive(false);

    m_Button_LearnSave.set_can_default();
    m_Button_LearnSave.grab_default();

    show_all_children();
}

App::~App()
{
}
    
gboolean App::connect_server ()
{
	TTcpConnectedPort *TcpConnectedPort=NULL;
	bool retvalue;

	if ((TcpConnectedPort=OpenTcpConnection("192.168.0.228", "5000",
			"../../custom/keys/ca/ca.crt",
			"../../custom/keys/client/client.crt",
			"../../custom/keys/client/client.key"))==NULL)	// Open UDP Network port
	{
		printf("OpenTcpConnection\n");
		return(-1);
	}

	namedWindow( "Server", WINDOW_AUTOSIZE );// Create a window for display.

	Mat Image;
	do {
		retvalue=TcpRecvImageAsJpeg(TcpConnectedPort,&Image);

		if( retvalue) imshow( "Server", Image ); // If a valid image is received then display it
		else break;

	} while (waitKey(10) != 'q'); // loop until user hits quit

	CloseTcpConnectedPort(&TcpConnectedPort); // Close network port;
	return TRUE;
}

void App::on_checkbox_secure_toggled()
{
    g_print("on_checkbox_secure_toggled, %d\n", m_CheckButton_Secure.get_active());
}

void App::on_button_login()
{
    g_print("id: %s\n", m_Entry_Id.get_text().c_str());
    g_print("pass: %s\n", m_Entry_Password.get_text().c_str());
    g_print("on_button_login\n");

    if (true) { //:TODO: check validation
        m_Entry_Id.set_sensitive(false);
        m_Entry_Password.set_sensitive(false);
        m_Button_Login.set_sensitive(false);
        m_Button_Logout.set_sensitive(true);
        m_Button_Run.set_sensitive(true);
        m_Button_TestRun.set_sensitive(true);
        m_Button_LearnCapture.set_sensitive(true);
        m_Button_LearnSave.set_sensitive(true);
        m_CheckButton_Secure.set_sensitive(true);
        m_Entry_Name.set_sensitive(true);
    }

    this->connect_server();
}

void App::on_button_logout()
{
    g_print("on_button_logout\n");

    if (true) { //:TODO: check validation
        m_Entry_Id.set_text("");
        m_Entry_Password.set_text("");
        m_Entry_Id.set_sensitive(true);
        m_Entry_Password.set_sensitive(true);
        m_Button_Login.set_sensitive(true);
        m_Button_Logout.set_sensitive(false);
        m_Button_Run.set_sensitive(false);
        m_Button_TestRun.set_sensitive(false);
        m_Button_LearnCapture.set_sensitive(false);
        m_Button_LearnSave.set_sensitive(false);
        m_CheckButton_Secure.set_sensitive(false);
        m_Entry_Name.set_sensitive(false);
    }
}

void App::on_button_run()
{
    g_print("on_button_run\n");
}

void App::on_button_test_run()
{
    g_print("on_button_test_run\n");
}

void App::on_button_learn_capture()
{
    g_print("on_button_learn_capture\n");
}

void App::on_button_learn_save()
{
    g_print("Name: %s\n", m_Entry_Name.get_text().c_str());
    g_print("on_button_learn_save\n");
}
