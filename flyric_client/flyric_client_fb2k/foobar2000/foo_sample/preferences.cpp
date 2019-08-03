#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>

static const GUID guid_config_branch = { 0xaa4f3edc, 0xbf2a, 0x4eb5, {0x91, 0xbd, 0x89, 0xef, 0xe8, 0x4a, 0x31, 0x2b } };

static const GUID guid_cfg_enable_remote = { 0xe1c0e96b,0x2549 ,0x4b15 ,{ 0xb2, 0x8e, 0x8c, 0x5e, 0x0f, 0xc5, 0xa5, 0xc7} };
static const GUID guid_cfg_enable_console = { 0x48891b35, 0x41c7, 0x4e88, {0x91, 0x09, 0xd3, 0xf1, 0x09, 0x0c, 0xae, 0xce } };
static const GUID guid_cfg_remote_ip = { 0x03132906, 0x5fb4, 0x496e, {0x91, 0x66, 0xe8, 0x45, 0x1e, 0x4b, 0xc6, 0x9c } };
static const GUID guid_cfg_remote_port = { 0x45e94a0d, 0xf89e, 0x4c25, {0xa1, 0x52, 0x8a, 0xfb, 0x98, 0x3b, 0xa8, 0x43 } };
static const GUID guid_cfg_render_text = { 0x994e9dfb, 0x5fa9, 0x43cd, {0x9c, 0x8f, 0x10, 0x12, 0xa7, 0x0a, 0x24, 0x3e } };
// some GUID
/*

{0x70ad83c0, 0x9202, 0x4daf, {0xa1, 0xe0, 0x07, 0x43, 0xc1, 0x9f, 0x33, 0x7e } }
{0x5fe8315e, 0x009d, 0x4afb, {0x88, 0x19, 0x78, 0xe1, 0x06, 0x61, 0xea, 0xbe } }
{0xb0aa6fd9, 0xed90, 0x4092, {0x98, 0xfb, 0x2a, 0xc9, 0x7e, 0x3f, 0x9f, 0x76 } }
{0x96ce2e09, 0x675e, 0x42dc, {0x86, 0xdd, 0xdd, 0xb7, 0x61, 0x63, 0x0b, 0xeb } }
{0x706aa9c0, 0xbe3e, 0x45cc, {0xbe, 0x8a, 0x12, 0x85, 0xe9, 0x70, 0xda, 0x44 } }
{0x39927901, 0x7361, 0x4424, {0xa2, 0x5a, 0x64, 0xe1, 0xa8, 0x13, 0xd4, 0x75 } }

*/

const bool default_enable_remote = true, default_enable_console = false;
#define DEFAULT_REMOTE_IP "127.0.0.1"
#define DEFAULT_RENDER_TEXT "%title%"
enum {
	default_remote_port = 9588
};

static cfg_bool cfg_enable_remote(guid_cfg_enable_remote, default_enable_remote),cfg_enable_console(guid_cfg_enable_console, default_enable_console);
static cfg_string cfg_remote_ip(guid_cfg_remote_ip, DEFAULT_REMOTE_IP);
static cfg_int cfg_remote_port(guid_cfg_remote_port, default_remote_port);
static cfg_string cfg_render_text(guid_cfg_render_text, DEFAULT_RENDER_TEXT);

static bool mark_render_changed = true;

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum {IDD = IDD_MYPREFERENCES};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP_EX(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_CHECK_ENABLE_REMOTE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_CHECK_ENABLE_CONSOLE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_IPADDRESS_REMOTE, IPN_FIELDCHANGED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_EDIT_REMOTE_PORT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_EDIT_RENDER_NAME, EN_CHANGE, OnEditChange)

		COMMAND_HANDLER(IDC_BUTTON_REFRESH_PRIVIEW, BN_CLICKED, OnBnClickedButtonRefreshPriview)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	void update_render_text();

	const preferences_page_callback::ptr m_callback;

	titleformat_object::ptr r_text_ptr;
public:
	LRESULT OnBnClickedButtonRefreshPriview(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_REMOTE)).SetCheck(cfg_enable_remote);
	((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_CONSOLE)).SetCheck(cfg_enable_console);

	uSetDlgItemText(*this, IDC_IPADDRESS_REMOTE, cfg_remote_ip);
	SetDlgItemInt(IDC_EDIT_REMOTE_PORT, cfg_remote_port);
	uSetDlgItemText(*this, IDC_EDIT_RENDER_NAME, cfg_render_text);
	update_render_text();
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();

	update_render_text();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	/*
	SetDlgItemInt(IDC_BOGO1, default_cfg_bogoSetting1, FALSE);
	SetDlgItemInt(IDC_BOGO2, default_cfg_bogoSetting2, FALSE);
	*/
	((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_REMOTE)).SetCheck(default_enable_remote);
	((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_CONSOLE)).SetCheck(default_enable_console);

	uSetDlgItemText(*this, IDC_IPADDRESS_REMOTE, DEFAULT_REMOTE_IP);
	SetDlgItemInt(IDC_EDIT_REMOTE_PORT, default_remote_port);
	uSetDlgItemText(*this, IDC_EDIT_RENDER_NAME, DEFAULT_RENDER_TEXT);

	update_render_text();
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_enable_remote = ((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_REMOTE)).GetCheck();
	cfg_enable_console = ((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_CONSOLE)).GetCheck();

	cfg_remote_ip = uGetDlgItemText(*this, IDC_IPADDRESS_REMOTE).c_str();

	cfg_remote_port = GetDlgItemInt(IDC_EDIT_REMOTE_PORT);
	if (cfg_remote_port > 65535 || cfg_remote_port < 0) {
		//input error:0 <= port <= 65535
		SetDlgItemInt(IDC_EDIT_REMOTE_PORT, default_remote_port);
		cfg_remote_port = default_remote_port;
	}
	cfg_render_text = uGetDlgItemText(*this, IDC_EDIT_RENDER_NAME).c_str();

	mark_render_changed = true;

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return
	cfg_enable_remote != bool(((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_REMOTE)).GetCheck())||
	cfg_enable_console != bool(((CCheckBox)GetDlgItem(IDC_CHECK_ENABLE_CONSOLE)).GetCheck())||
	uGetDlgItemText(*this, IDC_IPADDRESS_REMOTE) != cfg_remote_ip||
	cfg_remote_port != GetDlgItemInt(IDC_EDIT_REMOTE_PORT)||
	uGetDlgItemText(*this, IDC_EDIT_RENDER_NAME) != cfg_render_text
		;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

void CMyPreferences::update_render_text() {

	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(r_text_ptr, uGetDlgItemText(*this,IDC_EDIT_RENDER_NAME).c_str());

	pfc::string_formatter fmt;
	if (playback_control::get()->playback_format_title(NULL, fmt, r_text_ptr, NULL, playback_control::display_level_all)) {
		uSetDlgItemText(*this, IDC_EDIT_RENDER_NAME_RENDEROUTPUT, fmt.c_str());
	}
	else {
		uSetDlgItemText(*this, IDC_EDIT_RENDER_NAME_RENDEROUTPUT, "//ERROR");
	}
	
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "Flyric client Component";}
	GUID get_guid() {
		static const GUID guid = { 0xce3d1fcd, 0x18a1, 0x4d80, {0xaf, 0xd1, 0x46, 0xca, 0x13, 0x29, 0xfb, 0x0b } };
		return guid;
	}
	GUID get_parent_guid() {return guid_tools;}
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;


LRESULT CMyPreferences::OnBnClickedButtonRefreshPriview(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	update_render_text();
	return 0;
}

bool flyric_client_cfg_enable_remote() {
	return cfg_enable_remote;
}
bool flyric_client_cfg_enable_console() {
	return cfg_enable_console;
}
const char * flyric_client_cfg_remote_ip() {
	return cfg_remote_ip;
}
int flyric_client_cfg_remote_port() {
	return cfg_remote_port;
}

const char* flyric_cfg_changed_render_text() {
	if (mark_render_changed) {
		mark_render_changed = false;
		return cfg_render_text;
	}
	return NULL;
}
const char* flyric_cfg_render_text() {
	return cfg_render_text;
}